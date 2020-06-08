/* --COPYRIGHT--,BSD
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
#include <msp430.h>
#include <stdio.h>
#include "grlib/grlib.h"
#include "LCD_driver/SSD1306_Driver.h"
#include "images/images.h"
//#include "driverlib.h"

#define redLED BIT0 // Red LED at P1.0
#define greenLED BIT7 // Green LED at P9.7

// Counter coordinates
#define COUNTER_X 45
#define COUNTER_Y 32

#define ONE_SEC 32768 // 1 second at 32kHz

uint8_t flag_counter = 0;
uint8_t flag_footer = 0;

#if defined(__IAR_SYSTEMS_ICC__)
int16_t __low_level_init(void) {
    // Stop WDT (Watch Dog Timer)
    WDTCTL = WDTPW + WDTHOLD;
    return(1);
}

#endif

//**********************************
// Configures ACLK to 32 KHz crystal
void config_ACLK_to_32KHz_crystal() {
    // By default, ACLK runs on LFMODCLK at 5MHz/128 = 39 KHz
    // Reroute pins to LFXIN/LFXOUT functionality
    PJSEL1 &= ~BIT4;
    PJSEL0 |= BIT4;

    // Wait until the oscillator fault flags remain cleared
    CSCTL0 = CSKEY; // Unlock CS registers
    do {
        CSCTL5 &= ~LFXTOFFG; // Local fault flag
        SFRIFG1 &= ~OFIFG; // Global fault flag
    } while((CSCTL5 & LFXTOFFG) != 0);
        CSCTL0_H = 0; // Lock CS registers
    return;
}

void config_MCLK_to_16MHz_DCO() {
    // Configure one FRAM waitstate as required by the device datasheet for MCLK
    // operation beyond 8MHz _before_ configuring the clock system.
    FRCTL0 = FRCTLPW | NWAITS_1;

    // Clock System Setup
    CSCTL0_H = CSKEY >> 8;                    // Unlock CS registers
    CSCTL1 = DCOFSEL_4 | DCORSEL;            // Set DCO to 16MHz

    // Set SMCLK = MCLK = DCO, ACLK = LFXCLK = 32KHz crystal
    CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__2 | DIVM__1;     // Set all dividers
    CSCTL0_H = 0;   // Lock CS registers
}

void init_timer() {
    // Configure Channel 0 for up mode with interrupt
    TA0CCR0 = (ONE_SEC / 4) - 1; // Set time
    TA0CCTL0 |= CCIE; // Enable Channel 0 CCIE bit
    TA0CCTL0 &= ~CCIFG; // Clear Channel 0 CCIFG bit

    // Timer_A: ACLK, div by 1, up mode, clear TAR (leaves TAIE=0)
    TA0CTL = TASSEL_1 | ID_0 | MC_1 | TACLR;
}

//*******************************
#pragma vector = TIMER0_A0_VECTOR
__interrupt void T0A0_ISR() {
    // Toggle the LEDs for debugging
    P1OUT ^= redLED;
    TA0CCR0 += ONE_SEC / 4; // Schedule the next interrupt

    // Hardware clears the flag (CCIFG in TA0CCTL0)

    // Enable counter flag after one second
    if(TA0R == ONE_SEC - 1 || TA0R == 2*ONE_SEC - 1)
        flag_counter = 1;

    // Enable footer flag after 250ms
    flag_footer = 1;
}

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;  // Enable the GPIO pins

    config_MCLK_to_16MHz_DCO();

    P1DIR |= redLED; // Direct pin as output
    P9DIR |= greenLED; // Direct pin as output
    P1OUT &= ~redLED; // Turn LED Off
    P9OUT &= ~greenLED; // Turn LED Off

    Graphics_Context g_sContext;
    Graphics_Context counterContext;
    Graphics_Context footerContext;

    Graphics_Rectangle rect;
    Graphics_Rectangle footer_rect;

    int footer_posX = 1;
    int16_t counter = 100;
    char number[4] = "100";

    char text[] = "Safety Helmet System";
    char text2[] = "Count-down timer:";
    char footer[] = "Group 3 UCF Senior Design";

    sprintf(number, "%d", counter);

    config_ACLK_to_32KHz_crystal();

    // Set up the LCD also initialize I2C
    SSD1306_DriverInit();

    // Set up contexts
    Graphics_initContext(&g_sContext, &g_sSSD1306_Driver);
    Graphics_initContext(&counterContext, &g_sSSD1306_Driver);
    Graphics_initContext(&footerContext, &g_sSSD1306_Driver);

    Graphics_setForegroundColor(&g_sContext, ClrWhite);
    Graphics_setForegroundColor(&counterContext, ClrWhite);
    Graphics_setForegroundColor(&footerContext, ClrWhite);

    Graphics_setBackgroundColor(&g_sContext, ClrBlack);
    Graphics_setBackgroundColor(&counterContext, ClrBlack);
    Graphics_setBackgroundColor(&footerContext, ClrBlack);

    Graphics_setFont(&g_sContext, &g_sFontFixed6x8);
    Graphics_setFont(&counterContext, &g_sFontCm16);
    Graphics_setFont(&footerContext, &g_sFontFixed6x8);

    Graphics_clearDisplay(&g_sContext);

    // Draw welcome image
    Graphics_drawImage(&g_sContext,
                       &safetyHelmetWelcome1BPP_UNCOMP,
                       0,
                       0);

    // 2 sec delay on 16MHz clock
    __delay_cycles(8000000);
    __delay_cycles(8000000);

    Graphics_clearDisplay(&g_sContext);

    // Draw title on the screen
    Graphics_drawString(&g_sContext, text, strlen(text), 0, 1, GRAPHICS_TRANSPARENT_TEXT);

    // Draw underline
    Graphics_drawLineH(&g_sContext,
                       0,
                       Graphics_getStringWidth(&g_sContext, text, -1),
                       Graphics_getStringHeight(&g_sContext) + 1);

    // Timer title
    Graphics_drawString(&g_sContext, text2, strlen(text2), 1, 20, GRAPHICS_TRANSPARENT_TEXT);

    // Draw footer
    Graphics_drawString(&g_sContext,
                        footer,
                        strlen(footer),
                        footer_posX,
                        LCD_Y_SIZE - Graphics_getStringHeight(&g_sContext) - 1,
                        GRAPHICS_TRANSPARENT_TEXT);

    // Draw number for the counter
    Graphics_drawString(&counterContext, number, strlen(number), COUNTER_X, COUNTER_Y, GRAPHICS_TRANSPARENT_TEXT);

    //-------------- DO NOT INITIALIZE TIMER BEFORE THIS LINE-------------------//

    init_timer();

    _enable_interrupts();

    // Set deleting rectangle for counter
    rect.xMin = COUNTER_X;
    rect.xMax = COUNTER_X + Graphics_getStringWidth(&counterContext, number, -1);
    rect.yMin = COUNTER_Y - 1;
    rect.yMax = COUNTER_Y + Graphics_getStringHeight(&counterContext);

    // Set deleting rectangle for footer
    footer_rect.xMin = footer_posX;
    footer_rect.xMax = footer_posX + Graphics_getStringWidth(&footerContext, footer, -1);
    footer_rect.yMin = LCD_Y_SIZE - Graphics_getStringHeight(&g_sContext) - 3;
    footer_rect.yMax = LCD_Y_SIZE;


    while(1) {
        if(flag_counter) {
            counter--;

            // Deleting old number
            Graphics_setForegroundColor(&counterContext, ClrBlack);
            Graphics_fillRectangle(&counterContext, &rect);

            // Reset counter once goes below zero
            if(counter < 0)
                counter = 100;

            // Draw new number
            Graphics_setForegroundColor(&counterContext, ClrWhite);
            sprintf(number, "%d", counter);
            Graphics_drawString(&counterContext, number, strlen(number), COUNTER_X, COUNTER_Y, GRAPHICS_TRANSPARENT_TEXT);
            flag_counter = 0;
        }
        if(flag_footer) {
            // Update footer position
            if(footer_posX < (0 - Graphics_getStringWidth(&footerContext, footer, -1)))
                footer_posX = LCD_X_SIZE;

            // Update deleting rectangle position
            footer_rect.xMin = footer_posX;
            footer_rect.xMax = footer_posX + Graphics_getStringWidth(&footerContext, footer, -1);
            footer_posX -= 6;

            // Delete old footer
            Graphics_setForegroundColor(&footerContext, ClrBlack);
            Graphics_fillRectangle(&footerContext, &footer_rect);

            // Draw new footer
            Graphics_setForegroundColor(&footerContext, ClrWhite);
            Graphics_drawString(&g_sContext,
                                footer,
                                strlen(footer),
                                footer_posX,
                                LCD_Y_SIZE - Graphics_getStringHeight(&g_sContext) - 1,
                                GRAPHICS_TRANSPARENT_TEXT);

            flag_footer = 0;
        }
    }
}

