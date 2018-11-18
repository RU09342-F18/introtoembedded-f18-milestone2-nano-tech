#include <msp430.h>
#include <Functions.h>


float storage = 0;
int voltage;
unsigned char high;
int Past_Temperature[5] ={0,0,0,0,0};

//float Convert_VtoR(float vout);
//float Convert_RtoT(float resist);

void UART_Setup();
void Board_Setup();
void Timer_Setup();
void Set_PWM(int percent);

int main(void){
  //Board Setup
  int Target_Temperature = 33;
  int Current_PWM;
  float Temperature_Offset;
  float Current_Temperature;    //Legacy, should be replaced by Past_Temperature[0]
  float Swap_Space;             //Swap space only to be used by the main function
  float Slope_Aggresion = 0.5;  //How aggressively should the program change the fan speed
  float Slope;

  Board_Setup();
  UART_Setup();
  
  // Get initial temperature
  Swap_Space = Convert_VtoR(ADC12MEM0);
  Current_Temperature = Convert_RtoT(Swap_Space);
  Temperature_Offset = Current_Temperature - Target_Temperature;
  
  if(Temperature_Offset >= 1){
      Current_PWM = (Temperature_Offset * 7) + 15;
    Set_PWM(Current_PWM);
  }
      else{
          Current_PWM = 10;
    Set_PWM(Current_PWM);
  }
  Past_Temperature[0] = Current_Temperature;
  Past_Temperature[1] = Current_Temperature;
  Past_Temperature[2] = Current_Temperature;
  Past_Temperature[3] = Current_Temperature;
  Past_Temperature[4] = Current_Temperature;
  while(1){
      //Shift Values
      Past_Temperature[4] = Past_Temperature[3];
      Past_Temperature[3] = Past_Temperature[2];
      Past_Temperature[2] = Past_Temperature[1];
      Past_Temperature[1] = Past_Temperature[0];
      //Get next Temperature
      Swap_Space = Convert_VtoR(ADC12MEM0);
      Past_Temperature[0] = Convert_RtoT(Swap_Space);
      //Get the Slope
      Slope = Detect_Change(Past_Temperature[0], Past_Temperature[1], Past_Temperature[2], Past_Temperature[3], Past_Temperature[4]);
      /*
        +----------+---------------------+---------------------+
        |          | Negative Slope      | Positive Slope      |
        +----------+---------------------+---------------------+
        | Too High | Slow down A bit     | Make Drastic Change |
        |          | or                  |                     |
        |          | Speed up A bit      |                     |
        +----------+---------------------+---------------------+
        | Too Low  | Make Drastic Change | Slow down A bit     |
        |          |                     | or                  |
        |          |                     | Speed up A bit      |
        +----------+---------------------+---------------------+
       */
      //Adjust fan speed
      Temperature_Offset = Current_Temperature - Target_Temperature;
      if(Temperature_Offset > 0 && Slope < 0){                                                  // Temperature is too high and the slope is negative
          if(Slope_Aggresion <= Abs_Val(Slope)){                                                    //The temperature is changing too fast!
              Current_PWM = Current_PWM - (Current_PWM * (Abs_Val(Slope) - Slope_Aggresion))        //Slow down the Fan
                Set_PWM(Current_PWM);
          }
      }else if (Temperature_Offset > 0 && Slope > 0){                                           //Temperature is too high and the slope is positive
              Current_PWM = Current_PWM + (Current_PWM * (Abs_Val(Slope) + Slope_Aggresion));       //Speed up the Fan
              Set_PWM(Current_PWM);

      }else if (Temperature_Offset < 0 && Slope < 0){                                           //Temperature is too Low and the slope is negative
          Current_PWM = Current_PWM - (Current_PWM * (Abs_Val(Slope) - Slope_Aggresion));            //Slow down the Fan
          Set_PWM(Current_PWM);
      }else if (Temperature_Offset < 0 && Slope > 0){                                           //Temperature is too low and slope is positive
          if(Slope_Aggresion <= Abs_Val(Slope)){                                                    //Temperature is changing too fast!
              Current_PWM = Current_PWM + (Current_PWM * (Abs_Val(Slope) + Slope_Aggresion));       //Speed up  the Fan
              Set_PWM(Current_PWM);
          }
      }else{                                                                                    //All the edge cases
          if(Temperature_Offset > 0 && Slope == 0){                                                 //Temperature is too high and slope is steady
              Current_PWM = Current_PWM + (Past_Temperature[0] - Target_Temperature);                   //Increase the fan speed by some amount and wait for change
              Set_PWM(Current_PWM);
          }else if(Temperature_Offset < 0 && Slope == 0){                                          //Temperature is too low and slope is steady
            Current_PWM = Current_PWM + (Past_Temperature[0] - Target_Temperature);                    //Decrease the fan speed by some amount and wait for change
            Set_PWM(Current_PWM);
          }else if(Temperature_Offset == 0){                                                        //We are EXACTLY on the temperature we want!
              if(Slope > 0){
                  Current_PWM = Current_PWM - (Current_PWM * (Abs_Val(Slope) - Slope_Aggresion))        //Slow down the Fan
              }else if(Slope < 0){
                  Current_PWM = Current_PWM + (Current_PWM * (Abs_Val(Slope) + Slope_Aggresion));       //Speed up the Fan
              }
          }

      }
  }



  while (1){
    ADC12CTL0 |= ADC12SC;                     // Start sampling/conversion

    __bis_SR_register(LPM0_bits + GIE);       // LPM0, ADC12_ISR will force exit
    __no_operation();                         // For debugger
  }
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC12_VECTOR))) ADC12_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(ADC12IV,34))
  {
  case  6:                                    // Vector  6:  ADC12IFG0
    /*
    //Transmit the Voltage over UART in TWO pieces (Total of 16 bits)
    high = voltage >> 8;                      // Bt shift voltage over by 8 bits and store in "High"
    UCA1TXBUF = high;
    UCA1TXBUF = voltage;
    */
    //Flash a light
    if (ADC12MEM0 >= 0x7ff){                  // ADC12MEM = A0 > 0.5AVcc?
      P1OUT |= BIT0;                          // P1.0 = 1
    }
    else{
      P1OUT &= ~BIT0;                         // P1.0 = 0
    }
    __bic_SR_register_on_exit(LPM0_bits);     // Exit active CPU
  default: break;
  }
}
void UART_Setup(){
  UCA1CTL1 |= UCSWRST;                        // **Put state machine in reset**
  UCA1CTL1 |= UCSSEL_1;                       // CLK = ACLK
  UCA1BR0 = 0x03;                             // 32kHz/9600=3.41 (see User's Guide)
  UCA1BR1 = 0x00;                             //
  UCA1MCTL = UCBRS_3+UCBRF_0;                 // Modulation UCBRSx=3, UCBRFx=0
  UCA1CTL1 &= ~UCSWRST;                       // **Initialize USCI state machine**
  UCA1IE |= UCRXIE;                           // Enable USCI_A0 RX interrupt
}

void Board_Setup(){
  WDTCTL = WDTPW + WDTHOLD;                   // Stop WDT
  ADC12CTL0 = ADC12SHT02 + ADC12ON;           // Sampling time, ADC12 on
  ADC12CTL1 = ADC12SHP + ADC12CONSEQ_2;       // Use sampling timer +  ADC12 on sample single channel repeatedly
  ADC12IE = 0x01;                             // Enable interrupt
  ADC12CTL0 |= ADC12ENC;
  P6SEL |= 0x01;                              // P6.0 ADC option select
  P1DIR |= 0x01;                              // P1.0 output

  //Setup  Timer 0.1 output to board
  //P1.2
  P1SEL |= BIT2;
  P1DIR |= BIT2;
}

void Timer_Setup(){
  //P1DIR |= BIT2+BIT3;                       // P1.2 and P1.3 output
  //P1SEL |= BIT2+BIT3;                       // P1.2 and P1.3 options select
  TA0CCR0 = 100;                              // PWM Period
  TA0CCTL1 = OUTMOD_2;                        // Toggle or Reset Behavior
  TA0CTL = TASSEL_2 + MC_1 + TACLR;           // AMCLK, up mode, clear TAR
  TA0CCR1 = 100;                              // CCR1 PWM

  TA1CTL = TASSEL_1 + MC_1 + TACLR;
  TA1CCR0 = 32768;
  TA1CCTL1 = CCIE;
  }
}

#pragma vector=TIMER0_A1_VECTOR
__interrupt void TIMER_A1(void){
  TA1CCR1 += 3277;
}

void Set_PWM(int percent){
  if (percent >= 15){
    TA0CCR1 = percent;
  }                     
  else{
    TA0CCR1 = 15;
  }
}


