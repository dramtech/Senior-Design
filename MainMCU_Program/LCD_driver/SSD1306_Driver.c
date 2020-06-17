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
//*****************************************************************************
//
// Template_Driver.c - Display driver for any LCD Controller. This file serves as
//						a template for creating new LCD driver files
//
//*****************************************************************************
//
//! \addtogroup display_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// READ ME
//
// This template driver is intended to be modified for creating new LCD drivers
// It is setup so that only Template_DriverPixelDraw() and DPYCOLORTRANSLATE()
// and some LCD size configuration settings in the header file Template_Driver.h
// are REQUIRED to be written. These functions are marked with the string
// "TemplateDisplayFix" in the comments so that a search through Template_Driver.c and
// Template_Driver.h can quickly identify the necessary areas of change.
//
// Template_DriverPixelDraw() is the base function to write to the LCD
// display. Functions like WriteData(), WriteCommand(), and SetAddress()
// are suggested to be used to help implement the Template_DriverPixelDraw()
// function, but are not required. SetAddress() should be used by other pixel
// level functions to help optimize them.
//
// This is not an optimized driver however and will significantly impact
// performance. It is highly recommended to first get the prototypes working
// with the single pixel writes, and then go back and optimize the driver.
// Please see application note www.ti.com/lit/pdf/slaa548 for more information
// on how to fully optimize LCD driver files. In short, driver optimizations
// should take advantage of the auto-incrementing of the LCD controller.
// This should be utilized so that a loop of WriteData() can be used instead
// of a loop of Template_DriverPixelDraw(). The pixel draw loop contains both a
// SetAddress() + WriteData() compared to WriteData() alone. This is a big time
// saver especially for the line draws and Template_DriverPixelDrawMultiple.
// More optimization can be done by reducing function calls by writing macros,
// eliminating unnecessary instructions, and of course taking advantage of other
// features offered by the LCD controller. With so many pixels on an LCD screen
// each instruction can have a large impact on total drawing time.
//
//*****************************************************************************

//*****************************************************************************
//
// Include Files
//
//*****************************************************************************
#include <msp430.h>
#include "grlib/grlib.h"
#include "SSD1306_Driver.h"

//*****************************************************************************
//
// Global Variables
//
//*****************************************************************************
/* Global buffer for the display. This is especially useful on 1BPP, 2BPP, and 4BPP
   displays. This allows you to update pixels while reading the neighboring pixels
   from the buffer instead of a read from the LCD controller. A buffer is not required
   as a read followed by a write can be used instead.*/

// Store buffer in FRAM
#pragma NOINIT(SSD1306_Memory)
uint8_t SSD1306_Memory[(LCD_X_SIZE * LCD_Y_SIZE * BPP + 7) / 8];

//*****************************************************************************
//
// Suggested functions to help facilitate writing the required functions below
//
//*****************************************************************************
// defines for i2c
//#define RXFLAG UCRXIFG // Receive flag
//#define TXFLAG UCTXIFG // Transmit flag
#define OLED_ADDR 0x78 // OLED i2c address

static void Initialize_I2C(void) {
//    _disable_interrupts();

    // Enter reset state before the configuration starts...
    UCB1CTLW0 |= UCSWRST;

    // Pins
    // SCL -> P4.1
    // SDA -> P4.0

    // Divert pins to I2C functionality
    P4SEL1 |= (BIT1|BIT0);
    P4SEL0 &= ~(BIT1|BIT0);

    // Keep all the default values except the fields below...
    // (UCMode 3:I2C) (Master Mode) (UCSSEL 1:ACLK, 2,3:SMCLK)
    UCB1CTLW0 |= UCMODE_3 | UCMST | UCSSEL_3;

    // Clock divider = 8 (SMCLK @ 4MHz / 8 = 500KHz)
    UCB1BRW = 8;

    // Exit the reset mode
    UCB1CTLW0 &= ~UCSWRST;

//    _enable_interrupts();
}

int i2c_write_byte(unsigned char i2c_address, unsigned char * data) {
//    _disable_interrupts();

    UCB1I2CSA = i2c_address >> 1; // Set I2C address

    unsigned char ctl_byte = data[0];
    unsigned char dat = data[1];

    UCB1IFG &= ~UCTXIFG0; // Clear ACK flag
    UCB1CTLW0 |= UCTR; // Master writes (R/W bit = Write)
    UCB1CTLW0 |= UCTXSTT; // Initiate the Start Signal

    while ((UCB1IFG & UCTXIFG0) ==0) {} // Wait for ACK

    UCB1TXBUF = ctl_byte; // Byte = register address

    while((UCB1CTLW0 & UCTXSTT)!=0) {}
    while ( (UCB1IFG & UCTXIFG0) == 0) {} // Wait for ACK

    //********** Write Byte #1 ***************************
    UCB1TXBUF = dat;
    while ( (UCB1IFG & UCTXIFG0) == 0) {} // Wait for ACK

    UCB1CTLW0 |= UCTXSTP; // Set stop bit
    while ( (UCB1CTLW0 & UCTXSTP) != 0) {}

//    _enable_interrupts();

    return 0;
}

// Writing commands only
int i2c_write_array(unsigned char i2c_address, unsigned char * data, unsigned int dataSize) {
//    _disable_interrupts();
    int i = 0;

    unsigned char data_pocket[2];
    data_pocket[0] = 0x80; // set C0 = 1 (set control byte ON), D/C = 0 (set next byte as COMMAND)
    data_pocket[1] = data[i];

    UCB1I2CSA = i2c_address >> 1; // Set I2C address

    UCB1IFG &= ~UCTXIFG0;
    UCB1CTLW0 |= UCTR; // Master writes (R/W bit = Write)
    UCB1CTLW0 |= UCTXSTT; // Initiate the Start Signal

    while ((UCB1IFG & UCTXIFG0) ==0) {} // Wait for ACK


    UCB1TXBUF = data_pocket[0]; // Transmit control byte

    while((UCB1CTLW0 & UCTXSTT)!=0) {}

    while ( (UCB1IFG & UCTXIFG0) == 0) {} // Wait for ACK

    // Transmit COMMAND byte
    UCB1TXBUF = data_pocket[1];
    i++;
    dataSize--;

//    if(( UCB1IFG & UCNACKIFG )!= 0) P9OUT |= BIT7;

    while ( (UCB1IFG & UCTXIFG0) == 0) {} // Wait for ACK

    // Transmitting array of data
    while(dataSize) {
        data_pocket[1] = data[i];
        UCB1TXBUF = data_pocket[0];

        while ( (UCB1IFG & UCTXIFG0) == 0) {} // Wait for ACK

        UCB1TXBUF = data_pocket[1];

        while ( (UCB1IFG & UCTXIFG0) == 0) {} // Wait for ACK
        i++;
        dataSize--;
    }

    UCB1CTLW0 |= UCTXSTP; // Set stop signal
    while ( (UCB1CTLW0 & UCTXSTP) != 0) {}

//    _enable_interrupts();

    return 0;
}

// Writes data to the LCD controller
static void
WriteData(unsigned char usData)
{
    unsigned char data_pocket[2];

    // set C0 = 1 (set control byte ON), D/C = 0 (set next byte as DATA)
    data_pocket[0] = 0xC0;
    data_pocket[1] = usData;

    i2c_write_byte(OLED_ADDR, data_pocket);
}

// Writes a command to the LCD controller
static void
WriteCommand(unsigned char * ucCommand, unsigned int size)
{
    // Commands transmitted with an array is more efficient
    i2c_write_array(OLED_ADDR, ucCommand, size);
}

// Sets the pixel address of the LCD driver
void SetAddress(int16_t lX,
                int16_t endX,
                int16_t lY)
{
    if(lX > LCD_X_SIZE )
        lX = LCD_X_SIZE;

    if(lY > LCD_Y_SIZE )
        lY = LCD_Y_SIZE;

    // Set X coordinates
    unsigned char data_pocketX[3];
    data_pocketX[0] = 0x21; // Set register address for Column Address
    data_pocketX[1] = 0xFF & lX; // Set X coordinate as start address
    data_pocketX[2] = 0xFF & endX; // Set X coordinate end address

    // Set Y coordinates
    unsigned char data_pocketY[3];
    data_pocketY[0] = 0x22; // Set register address for Page Address
    data_pocketY[1] = lY / 8; // Set page start address
    data_pocketY[2] = 0x07; // Set page end address

    // Transmitting the data
    WriteCommand(data_pocketX, 3);
    WriteCommand(data_pocketY, 3);

}

static void
InitGPIOLCDInterface(void)
{
    Initialize_I2C();
}

// Initialize DisplayBuffer.
// This function initializes the display buffer and discards any cached data.
static void
InitLCDDisplayBuffer(uint32_t ulValue)
{
    uint16_t i = 0,j = 0;
    for(i = 0; i < LCD_Y_SIZE / 8; i++)
    {
        for(j = 0; j < (LCD_X_SIZE * BPP); j++)
        {
            SSD1306_Memory[i * LCD_X_SIZE + j] = (uint8_t)ulValue;
        }
    }
}

// Initializes the display driver.
// This function initializes the LCD controller
void
SSD1306_DriverInit(void)
{
    uint32_t backGroudValue = 0x0;

    InitGPIOLCDInterface();

    InitLCDDisplayBuffer(backGroudValue);

    const unsigned char init_cmd_array[]={0xAE,          // DISPLAY OFF
                                         0xD5,          // SET OSC FREQUENY
                                         0xF0,          // divide ratio = 1 (bit 3-0), OSC (bit 7-4)
                                         0xA8,          // SET MUX RATIO
                                         0x3F,          // 64MUX
                                         0xD3,          // SET DISPLAY OFFSET
                                         0x00,          // offset = 0
                                         0x40,          // set display start line, start line = 0
                                         0x8D,          // ENABLE CHARGE PUMP REGULATOR
                                         0x14,          //
                                         0x20,          // SET MEMORY ADDRESSING MODE
                                         0x00,          // horizontal addressing mode
                                         0xA0,          // set segment re-map, column address 0 is mapped to SEG0
                                         0xC0,          // set COM/Output scan direction, normal mode (COM0 to COM[N –1])
                                         0xDA,          // SET COM PINS HARDWARE CONFIGURATION
                                         0x12,          // alternative COM pin configuration /////////////// MIGHT NEED TO BE CHANGED /////////////
                                         0x81,          // SET CONTRAST CONTROL
                                         0xCF,          //
                                         0xD9,          // SET PRE CHARGE PERIOD
                                         0xF1,          //
                                         0xDB,          // SET V_COMH DESELECT LEVEL
                                         0x40,          //
                                         0xA4,          // DISABLE ENTIRE DISPLAY ON
                                         0xA6,          // NORMAL MODE (A7 for inverse display)
                                         0xAF};         // DISPLAY ON

    i2c_write_array(OLED_ADDR, init_cmd_array, 25);
}

//*****************************************************************************
//
// All the following functions (below) for the LCD driver are required by grlib
//
//*****************************************************************************

//*****************************************************************************
//
//! Draws a pixel on the screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX is the X coordinate of the pixel.
//! \param lY is the Y coordinate of the pixel.
//! \param ulValue is the color of the pixel.
//!
//! This function sets the given pixel to a particular color.  The coordinates
//! of the pixel are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void
SSD1306_DriverPixelDraw(void *pvDisplayData,
                         int16_t lX,
                         int16_t lY,
                         uint16_t ulValue)
{
    if (lY > LCD_Y_SIZE)
        lY = LCD_Y_SIZE;

    if(lX > LCD_X_SIZE)
        lX = LCD_X_SIZE;

    uint8_t pixel_byte;

    // Converting X and Y coordinates in memory buffer
    pixel_byte = 0x01 << (lY % 8);

    unsigned int index = ((lY / 8 ) * LCD_X_SIZE ) + (lX * BPP);

    // Display has only one color value
    if(ulValue == 0)
        SSD1306_Memory[index] &= ~pixel_byte;
    else
        SSD1306_Memory[index] |= pixel_byte;

    pixel_byte = SSD1306_Memory[index];

    SetAddress(MAPPED_X(lX, lY), MAPPED_X(LCD_X_SIZE - 1, lY), MAPPED_Y(lX, lY));

    WriteData(pixel_byte);
}

//*****************************************************************************
//
//! Draws a horizontal sequence of pixels on the screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX is the X coordinate of the first pixel.
//! \param lY is the Y coordinate of the first pixel.
//! \param lX0 is sub-pixel offset within the pixel data, which is valid for 1
//! or 4 bit per pixel formats.
//! \param lCount is the number of pixels to draw.
//! \param lBPP is the number of bits per pixel; must be 1, 4, or 8.
//! \param pucData is a pointer to the pixel data.  For 1 and 4 bit per pixel
//! formats, the most significant bit(s) represent the left-most pixel.
//! \param pucPalette is a pointer to the palette used to draw the pixels.
//!
//! This function draws a horizontal sequence of pixels on the screen, using
//! the supplied palette.  For 1 bit per pixel format, the palette contains
//! pre-translated colors; for 4 and 8 bit per pixel formats, the palette
//! contains 24-bit RGB values that must be translated before being written to
//! the display.
//!
//! \return None.
//
//*****************************************************************************
static void
SSD1306_DriverPixelDrawMultiple(void *pvDisplayData,
                                 int16_t lX,
                                 int16_t lY,
                                 int16_t lX0,
                                 int16_t lCount,
                                 int16_t lBPP,
                                 const uint8_t *pucData,
                                 const uint32_t *pucPalette)
{
    uint16_t ulByte;

    //
    // Determine how to interpret the pixel data based on the number of bits
    // per pixel.
    //
    switch(lBPP)
    {
    // The pixel data is in 1 bit per pixel format
    case 1:
    {
        // Loop while there are more pixels to draw
        while(lCount > 0)
        {
            // Get the next byte of image data
            ulByte = *pucData++;

            // Loop through the pixels in this byte of image data
            for(; (lX0 < 8) && lCount; lX0++, lCount--)
            {
                // Draw this pixel in the appropriate color
                SSD1306_DriverPixelDraw(pvDisplayData, lX++, lY,
                                         ((uint16_t *)pucPalette)[(ulByte >>
                                                                   (7 -
                                                                    lX0)) & 1]);
            }

            // Start at the beginning of the next byte of image data
            lX0 = 0;
        }
        // The image data has been drawn

        break;
    }

    // The pixel data is in 2 bit per pixel format
    case 2:
    {
        // Loop while there are more pixels to draw
        while(lCount > 0)
        {
            // Get the next byte of image data
            ulByte = *pucData++;

            // Loop through the pixels in this byte of image data
            for(; (lX0 < 4) && lCount; lX0++, lCount--)
            {
                // Draw this pixel in the appropriate color
                SSD1306_DriverPixelDraw(pvDisplayData, lX++, lY,
                                         ((uint16_t *)pucPalette)[(ulByte >>
                                                                   (6 -
                                                                    (lX0 <<
                    1))) & 3]);
            }

            // Start at the beginning of the next byte of image data
            lX0 = 0;
        }
        // The image data has been drawn

        break;
    }
    // The pixel data is in 4 bit per pixel format
    case 4:
    {
        // Loop while there are more pixels to draw.  "Duff's device" is
        // used to jump into the middle of the loop if the first nibble of
        // the pixel data should not be used.  Duff's device makes use of
        // the fact that a case statement is legal anywhere within a
        // sub-block of a switch statement.  See
        // http://en.wikipedia.org/wiki/Duff's_device for detailed
        // information about Duff's device.
        switch(lX0 & 1)
        {
        case 0:

            while(lCount)
            {
                // Get the upper nibble of the next byte of pixel data
                // and extract the corresponding entry from the palette
                ulByte = (*pucData >> 4);
                ulByte = (*(uint16_t *)(pucPalette + ulByte));
                // Write to LCD screen
                SSD1306_DriverPixelDraw(pvDisplayData, lX++, lY, ulByte);

                // Decrement the count of pixels to draw
                lCount--;

                // See if there is another pixel to draw
                if(lCount)
                {
                case 1:
                    // Get the lower nibble of the next byte of pixel
                    // data and extract the corresponding entry from
                    // the palette
                    ulByte = (*pucData++ & 15);
                    ulByte = (*(uint16_t *)(pucPalette + ulByte));
                    // Write to LCD screen
                    SSD1306_DriverPixelDraw(pvDisplayData, lX++, lY, ulByte);

                    // Decrement the count of pixels to draw
                    lCount--;
                }
            }
        }
        // The image data has been drawn.

        break;
    }

    // The pixel data is in 8 bit per pixel format
    case 8:
    {
        // Loop while there are more pixels to draw
        while(lCount--)
        {
            // Get the next byte of pixel data and extract the
            // corresponding entry from the palette
            ulByte = *pucData++;
            ulByte = (*(uint16_t *)(pucPalette + ulByte));
            // Write to LCD screen
            SSD1306_DriverPixelDraw(pvDisplayData, lX++, lY, ulByte);
        }
        // The image data has been drawn
        break;
    }
    }
}

//*****************************************************************************
//
//! Draws a horizontal line.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX1 is the X coordinate of the start of the line.
//! \param lX2 is the X coordinate of the end of the line.
//! \param lY is the Y coordinate of the line.
//! \param ulValue is the color of the line.
//!
//! This function draws a horizontal line on the display.  The coordinates of
//! the line are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void
SSD1306_DriverLineDrawH(void *pvDisplayData,
                         int16_t lX1,
                         int16_t lX2,
                         int16_t lY,
                         uint16_t ulValue)
{
    uint8_t pixel_byte, y_bit_offset;
    unsigned int index = 0;

    // Converting Y coordinate in memory buffer

    y_bit_offset = 0x01 << (lY % 8);

    SetAddress(MAPPED_X(lX1, lY), MAPPED_X(lX2, lY), MAPPED_Y(lX1, lY));

    do
    {
//        SSD1306_DriverPixelDraw(pvDisplayData, lX1, lY, ulValue);

        if(lY < 64)
            index = ((lY / 8 ) * LCD_X_SIZE ) + lX1;
        else
            return;

        // Update buffer
        // Display has only one color
        if(ulValue == 0)
            SSD1306_Memory[index] &= ~y_bit_offset;
        else
            SSD1306_Memory[index] |= y_bit_offset;

        pixel_byte = SSD1306_Memory[index];

        WriteData(pixel_byte);
    }
    while(lX1++ < lX2);
}

//*****************************************************************************
//
//! Draws a vertical line.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX is the X coordinate of the line.
//! \param lY1 is the Y coordinate of the start of the line.
//! \param lY2 is the Y coordinate of the end of the line.
//! \param ulValue is the color of the line.
//!
//! This function draws a vertical line on the display.  The coordinates of the
//! line are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void
SSD1306_DriverLineDrawV(void *pvDisplayData,
                         int16_t lX,
                         int16_t lY1,
                         int16_t lY2,
                         uint16_t ulValue)
{
    do
    {
        SSD1306_DriverPixelDraw(pvDisplayData, lX, lY1, ulValue);
    }
    while(lY1++ < lY2);
}

//*****************************************************************************
//
//! Fills a rectangle.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param pRect is a pointer to the structure describing the rectangle.
//! \param ulValue is the color of the rectangle.
//!
//! This function fills a rectangle on the display.  The coordinates of the
//! rectangle are assumed to be within the extents of the display, and the
//! rectangle specification is fully inclusive (in other words, both sXMin and
//! sXMax are drawn, along with sYMin and sYMax).
//!
//! \return None.
//
//*****************************************************************************
static void
SSD1306_DriverRectFill(void *pvDisplayData,
                        const Graphics_Rectangle *pRect,
                        uint16_t ulValue)
{
    int16_t x0 = pRect->sXMin;
    int16_t x1 = pRect->sXMax;
    int16_t y0 = pRect->sYMin;
    int16_t y1 = pRect->sYMax;

    while(y0++ <= y1)
    {
        SSD1306_DriverLineDrawH(pvDisplayData, x0, x1, y0, ulValue);
    }
}

//*****************************************************************************
//
//! Translates a 24-bit RGB color to a display driver-specific color.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param ulValue is the 24-bit RGB color.  The least-significant byte is the
//! blue channel, the next byte is the green channel, and the third byte is the
//! red channel.
//!
//! This function translates a 24-bit RGB color into a value that can be
//! written into the display's frame buffer in order to reproduce that color,
//! or the closest possible approximation of that color.
//!
//! \return Returns the display-driver specific color.
//
//*****************************************************************************
static uint32_t
SSD1306_DriverColorTranslate(void *pvDisplayData,
                              uint32_t ulValue)
{
    /* The DPYCOLORTRANSLATE macro should be defined in TemplateDriver.h */

    //
    // Translate from a 24-bit RGB color to a color accepted by the LCD.
    //
    return(DPYCOLORTRANSLATE(ulValue));
}

//*****************************************************************************
//
//! Flushes any cached drawing operations.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//!
//! This functions flushes any cached drawing operations to the display.  This
//! is useful when a local frame buffer is used for drawing operations, and the
//! flush would copy the local frame buffer to the display.
//!
//! \return None.
//
//*****************************************************************************
static void
SSD1306_DriverFlush(void *pvDisplayData)
{
    // Flush Buffer here. This function is not needed if a buffer is not used,
    // or if the buffer is always updated with the screen writes.
    int16_t i = 0,j = 0;
    for(i = 0; i < LCD_Y_SIZE; i++)
    {
        for(j = 0; j < (LCD_X_SIZE * BPP + 7) / 8; j++)
        {
            SSD1306_DriverPixelDraw(pvDisplayData, j, i,
                                    SSD1306_Memory[i * LCD_Y_SIZE + j]);
        }
    }
}

//*****************************************************************************
//
//! Send command to clear screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//!
//! This function does a clear screen and the Display Buffer contents
//! are initialized to the current background color.
//!
//! \return None.
//
//*****************************************************************************
static void
SSD1306_DriverClearScreen(void *pvDisplayData,
                           uint16_t ulValue)
{
    // This fills the entire display to clear it
    // Some LCD drivers support a simple command to clear the display
    int16_t y0 = 0;
    while(y0++ <= (LCD_Y_SIZE - 1))
    {
        SSD1306_DriverLineDrawH(pvDisplayData, 0, LCD_X_SIZE - 1, y0, ulValue);
    }
}

//*****************************************************************************
//
//! The display structure that describes the driver for the blank template.
//
//*****************************************************************************
const Graphics_Display g_sSSD1306_Driver =
{
    sizeof(tDisplay),
    SSD1306_Memory,
#if defined(PORTRAIT) || defined(PORTRAIT_FLIP)
    LCD_Y_SIZE,
    LCD_X_SIZE,
#else
    LCD_X_SIZE,
    LCD_Y_SIZE,
#endif
    SSD1306_DriverPixelDraw,
    SSD1306_DriverPixelDrawMultiple,
    SSD1306_DriverLineDrawH,
    SSD1306_DriverLineDrawV,
    SSD1306_DriverRectFill,
    SSD1306_DriverColorTranslate,
    SSD1306_DriverFlush,
    SSD1306_DriverClearScreen
};

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
