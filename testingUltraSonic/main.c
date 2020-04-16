/********************************************************************/
/* Project: Ultrasonice based Distance Meter with LCD Display       */
/* Microcontroller: MSP430G2231 on MSP-EXP430G2 Launchpad           */
/* Ultrasonic Ranging Module: HC-SR04                               */
/* 16x2 LCD Display: 1602K27-00                                     */
/********************************************************************/
/* Build command on Linux:
    $ msp430-gcc -mmcu=msp430g2553 -g -o msp_dist.elf msp_dist.c    */
/********************************************************************/
/* Board jumper changes:
    1. Isolate LEDs connected to P1.0 and P1.6 by
    removing Jumpers cap J5.
    2. Isolate RX/TX connected to P1.1 and P1.2 by
    removing those Jumper cap in J3                                 */
/********************************************************************/
/* uC and Ultrasonic sensor Connections
    P2.1 - Trigger
    P2.2 - Echo - This should not be changed!
                - Why P1.1? - msp430g2231 datasheet mention this as
                - input for Timer A0 - Compare/Capture input        */
/********************************************************************/
/* uC and LCD Connections
    TP1 - Vcc (+5v)
    TP3 - Vss (Gnd)
    P3.0 - EN
    P3.1 - RS
    P3.2 - D4
    P3.3 - D5
    P3.6 - D6
    P3.7 - D7
    Gnd  - RW
    Gnd  - Vee/Vdd - Connect to Gnd through a 1K Resistor
            - this value determines contrast -
            - i.e. without resistor all dots always visible, whereas
            - higher resistor means dots not at all displayed.
    Gnd  - K (LED-)
    Vcc  - A (LED+) +5V - For Backlight
    Clock: 1MHz                                                     */
/********************************************************************/

#include <msp430.h> 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// uC GPIO Port assignment
#define UC_PORT     P3OUT
#define UC_PORT_DIR P3DIR
#define US_PORT     P2OUT
#define US_PORT_DIR P2DIR

// Peripheral pin assignments
#define US_TRIG         BIT1
#define US_ECHO         BIT2
#define LCD_EN          BIT0
#define LCD_RS          BIT1
#define LCD_DATA        BIT2 | BIT3 | BIT6 | BIT7
#define LCD_D0_OFFSET   4   // D0 at BIT4, so it is 4
#define US_MASK         US_TRIG | US_ECHO
#define LCD_MASK        LCD_EN | LCD_RS | LCD_DATA

// Debug LEDs
#define LED1 BIT0

unsigned int up_counter;
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
    TB0CCTL4 &= ~CCIFG;
    //TB0CTL &= ~TAIFG;           // Clear interrupt flags - handled
}

unsigned char set_data(unsigned char value)
{
    unsigned char val1 = (value << 4) & 0xC0;
    unsigned char val2 = (value << 2) & 0x0C;
    return (val1)|(val2);
}

void timer_init()
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
    Timer B clock input divider 0 - No divider */
    TB0CTL |= TBSSEL_2 + MC_2 + ID_0;

    // Global Interrupt Enable
    _BIS_SR(GIE);

}

void lcd_reset()
{
    UC_PORT = 0x00;
    __delay_cycles(20000);

    UC_PORT = (set_data(0x03)) | LCD_EN;
    UC_PORT &= ~LCD_EN;
    __delay_cycles(10000);

    UC_PORT = (set_data(0x03)) | LCD_EN;
    UC_PORT &= ~LCD_EN;
    __delay_cycles(1000);

    UC_PORT = (set_data(0x03)) | LCD_EN;
    UC_PORT &= ~LCD_EN;
    __delay_cycles(1000);

    UC_PORT = (set_data(0x02)) | LCD_EN;
    UC_PORT &= ~LCD_EN;
    __delay_cycles(1000);

}

void lcd_cmd (char cmd)
{
    // Send upper nibble
    UC_PORT = (set_data((cmd >> 4) & 0x0F) | LCD_EN);
    UC_PORT &= ~LCD_EN;

    // Send lower nibble
    UC_PORT = (set_data(cmd & 0x0F)) | LCD_EN;
    UC_PORT &= ~LCD_EN;

    __delay_cycles(5000);
}

void lcd_data (unsigned char dat)
{
    // Send upper nibble
    UC_PORT = (set_data((dat >> 4) & 0x0F) | LCD_EN | LCD_RS);
    UC_PORT &= ~LCD_EN;

    // Send lower nibble
    UC_PORT = (set_data(dat & 0x0F) | LCD_EN | LCD_RS);
    UC_PORT &= ~LCD_EN;

    __delay_cycles(5000); // a small delay may result in missing char display
}

void lcd_init ()
{
    lcd_reset();         // Call LCD reset

    lcd_cmd(0x28);       // 4-bit mode - 2 line - 5x7 font.
    lcd_cmd(0x0C);       // Display no cursor - no blink.
    lcd_cmd(0x06);       // Automatic Increment - No Display shift.
    lcd_cmd(0x80);       // Address DDRAM with 0 offset 80h.
    lcd_cmd(0x01);       // Clear screen

}

void display_line(char *line)
{
    while (*line)
        lcd_data(*line++);
}

void display_distance(char *line, int len)
{
    while (len--)
        if (*line)
            lcd_data(*line++);
        else
            lcd_data('    ');
}



int main()
{
    char distance_string[4];

    WDTCTL = WDTPW + WDTHOLD;       // Stop Watch Dog Timer
    // Enable the GPIO pins
    PM5CTL0 &= ~LOCKLPM5;
    UC_PORT_DIR = LCD_MASK;         // Output direction for LCD connections
    US_PORT_DIR |= US_TRIG;         // Output direction for trigger to sensor
    US_PORT_DIR &= ~US_ECHO;        // Input direction for echo from sensor
    US_PORT &= ~US_TRIG;            // keep trigger at low
    //P2SEL = US_ECHO;                // set US_ECHO as trigger for Timer from Port-1
    P2SEL1 |= US_ECHO;
    P2SEL0 &= ~US_ECHO;


    P1DIR |= LED1; // Set P1.0 as output
    P1OUT &= ~LED1; // Make sure LED is off
    //setup_SMCLK();

    // Initialize LCD
    lcd_init();

    // Initialize timer for Ultrasonice distance sensing
    timer_init();
    lcd_cmd(0x80); // select 1st line (0x80 + addr) - here addr = 0x00
    display_line(" Distance Meter");
    lcd_cmd(0xce); // select 2nd line (0x80 + addr) - here addr = 0x4e
    display_line("cm");

    _enable_interrupts();
    while (1)
    {
        // measuring the distance
        US_PORT ^= US_TRIG;                 // assert
        __delay_cycles(10);                 // 10us wide
        US_PORT ^= US_TRIG;                 // deassert
        //__delay_cycles(60000);            // 60ms measurement cycle
        __delay_cycles(500000);             // 0.5sec measurement cycle
        P1OUT ^= LED1;
        //distance_cm = TB0CCR4;
        // displaying the current distance
        //itoa(distance_cm, distance_string, 10);
        sprintf(distance_string, "%d", distance_cm);
        lcd_cmd(0xcb); // select 2nd line (0x80 + addr) - here addr = 0x4b
        display_distance(distance_string, 3);
    }

}
