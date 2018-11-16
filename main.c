#include <msp430.h>
#include <Functions.h>


float storage = 0;
int voltage;
unsigned char high;

//float Convert_VtoR(float vout);
//float Convert_RtoT(float resist);

void UART_Setup();
void Board_Setup();
void Timer_Setup();
void Set_PWM(int percent);

int main(void)
{//Board Setup
  int Target_Temperature = 33;
  float Temperature_Offset;
  float Current_Temperature;
  float Swap_Space;
  Board_Setup();
  UART_Setup();
  // Get initial temperature
Swap_Space = Convert_VtoR(ADC12MEM0);
Current_Temperature = Convert_RtoT(Swap_Space);
Temperature_Offset = Current_Temperature - Target_Temperature;
if(Temperature_Offset >= 1){
    Set_PWM((Temperature_Offset * 7) + 15);
    }else{
    Set_PWM(10);
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
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  ADC12CTL0 = ADC12SHT02 + ADC12ON;         // Sampling time, ADC12 on
  ADC12CTL1 = ADC12SHP + ADC12CONSEQ_2;                     // Use sampling timer +  ADC12 on sample single channel repeatedly
  ADC12IE = 0x01;                           // Enable interrupt
  ADC12CTL0 |= ADC12ENC;
  P6SEL |= 0x01;                            // P6.0 ADC option select
  P1DIR |= 0x01;                            // P1.0 output

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
  TA0CTL = TASSEL_2 + MC_1 + TACLR;           // SMCLK, up mode, clear TAR
  TA0CCR1 = 100;                              // CCR1 PWM
}

void Set_PWM(int percent){
  if (percent >= 15){
    TA0CCR1 = percent;
  }                     
  else{
    TA0CCR1 = 15;
  }
}


