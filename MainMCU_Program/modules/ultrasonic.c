#include <msp430.h>
#include "modules.h"

/********************************************************************/
/* uC and Ultrasonic sensor Connections
    P2.1 - Trigger
    P2.2 - Echo - This should not be changed!
                - Why P2.2? - msp430fr6989 datasheet mention this as
                - input for Timer B0 - Compare/Capture input        */
/********************************************************************/

// US GPIO Port assignments
#define US_PORT     P2OUT
#define US_PORT_DIR P2DIR

// Peripheral pin assignments
#define US_TRIG         BIT1
#define US_ECHO         BIT2

static unsigned int up_counter;
unsigned int distance_cm;

/* Timer B0 Capture Interrupt routine
 P2.2 (echo) causes this routine to be called */
#pragma vector=TIMER0_B1_VECTOR
__interrupt void TimerB0(void)
{

    if (TB0CCTL4 & CCI)            // Raising edge
    {
        up_counter = TB0CCR4;      // Copy counter to variable
        //P1OUT |= LED1;

    }
    else                        // Falling edge
    {
        // Formula: Distance in cm = (Time in uSec)/58
        distance_cm = (TB0CCR4 - up_counter) / 58;
    }
    TB0CCTL4 &= ~CCIFG;          // Clear interrupt flags - handled
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

    /* Timer B Control configuration =>
    Timer B clock source select: 2 - SMClock +
    Timer B mode control: 2 - Continous up +
    Timer B clock input divider 3 - Divide by 8 */
    TB0CTL |= TBSSEL_2 + MC_2 + ID_3;

    // Global Interrupt Enable
    _BIS_SR(GIE);

}

void us_init() {
    US_PORT_DIR |= US_TRIG;         // Output direction for trigger to sensor
    US_PORT_DIR &= ~US_ECHO;        // Input direction for echo from sensor
    US_PORT &= ~US_TRIG;            // keep trigger at low
    P2SEL1 |= US_ECHO;              // set US_ECHO as trigger for Timer from Port-1
    P2SEL0 &= ~US_ECHO;
}

void getDistance(unsigned int * data) {
    timer_init();
    _enable_interrupts();

    // measuring the distance
    US_PORT |= US_TRIG;                 // assert
    __delay_cycles(160);                 // 10us wide on 16MHz clock
    US_PORT &= ~US_TRIG;                 // deassert
    __delay_cycles(1120000);            // 70ms with 16MHz clock measurement cycle
//    __delay_cycles(500000);             // 0.5sec measurement cycle

    // Disable timer interrupt
    TB0CCTL4 &= ~CCIE;

    *data = distance_cm;
}
