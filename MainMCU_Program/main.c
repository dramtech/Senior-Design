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
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "grlib/grlib.h"
#include "LCD_driver/SSD1306_Driver.h"
#include "images/images.h"
#include "clocks/clocks.h"
#include "modules/modules.h"


#define redLED BIT7 // Red LED at P1.6
#define greenLED BIT6 // Green LED at P1.7
#define ONE_SEC 32768
#define ON 0x01
#define OFF 0x00

// Car warning position
#define RIGHT_CAR_POS_xMIN 107
#define LEFT_CAR_POS_xMIN 0

// Car detection distance
#define DETECT_DIST 30

// Road image position
#define ROAD_POS_xMIN 31
#define ROAD_POS_yMIN 9

// Temperature position
#define TEMP_TEXT_POS_yMIN (LCD_Y_SIZE - 10)

uint8_t togg = 0;
uint8_t flag_L = 0;
uint8_t flag_R = 0;
uint8_t measure_dis = 0;
uint8_t temp_unit_flag = 0;
uint8_t temp_flag = 20;
uint8_t angle_flag = 0;
uint8_t bluetooth_conn_flag = 0;
uint8_t angle_threshold_flag = 0;
uint8_t threshold_counter = 0;

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
    TA0CCR0 += 16384; // Schedule the next interrupt

    togg ^= BIT0;
    // Hardware clears the flag (CCIFG in TA0CCTL0)

    temp_unit_flag = 1;

    if(togg == 0) {
        flag_L = 1;
        flag_R = 1;
    }

    if(togg == 0x01) {
        flag_L = 2;
        flag_R = 2;
        temp_flag++;
        angle_flag++;
        measure_dis = 1;
        temp_unit_flag = 1;
    }

    if (angle_threshold_flag) {
        threshold_counter++;
    }
}

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;  // Enable the GPIO pins

    // Set pins for LEDs
    P1DIR |= redLED | greenLED; // Direct pin as output
//    P9DIR |= greenLED; // Direct pin as output
    P1OUT &= ~(redLED | greenLED); // Turn LED Off
//    P9OUT &= ~greenLED; // Turn LED Off

    // Set wear detection button
    P1DIR &= ~BIT1;         // Set pin 1.1 as input
    P1REN |= BIT1;          // Enable pull up resistor
    P1OUT |= BIT1;          // Set pull up resistor


    Graphics_Context g_sContext;

    uint8_t procedure[4] = {OFF,    // Bluetooth
                            OFF,   // Ultrasonic
                            OFF,   // Brightness
                            OFF};   //Gyro/Acc

    unsigned int distance_cm[2];
    distance_cm[0] = 0;
    distance_cm[1] = 0;
    int temp = 0;
    float angle = 0;
    char distance_str[10];
    char temp_str[7] = "0";
    char angle_str[5] = "90.0";
    unsigned char c = 'A';
    int bluetooth_pair_flag = 0;
    const char *signal = "sSS"; // emergency signal value.
    char tempUnit = 'C';
    unsigned char initialSetting = '\0';

    unsigned char value = '\0';
    char unit = '0';

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

    // Drae road
    Graphics_drawImage(&g_sContext,
                       &road1BPP_COMP_RLE4,
                       ROAD_POS_xMIN,
                       ROAD_POS_yMIN);

    Graphics_drawStringCentered(&g_sContext,
                                "BT not connected",
                                strlen("BT not connected"),
                                (LCD_X_SIZE / 2) - 1,
                                (Graphics_getStringHeight(&g_sContext) / 2) + 1,
                                GRAPHICS_TRANSPARENT_TEXT);


    // Draw right car warning image
//    Graphics_drawImage(&g_sContext,
//                       &rightCarIndicator1BPP_COMP_RLE4,
//                       RIGHT_CAR_POS_xMIN,
//                       22);

    // Text for debugging
//    Graphics_drawString(&g_sContext, "Temperature: ", strlen("Temperature: "), 0, TEMP_TEXT_POS_yMIN, GRAPHICS_TRANSPARENT_TEXT);
//    Graphics_drawString(&g_sContext, "Angle: ", strlen("Angle: "), 0, TEMP_TEXT_POS_yMIN - 15, GRAPHICS_TRANSPARENT_TEXT);

    Graphics_Rectangle rect_R, rect_L, rect2, temp_del, angle_rect, title_rect;

    // Title deleting square
    title_rect.xMin = 0;
    title_rect.xMax = LCD_X_SIZE - 1;
    title_rect.yMin = 0;
    title_rect.yMax = Graphics_getStringHeight(&g_sContext) + 1;

    // RIGHT car deleting square
    rect_R.xMin = RIGHT_CAR_POS_xMIN;
    rect_R.xMax = LCD_X_SIZE;
    rect_R.yMin = LCD_Y_SIZE / 2 - 10;
    rect_R.yMax = LCD_Y_SIZE / 2 + 10;

    // LEFT car deleting square
    rect_L.xMin = LEFT_CAR_POS_xMIN;
    rect_L.xMax = LEFT_CAR_POS_xMIN + 20;
    rect_L.yMin = LCD_Y_SIZE / 2 - 10;
    rect_L.yMax = LCD_Y_SIZE / 2 + 10;

    // Car warning deleting square (debugging)
    rect2.xMin = 0;
    rect2.xMax = Graphics_getStringWidth(&g_sContext, distance_str, -1);
    rect2.yMin = 0;
    rect2.yMax = Graphics_getStringHeight(&g_sContext);

    // Temperature deleting square (debugging)
//    temp_del.xMin = Graphics_getStringWidth(&g_sContext, "Temperature: ", -1);
    temp_del.xMin = 0;
//    temp_del.xMax = Graphics_getStringWidth(&g_sContext, "Temperature: ", -1)
//                    + Graphics_getStringWidth(&g_sContext, temp_str, -1);
    temp_del.xMax = Graphics_getStringWidth(&g_sContext, temp_str, -1);
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

//    _disable_interrupts();
    _enable_interrupts();
    while(1) {
        if((P1IN & BIT1) == BIT1) {
            procedure[1] = OFF;
            procedure[3] = OFF;
            P1OUT &= ~greenLED;
        } else {
            procedure[1] = ON;
            procedure[3] = ON;
            P1OUT |= greenLED;
            if(temp_flag >= 20)
                temp_flag = 19;

        }

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
//            SetBrightness();
            if (!bluetooth_pair_flag) {

                initialSetting = receive_data();

                if (initialSetting == c) {
                    procedure[1] = ON; // Ultrasonic
                    procedure[3] = ON; //Gyro/Acc

                    bluetooth_pair_flag = 1; // Set bluetooth pair ON.
                    temp_flag = 15; // Set temp_flag to output temperature value immediately after bluetooth is paired.
                    angle_flag = 0;

                    __delay_cycles(1000000);
                    initialSetting = receive_data();

                    // Set brightness
                    SetBrightness(initialSetting);

                    // Set temperature unites
                    __delay_cycles(1000000);
                    tempUnit = receive_data();

                    _enable_interrupts();

                    // Update title
                    Graphics_setForegroundColor(&g_sContext, ClrBlack);
                    Graphics_fillRectangle(&g_sContext, &title_rect);
                    Graphics_setForegroundColor(&g_sContext, ClrWhite);

                    Graphics_drawStringCentered(&g_sContext,
                                                "Connected",
                                                strlen("Connected"),
                                                (LCD_X_SIZE / 2) - 1,
                                                (Graphics_getStringHeight(&g_sContext) / 2) + 1,
                                                GRAPHICS_TRANSPARENT_TEXT);
                }
            }

            if (temp_unit_flag) {
              temp_unit_flag = 0;
              value = receive_data();
              unit = (char) value;

              if (unit == 'C' || unit == 'F') {
                  tempUnit = unit;
                  temp_flag = 19;
              } else if (value){
                  SetBrightness(value);
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
            if(distance_cm[0] < DETECT_DIST) {
                if(flag_R == 1) {
                    flag_R = 0;
                    Graphics_setForegroundColor(&g_sContext, ClrWhite);
                    Graphics_drawImage(&g_sContext,
                                       &rightCarIndicator1BPP_COMP_RLE4,
                                       RIGHT_CAR_POS_xMIN,
                                       LCD_Y_SIZE / 2 - 10);
                }
                if(flag_R == 2) {
                    flag_R = 0;
                    Graphics_setForegroundColor(&g_sContext, ClrBlack);
                    Graphics_fillRectangle(&g_sContext, &rect_R);
                }
            } else {
                Graphics_setForegroundColor(&g_sContext, ClrBlack);
                Graphics_fillRectangle(&g_sContext, &rect_R);
            }

            // Left car warning
            if(distance_cm[1] < DETECT_DIST) {
                if(flag_L == 1) {
                    flag_L = 0;
                    Graphics_setForegroundColor(&g_sContext, ClrWhite);
                    Graphics_drawImage(&g_sContext,
                                       &leftCarIndicator1BPP_COMP_RLE4,
                                       LEFT_CAR_POS_xMIN,
                                       LCD_Y_SIZE / 2 - 10);
                }
                if(flag_L == 2) {
                    flag_L = 0;
                    Graphics_setForegroundColor(&g_sContext, ClrBlack);
                    Graphics_fillRectangle(&g_sContext, &rect_L);
                }
            } else {
                Graphics_setForegroundColor(&g_sContext, ClrBlack);
                Graphics_fillRectangle(&g_sContext, &rect_L);
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

                // Temp conversion.
                if (tempUnit == 'F') {
                    temp = (int)round(temp * 1.8 + 32.0);
                }

                sprintf(temp_str, "t%d", temp);

                transmit_data(temp_str); // Send temp data through bluetooth with temp header.

                sprintf(temp_str, "%s %c", temp_str, tempUnit);

                Graphics_setForegroundColor(&g_sContext, ClrBlack);
                Graphics_fillRectangle(&g_sContext, &temp_del);
                Graphics_setForegroundColor(&g_sContext, ClrWhite);
//                temp_del.xMax = Graphics_getStringWidth(&g_sContext, "Temperature: ", -1)
//                                + Graphics_getStringWidth(&g_sContext, temp_str + 1, -1);

                temp_del.xMax = Graphics_getStringWidth(&g_sContext, temp_str + 1, -1);

                Graphics_drawString(&g_sContext,
                                    temp_str + 1, strlen(temp_str),
                                    0,
                                    TEMP_TEXT_POS_yMIN,
                                    GRAPHICS_TRANSPARENT_TEXT);
            }
            // Retrieve angle of inclination (every 1 seconds)
            // (angle between 0 to 90 degrees)
            if(angle_flag == 2) {
                angle_flag = 0;
                getAngle(&angle);

                float threshold = 60.0;

                if (angle > threshold) {
                    angle_threshold_flag = 1;
                } else {
                    angle_threshold_flag = 0;
                    threshold_counter = 0;
                }

                if (threshold_counter > 9) {
                    threshold_counter = 0;
                    angle_threshold_flag = 0;

                    transmit_data(signal);
                }

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
