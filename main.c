#include <msp430.h>

long Last_ADC = 0;
float storage = 0;
int voltage;
unsigned char high;

float Convert_VtoR(float vout);                
float Convert_RtoT(float resist);

void UART_Setup();
void Board_Setup();
void Timer_Setup();

int main(void)
{
    float temp;
    Board_Setup();
    UART_Setup();
while(1){
    temp = Convert_VtoR(Last_ADC);
    temp = Convert_RtoT(temp);
    TA0CCR1 = temp * 630;
}





  while (1)
  {
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
      Last_ADC = ADC12MEM0;
    //Convert the ADC number, back to a voltage
 /*   storage = (float)ADC12MEM0;               // cast the ADC value to a float)
    storage = storage * 3.3;                  // Multiply by the max voltage
    storage = storage / 495;                  // Divide by scaling value
    storage = storage * 1000;                 // scale up the voltage value so we don't lose decimals and accuracy
    voltage = (int)storage;                   // cast float to an int, store in voltage
    */
    //Transmit the Voltage over UART in TWO pieces (Total of 16 bits)
    high = voltage >> 8;                      // Bt shift voltage over by 8 bits and store in "High"
    UCA1TXBUF = high;
    UCA1TXBUF = voltage;
    
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
    UCA1CTL1 |= UCSWRST;                      // **Put state machine in reset**
    UCA1CTL1 |= UCSSEL_1;                     // CLK = ACLK
    UCA1BR0 = 0x03;                           // 32kHz/9600=3.41 (see User's Guide)
    UCA1BR1 = 0x00;                           //
    UCA1MCTL = UCBRS_3+UCBRF_0;               // Modulation UCBRSx=3, UCBRFx=0
    UCA1CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
    UCA1IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt
}

void Board_Setup(){
    WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
    ADC12CTL0 = ADC12SHT02 + ADC12ON;         // Sampling time, ADC12 on
    ADC12CTL1 = ADC12SHP;                     // Use sampling timer
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
    //P1DIR |= BIT2+BIT3;                     // P1.2 and P1.3 output
    //P1SEL |= BIT2+BIT3;                     // P1.2 and P1.3 options select
    TA0CCR0 = 65536;                          // PWM Period
    TA0CCTL1 = OUTMOD_7;                      // CCR1 reset/set
    TA0CCTL2 = OUTMOD_7;                      // CCR2 reset/set
    TA0CTL = TASSEL_2 + MC_1 + TACLR;         // SMCLK, up mode, clear TAR

    TA0CCR1 = 65535;                          // CCR1 PWM
    TA0CCR2 = 65534;                          // CCR2 PWM
}

float Convert_VtoR(float vout){               // funtion for converting the vout value to resistance
  float R2_value;

  R2_value = (vout * 10000) / (vout - 3.3);

  return R2_value;
}

float Convert_RtoT(float R2_value){             // // function for converting the resistance value to temperature
  float resist;
  float temperature;
  resist = R2_value;

  if ((resist <= 32554) & (resist > 19872)){
    temperature = - 0.0008 * resist + 25.311;
  }
  else if ((resist <= 19872) & (resist > 10000)){
    temperature = - 0.0015 * resist + 39.319;
  }
  else if ((resist <= 10000) & (resist > 4372)){
    temperature = - 0.0035 * resist + 59.061;
  }
  else if ((resist <= 4372) & (resist > 1753)){
    temperature = - 0.0094 * resist + 84.646;
  }
  else if ((resist <= 1753) & (resist > 786)){
    temperature = - 0.0256 * resist + 113.46;
  }
  else if ((resist <= 786) & (resist > 442.6)){
    temperature = - 0.0579 * resist + 139.74;
  }
  else if ((resist <= 442.6) & (resist > 182.6)){
    temperature = - 0.1334 * resist + 171.64;
  }
  return temperature;
}
