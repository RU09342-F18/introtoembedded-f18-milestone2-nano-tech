#include <msp430.h>
#include <Functions.h>


float storage = 0;
int voltage;
unsigned char high;
int Past_Temperature[5] ={0,0,0,0,0};
int Current_PWM;
long ADC12;

//float Convert_VtoR(float vout);
//float Convert_RtoT(float resist);

void UART_Setup();
void Board_Setup();
void Timer_Setup();
void Set_Pwm(int percent);
void New_PWM(float PWM);

int main(void){
  //Board Setup
  int Target_Temperature = 33;

  float Temperature_Offset;
  float Current_Temperature;    //Legacy, should be replaced by Past_Temperature[0]
  float Swap_Space;             //Swap space only to be used by the main function
  float Slope_Aggresion = 0.5;  //How aggressively should the program change the fan speed
  float Slope;
  char C_Current;

  Board_Setup();
  UART_Setup();
  Timer_Setup();
  
  // Get initial temperature
  Swap_Space = Convert_VtoR(ADC12MEM0);
  Current_Temperature = Convert_RtoT(Swap_Space);
  UCA0TXBUF = Current_Temperature;
  Temperature_Offset = Current_Temperature - Target_Temperature;
  
  if(Temperature_Offset >= 1){
    Current_PWM = (Temperature_Offset * 7) + 15;
    Set_Pwm(Current_PWM);
  }else{
    Current_PWM = 10;
    Set_Pwm(Current_PWM);
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
    UCA0TXBUF = Past_Temperature[0];

    //Get the Slope
    Slope = Detect_Change(Past_Temperature[0], Past_Temperature[1], Past_Temperature[2], Past_Temperature[3], Past_Temperature[4]);

    /*
      +-----------+---------------------+---------------------+
      |Temperature| Negative Slope      | Positive Slope      |
      +-----------+---------------------+---------------------+
      | Too High  | Slow down A bit     | Make Drastic Change |
      |           | or                  |                     |
      |           | Speed up A bit      |                     |
      +-----------+---------------------+---------------------+
      | Too Low   | Make Drastic Change | Slow down A bit     |
      |           |                     | or                  |
      |           |                     | Speed up A bit      |
      +-----------+---------------------+---------------------+
      */

    //Adjust fan speed
    //void Set_Pwm(int thing);

    float Current_PWM;
    float Slope_Aggression;
    float Slope;
    float Temperature_Offset; //make this an int
    int PWM_Diff; //New int

    //Calculate the recoommended change first
    //PWM_Diff = Current_PWM * (Abs_Val(Slope) - Slope_Aggression);         //This is how much we want to change the fan PWM by
    PWM_Diff = (((float)Current_PWM / 10) + 0.5)* Temperature_Offset;
    if(PWM_Diff >= Current_PWM / 3){                                    //Check if we are changing it by too much
    PWM_Diff = Current_PWM * 0.3;                                       //Lets only change the PWM by at most 30% of it's current value
    }
    //All conditions based on current temperature
    switch((int)Temperature_Offset){
        case -5 :
        case -4 :
        case -3 :
            if(Slope > 0){                                              //Temperature is too low and it's going up, GOOD!
                if(Slope_Aggression <= Slope){                          //Is the temperature rising too fast?
                    Current_PWM = Current_PWM + PWM_Diff;
                    Set_Pwm(Current_PWM);
                }               //The lets speed up the fan a bit
            }else if (Slope <= 0){
                Current_PWM = Current_PWM - PWM_Diff;
                Set_Pwm(Current_PWM);
            }
        break;
        case -2 :
            if(Slope > 0){
                if(Slope_Aggression <= Slope){
                    Current_PWM = Current_PWM + PWM_Diff;
                    Set_Pwm(Current_PWM);
                }else{
                    Current_PWM = Current_PWM + 3;
                    Set_Pwm(Current_PWM);                                   //Make a tiny adjustment
                }
            }else if (Slope < 0){
                Current_PWM = Current_PWM - PWM_Diff;
                    Set_Pwm(Current_PWM);
            }
        break;
        case -1 :
            if (Slope > 0){
                if(Slope_Aggression <= Slope){
                    Current_PWM = Current_PWM + PWM_Diff;
                    Set_Pwm(Current_PWM);
                }else{
                    Current_PWM = Current_PWM + 2;
                    Set_Pwm(Current_PWM);                               //Make a tiny adjustment
                }

            }else if (Slope < 0){
                if(Slope_Aggression <= Abs_Val(Slope)){
                    Current_PWM = Current_PWM - PWM_Diff;
                    Set_Pwm(Current_PWM);
                }else{
                    Current_PWM = Current_PWM - 2;
                    Set_Pwm(Current_PWM);
                }
            }
        break;
        case 0 :
            if (Slope > 0){
                if(Slope_Aggression <= Slope){
                    Current_PWM = Current_PWM + PWM_Diff;
                    Set_Pwm(Current_PWM);
                }else{
                    Current_PWM = Current_PWM + 1;
                    Set_Pwm(Current_PWM);
                }
            }else if(Slope < 0){
                if(Slope_Aggression <= Slope){
                    Current_PWM = Current_PWM + PWM_Diff;
                    Set_Pwm(Current_PWM);
                }//else                                             I had something planned here?
            }
        break;
        case 1 :
            if (Slope > 0){
                if(Slope_Aggression <= Abs_Val(Slope)){
                    Current_PWM = Current_PWM + PWM_Diff;
                    Set_Pwm(Current_PWM);
                }else{
                    Current_PWM = Current_PWM + 2;
                    Set_Pwm(Current_PWM);
                }
            }else if (Slope < 0){
                if(Slope_Aggression <= Abs_Val(Slope)){
                    Current_PWM = Current_PWM - PWM_Diff;
                    Set_Pwm(Current_PWM);
                }else{
                    Current_PWM = Current_PWM - 2;
                    Set_Pwm(Current_PWM);
                }
            }
            break;
        case 2 :
            if (Slope > 0){
                if(Slope_Aggression <= Slope){
                    Set_Pwm(Current_PWM + PWM_Diff);
                }else{
                    Set_Pwm(Current_PWM + 3);
                }
            }else if (Slope < 0){
                if (Slope_Aggression <= Abs_Val(Slope)){
                    Current_PWM = Current_PWM - PWM_Diff;
                    Set_Pwm(Current_PWM);
                }else{
                    Current_PWM = Current_PWM -3;
                    Set_Pwm(Current_PWM);
                }
            }
            break;
        case 4 :
        case 5 :
        case 3 :
            if (Slope > 0){
                Current_PWM = Current_PWM - PWM_Diff;
                Set_Pwm(Current_PWM);
            }else{
                if (Slope_Aggression <= Abs_Val(Slope)){
                    Current_PWM = Current_PWM + 2;
                }
            }
            break;
        default :
            if (Temperature_Offset > 0){
                if(Slope >= 0){
                    Current_PWM = Current_PWM - PWM_Diff;
                    Set_Pwm(Current_PWM);
                }else{
                    if (Slope_Aggression <= Slope){
                        Current_PWM = Current_PWM + PWM_Diff;
                        Set_Pwm(Current_PWM);
                    }
                }
            }else if (Slope < 0){
                if(Slope > 0){
                    if(Slope_Aggression <= Abs_Val(Slope)){
                        Current_PWM = Current_PWM - PWM_Diff;
                        Set_Pwm(Current_PWM);
                    }

                }else{
                    Current_PWM = Current_PWM - PWM_Diff;
                    Set_Pwm(Current_PWM);
                }
            }
            break;


  }
            UCA1TXBUF = Current_PWM;
            C_Current = (char)Current_Temperature;
            UCA1TXBUF = C_Current;
  }// end while

  while (1){
    ADC12CTL0 |= ADC12SC;                     // Start sampling/conversion

    __bis_SR_register(LPM0_bits + GIE);       // LPM0, ADC12_ISR will force exit
    __no_operation();                         // For debugger
  }
} // end main 

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
    }else{
      P1OUT &= ~BIT0;                         // P1.0 = 0
    }
    __bic_SR_register_on_exit(LPM0_bits);     // Exit active CPU
    default: break;
  }
}

void UART_Setup(){
    P4SEL |= BIT4 + BIT5;
  UCA1CTL1 |= UCSWRST;                        // **Put state machine in reset**
  UCA1CTL1 |= UCSSEL_2;                       // CLK = ACLK
  UCA1BR0 = 9;                             // 32kHz/9600=3.41 (see User's Guide)
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
  P4DIR |= BIT7;                             
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

#pragma vector=TIMER0_A1_VECTOR
__interrupt void TIMER_A1(void){
  TA1CCR1 += 3277;
  P4OUT ^= BIT7;                             // Toggle LED
}

void Set_Pwm(int percent){
  if ((percent >= 15) && (percent <= 100)){
    TA0CCR1 = percent;
  }else if (percent > 100){
    TA0CCR1 = 100;
    New_PWM(100);
  }else{
    TA0CCR1 = 15;
    New_PWM(15);
  }
}

void New_PWM(float PWM){
    Current_PWM = PWM;
}
