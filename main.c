#include <msp430.h>
#include <Functions.h>


float storage = 0;
int voltage;
unsigned char high;
int Past_Temperature[5] ={0,0,0,0,0};
int A = 0;
long ADC12;
int PWM_Diff = 0;

//float Convert_VtoR(float vout);
//float Convert_RtoT(float resist);

void UART_Setup();
void Board_Setup();
void Timer_Setup();
void Set_Pwm(int percent);
void New_PWM(float PWM);

int main(void){
  //Board Setup
  int Target_Temperature = 60;

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
  
A = 10;
Set_Pwm (A);

  Past_Temperature[0] = Current_Temperature;
  Past_Temperature[1] = Current_Temperature;
  Past_Temperature[2] = Current_Temperature;
  Past_Temperature[3] = Current_Temperature;
  Past_Temperature[4] = Current_Temperature;





while(1){
    Swap_Space = Convert_VtoR(ADC12MEM0);
    Current_Temperature = Convert_RtoT(Swap_Space);
    UCA0TXBUF = Current_Temperature;
    Temperature_Offset = Current_Temperature - Target_Temperature;

//PWM_Diff = (((float)A / 10) + 0.5)* Temperature_Offset;
UCA1TXBUF = Current_Temperature;
UCA1TXBUF = (char)A;
if(Temperature_Offset > 0){
    A = A + 1;
}else if (Temperature_Offset < 0){
    A = A - 1;
}
UCA1TXBUF = (char)A;
Set_Pwm(A);
}




  // end while

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

  P6SEL |= 0x01;                            // Enable A/D channel A0
   ADC12CTL0 = ADC12ON+ADC12SHT0_8+ADC12MSC; // Turn on ADC12, set sampling time
                                             // set multiple sample conversion
   ADC12CTL1 = ADC12SHP+ADC12CONSEQ_2;       // Use sampling timer, set mode
   ADC12IE = 0x01;                           // Enable ADC12IFG.0
   ADC12CTL0 |= ADC12ENC;                    // Enable conversions
   ADC12CTL0 |= ADC12SC;                     // Start conversion


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
  //TA0CCTL1 = OUTMOD_6;                          //Toggle Set
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
 if ((percent >= 0) && (percent <= 100)){
    TA0CCR1 = percent;
  }else if (percent > 100){
    TA0CCR1 = 100;
    //New_PWM(100);
    A = 100;
  }else{
    TA0CCR1 = 0;
    //New_PWM(15);
    A = 0;
  }

}

