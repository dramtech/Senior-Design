#include <msp430.h>
#include "modules.h"

/********************************************************************/
/* Right Ultrasonic sensor Connections
    P3.0 - Trigger
    P2.2 - Echo - This should not be changed!
                - Why P2.2? - msp430fr6989 datasheet mention this as
                - input for Timer B0 - Compare/Capture input

    Left Ultrasonic sensor Connections
    P3.1 - Trigger
    P2.1 - Echo */
/********************************************************************/
// Second sensor pins 3.0 and 3.1

// US GPIO Port assignments
#define US_PORT   P2OUT
#define US_PORT_DIR P2DIR

// Peripheral pin assignments
#define US_TRIG         BIT1
#define US_ECHO         BIT2

static unsigned int up_counter[2];
static unsigned char flag[2];
unsigned int distance_cm[2];

/* Timer B0 Capture Interrupt routine
 P2.2 (echo) causes this routine to be called */
#pragma vector=TIMER0_B1_VECTOR
__interrupt void TimerB0(void)
{
    if (TB0CCTL4 & CCI) {           // Raising edge
        up_counter[0] = TB0CCR4;      // Copy counter to variable
        flag[0] = 1;
        //P1OUT |= LED1;

    } else if(TB0CCTL5 & CCI) {
        up_counter[1] = TB0CCR5;      // Copy counter to variable
        flag[1] = 1;
    } else {                      // Falling edge
        if(flag[0]) {
            // Formula: Distance in cm = (Time in uSec)/58
            distance_cm[0] = (TB0CCR4 - up_counter[0]) / 58;
            flag[0] = 0;

        } else if(flag[1]) {
            // Formula: Distance in cm = (Time in uSec)/58
            distance_cm[1] = (TB0CCR5 - up_counter[1]) / 58;
            flag[1] = 0;

        }

    }
    TB0CCTL4 &= ~CCIFG;          // Clear interrupt flags - handled
    TB0CCTL5 &= ~CCIFG;          // Clear interrupt flags - handled
}

static void timer_init()
{
    /* Timer B0 configure to read echo signal:
    Timer B Capture/Compare Control 0 =>
    capture mode: 1 - both edges +
    capture sychronize +
    capture input select 0 => P1.1 (CCI4B) +
    capture mode +
    capture compare interrupt enable */
    TB0CCTL4 |= CM_3 + SCS + CCIS_1 + CAP + CCIE;
    TB0CCTL5 |= CM_3 + SCS + CCIS_1 + CAP + CCIE;

    /* Timer B Control configuration =>
    Timer B clock source select: 2 - SMClock +
    Timer B mode control: 2 - Continous up +
    Timer B clock input divider 3 - Divide by 8 */
    TB0CTL |= TBSSEL_2 + MC_2 + ID_3;

    // Global Interrupt Enable
    _BIS_SR(GIE);

}

void us_init() {
    P3DIR |= BIT0;                  // Output direction for RIGHT trigger to sensor
    P3DIR |= BIT1;                  // Output direction for LEFT trigger to sensor
    P2DIR &= ~BIT2;                 // Input direction for RIGHT echo from sensor
    P2DIR &= ~BIT1;                 // Input direction for LEFT echo from sensor

    P2OUT &= ~BIT2;                 // Set RIGHT trigger to low
    P2OUT &= ~BIT1;                // Set LEFT trigger to low

    P2SEL1 |= BIT2 | BIT1;              // set ECHO as trigger for Timer from Port-1 and 2
    P2SEL0 &= ~(BIT2 | BIT1);

    // Reset flags
    flag[0] = 0;
    flag[1] = 0;
}

void getDistance(unsigned int * data) {
    timer_init();
    _enable_interrupts();

    // measuring the distance
    P3OUT |= BIT0 | BIT1;                // assert both RIGHT and LEFT
    __delay_cycles(160);                 // 10us wide on 16MHz clock
    P3OUT  &= ~(BIT0 | BIT1);            // deassert
    __delay_cycles(1120000);             // 70ms with 16MHz clock measurement cycle
//    __delay_cycles(500000);            // 0.5sec measurement cycle

    // Disable timer interrupt
    TB0CCTL4 &= ~CCIE;
    TB0CCTL5 &= ~CCIE;

    data[0] = distance_cm[0];
    data[1] = distance_cm[1];
}
