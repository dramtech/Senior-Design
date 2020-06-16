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
#include <string.h>

#include "grlib/grlib.h"
#include "LCD_driver/SSD1306_Driver.h"
#include "images/images.h"
#include "clocks/clocks.h"
#include "modules/modules.h"


#define redLED BIT0 // Red LED at P1.0
#define greenLED BIT7 // Green LED at P9.7
#define ONE_SEC 32768
#define ON 0x01
#define OFF 0x00

// Car warning position
#define RIGHT_CAR_POS_xMIN 107

// Temperature position
#define TEMP_TEXT_POS_yMIN (LCD_Y_SIZE - 10)

uint8_t togg = 0;
uint8_t flag = 0;
uint8_t measure_dis = 0;
uint8_t temp_flag = 20;
uint8_t angle_flag = 0;
uint8_t bluetooth_conn_flag = 0;

#if defined(__IAR_SYSTEMS_ICC__)
int16_t __low_level_init(void) {
    // Stop WDT (Watch Dog Timer)
    WDTCTL = WDTPW + WDTHOLD;
    return(1);
}

#endif

void init_timer() {
    // Configure Channel 0 for up mode with interrupt
    TA0CCR0 = (ONE_SEC / 2) - 1; // Set time 0.5 sec
    TA0CCTL0 |= CCIE; // Enable Channel 0 CCIE bit
    TA0CCTL0 &= ~CCIFG; // Clear Channel 0 CCIFG bit

    // Timer_A: ACLK, div by 1, up mode, clear TAR (leaves TAIE=0)
    TA0CTL = TASSEL_1 | ID_0 | MC_1 | TACLR;
}

// Converting a float to a string 2 decimal places
// (Only use for debugging)
void floatToString(float input, char * output) {
    input = input * 100.0;
    int dec1 = (int)(input) % 10;
    input /= 10.0;
    int dec2 = (int)(input) % 10;
    input /= 10.0;

    sprintf(output, "%d.%d%d",(int)input, dec1, dec2);
}

//*******************************
#pragma vector = TIMER0_A0_VECTOR
__interrupt void T0A0_ISR() {
    // Toggle the LEDs for debugging
    P1OUT ^= redLED;
    TA0CCR0 += ONE_SEC / 2; // Schedule the next interrupt

    togg ^= BIT0;
    // Hardware clears the flag (CCIFG in TA0CCTL0)

    if(togg == 0)
        flag = 1;

    if(togg == 0x01) {
        flag = 2;
        temp_flag++;
        angle_flag++;
        measure_dis = 1;
    }

}

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;  // Enable the GPIO pins

    // Set pins for LEDs
    P1DIR |= redLED; // Direct pin as output
    P9DIR |= greenLED; // Direct pin as output
    P1OUT &= ~redLED; // Turn LED Off
    P9OUT &= ~greenLED; // Turn LED Off

    Graphics_Context g_sContext;

    uint8_t procedure[4] = {ON,    // Bluetooth
                            OFF,   // Ultrasonic
                            OFF,   // Brightness
                            OFF};   //Gyro/Acc

    unsigned int distance_cm[2];
    distance_cm[0] = 0;
    distance_cm[1] = 0;
    int temp = 0;
    float angle = 0;
    char distance_str[10];
    char temp_str[4] = "0";
    char angle_str[5] = "90.0";
    unsigned char c = 'A';
    int bluetooth_pair_flag = 0;


    // Set up the LCD also initialize I2C
    SSD1306_DriverInit();

    config_ACLK_to_32KHz_crystal();

    us_init();
    Initialize_Gyro();

    // Set MCLK = SMCLK = 16MHz, ACLK = 32KHz crystal
    // Set dividers
    config_MCLK_to_16MHz_DCO();

    // Reading initial temperature value (initial value is wrong)
    getTemp(&temp);

    // Set up context
    Graphics_initContext(&g_sContext, &g_sSSD1306_Driver);
    Graphics_setForegroundColor(&g_sContext, ClrWhite);
    Graphics_setBackgroundColor(&g_sContext, ClrBlack);
    Graphics_setFont(&g_sContext, &g_sFontFixed6x8);

    Graphics_clearDisplay(&g_sContext);

    // Draw welcome image
    Graphics_drawImage(&g_sContext,
                       &safetyHelmetWelcome1BPP_UNCOMP,
                       0,
                       0);

    // 1 sec delay on 16MHz clock
    __delay_cycles(8000000);
    __delay_cycles(8000000);

    Graphics_clearDisplay(&g_sContext);

    // Draw right car warning image
    Graphics_drawImage(&g_sContext,
                       &rightCarIndicator1BPP_COMP_RLE4,
                       RIGHT_CAR_POS_xMIN,
                       22);

    // Text for debugging
    Graphics_drawString(&g_sContext, "Temperature: ", strlen("Temperature: "), 0, TEMP_TEXT_POS_yMIN, GRAPHICS_TRANSPARENT_TEXT);
    Graphics_drawString(&g_sContext, "Angle: ", strlen("Angle: "), 0, TEMP_TEXT_POS_yMIN - 15, GRAPHICS_TRANSPARENT_TEXT);

    Graphics_Rectangle rect, rect2, temp_del, angle_rect;

    // Distance deleting square
    rect.xMin = RIGHT_CAR_POS_xMIN;
    rect.xMax = LCD_X_SIZE;
    rect.yMin = LCD_Y_SIZE / 2 - 10;
    rect.yMax = LCD_Y_SIZE / 2 + 10;

    // Car warning deleting square
    rect2.xMin = 0;
    rect2.xMax = Graphics_getStringWidth(&g_sContext, distance_str, -1);
    rect2.yMin = 0;
    rect2.yMax = Graphics_getStringHeight(&g_sContext);

    // Temperature deleting square
    temp_del.xMin = Graphics_getStringWidth(&g_sContext, "Temperature: ", -1);
    temp_del.xMax = Graphics_getStringWidth(&g_sContext, "Temperature: ", -1)
                    + Graphics_getStringWidth(&g_sContext, temp_str, -1);
    temp_del.yMin = TEMP_TEXT_POS_yMIN - 1;
    temp_del.yMax = TEMP_TEXT_POS_yMIN + Graphics_getStringHeight(&g_sContext);

    // Angle deleting square (debugging)
    angle_rect.xMin = Graphics_getStringWidth(&g_sContext, "Angle: ", -1);
    angle_rect.xMax = Graphics_getStringWidth(&g_sContext, "Angle: ", -1)
                      + Graphics_getStringWidth(&g_sContext, angle_str, -1);
    angle_rect.yMin = TEMP_TEXT_POS_yMIN - 16;
    angle_rect.yMax = TEMP_TEXT_POS_yMIN - 15 + Graphics_getStringHeight(&g_sContext);

    init_timer();
    init_bluetooth();

    _enable_interrupts();

    while(1) {
        // TODO Bluetooth procedure
        if(procedure[0]) {
            // TODO implement all Bluetooth interaction inside this if statement
            // Bluetooth should be ON upon start-up all other procedures should be off.
            // After Bluetooth is paired turn on all other procedures and update
            // variables to grab needed information periodically set appropriate delay
            // with timer A.

            // Functions and constants for Bluetooth should be implemented in a separate
            // source file use modules.h header for extern functions.

            // Temperature value is stored in temp variable (signed int)
            // as celsius unites

            if (!bluetooth_pair_flag) {
                unsigned char retval = '0';
                retval = receive_data();

                if (retval == c) {
                    procedure[1] = ON; // Ultrasonic
                    procedure[3] = ON; //Gyro/Acc

                    bluetooth_pair_flag = 1; // Set bluetooth pair ON.
                }
            }
        }
        // Ultrasonic procedure
        if(procedure[1]) {
            if(measure_dis) {
                measure_dis = 0;
                getDistance(distance_cm);
                rect2.xMax = Graphics_getStringWidth(&g_sContext, distance_str, -1);
                sprintf(distance_str, "%d  %d", distance_cm[0], distance_cm[1]);
                Graphics_setForegroundColor(&g_sContext, ClrBlack);
                Graphics_fillRectangle(&g_sContext, &rect2);
                Graphics_setForegroundColor(&g_sContext, ClrWhite);
                Graphics_drawString(&g_sContext, distance_str, strlen(distance_str), 0, 1, GRAPHICS_TRANSPARENT_TEXT);
            }

            // Right car warning
            if(distance_cm[0] < 220) {
                if(flag == 1) {
                    flag = 0;
                    Graphics_setForegroundColor(&g_sContext, ClrWhite);
                    Graphics_drawImage(&g_sContext,
                                       &rightCarIndicator1BPP_COMP_RLE4,
                                       RIGHT_CAR_POS_xMIN,
                                       LCD_Y_SIZE / 2 - 10);
                }
                if(flag == 2) {
                    flag = 0;
                    Graphics_setForegroundColor(&g_sContext, ClrBlack);
                    Graphics_fillRectangle(&g_sContext, &rect);
                }
            }else {
                Graphics_setForegroundColor(&g_sContext, ClrBlack);
                Graphics_fillRectangle(&g_sContext, &rect);
            }

            // TODO left car warning - need second US sensor

        }
        // TODO Brightness control procedure
        if(procedure[2]) {
            // When set brightness value set procedure to OFF
        }
        // TODO Gyro/Accelerometer procedure
        if(procedure[3]) {
            // Retrieve temperature data (every 10 seconds)
            if(temp_flag == 20) {
                temp_flag = 0;
                getTemp(&temp);

                sprintf(temp_str, "%d", temp);

                transmit_data(temp_str); // Send temp data through bluetooth.

                Graphics_setForegroundColor(&g_sContext, ClrBlack);
                Graphics_fillRectangle(&g_sContext, &temp_del);
                Graphics_setForegroundColor(&g_sContext, ClrWhite);
                temp_del.xMax = Graphics_getStringWidth(&g_sContext, "Temperature: ", -1)
                                + Graphics_getStringWidth(&g_sContext, temp_str, -1);

                Graphics_drawString(&g_sContext,
                                    temp_str, strlen(temp_str),
                                    Graphics_getStringWidth(&g_sContext, "Temperature: ", -1),
                                    TEMP_TEXT_POS_yMIN,
                                    GRAPHICS_TRANSPARENT_TEXT);
            }
            // Retrieve angle of inclination (every 1 seconds)
            // (angle between 0 to 90 degrees)
            if(angle_flag == 2) {
                angle_flag = 0;
                getAngle(&angle);
//                floatToString(angle, angle_str);
//
//                // Print to the screen for debugging
//                Graphics_setForegroundColor(&g_sContext, ClrBlack);
//                Graphics_fillRectangle(&g_sContext, &angle_rect);
//                Graphics_setForegroundColor(&g_sContext, ClrWhite);
//                angle_rect.xMax = Graphics_getStringWidth(&g_sContext, "Angle: ", -1)
//                                  + Graphics_getStringWidth(&g_sContext, angle_str, -1);
//
//                Graphics_drawString(&g_sContext,
//                                    angle_str, strlen(angle_str),
//                                    Graphics_getStringWidth(&g_sContext, "Angle: ", -1),
//                                    TEMP_TEXT_POS_yMIN - 15,
//                                    GRAPHICS_TRANSPARENT_TEXT);

            }
        }
    }
	
	return 0;
}
