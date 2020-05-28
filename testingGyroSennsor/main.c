#include <msp430.h>
#include <stdio.h>
#include <math.h>

/*
 * Test code for MPU6050
 * I2C used to communicate with sensor
 * UART used to transmit data to PC terminal.
 * Computing angle of inclination using the accelerometer
 *
 * Pin assignment for MPU6050 -> MSP430FR6989:
 * Vcc - 3.3v
 * GND - GND
 * SCL - P4.1
 * SDA - P4.0
 * AD0 - GND
 */

#define FLAGS UCA1IFG // Contains the transmit & receive flags
#define RXFLAG UCRXIFG // Receive flag
#define TXFLAG UCTXIFG // Transmit flag
#define TXBUFFER UCA1TXBUF // Transmit buffer
#define RXBUFFER UCA1RXBUF // Receive buffer
#define redLED BIT0 // Red LED at P1.0
#define greenLED BIT7 // Green LED at P9.7

#define GYRO_ADDRESS 0x68 // MPU6050 I2C address
#define g_VALUE 16384.0 // 1g = 16384 from datasheet

#define PI 3.14159265

// Configure UART to the popular configuration
// 4800 baud, 8-bit data, LSB first, no parity bits, 1 stop bit
// no flow control
// Initial clock: ACLK @ 32 kHz without oversampling
void Initialize_UART(void){

    // Divert pins to UART functionality
    P3SEL1 &= ~(BIT4|BIT5);
    P3SEL0 |= (BIT4|BIT5);

    // Use ACLK clock
    UCA1CTLW0 |= UCSSEL_1;

    // Configure the clock dividers and modulators
    // UCBR=6, UCBRF=x, UCBRS=0xEE, UCOS16=0 (oversampling)
    UCA1BRW = 6;
    UCA1MCTLW = UCBRS7|UCBRS6|UCBRS5|UCBRS3|UCBRS2|UCBRS1;

    // Exit the reset state (so transmission/reception can begin)
    UCA1CTLW0 &= ~UCSWRST;
}

// Configure eUSCI in I2C master mode
void Initialize_I2C(void) {
    // Enter reset state before the configuration starts...
    UCB1CTLW0 |= UCSWRST;

    // Divert pins to I2C functionality
    P4SEL1 |= (BIT1|BIT0);
    P4SEL0 &= ~(BIT1|BIT0);

    // Keep all the default values except the fields below...
    // (UCMode 3:I2C) (Master Mode) (UCSSEL 1:ACLK, 2,3:SMCLK)
    UCB1CTLW0 |= UCMODE_3 | UCMST | UCSSEL_3;

    // Clock divider = 8 (SMCLK @ 1.048 MHz / 8 = 131 KHz)
    UCB1BRW = 8;

    // Exit the reset mode
    UCB1CTLW0 &= ~UCSWRST;
}

// The function transmit a byte over UART
void uart_write_char(unsigned char ch){
    // Wait for any ongoing transmission to complete
    while ( (FLAGS & TXFLAG)==0 ) {}

    // Write the byte to the transmit buffer
    TXBUFFER = ch;
}

void uart_write_string(char * str){
    // Looping through the string and transmitting each char
    while(*str != NULL){
        uart_write_char(*str);
        str++;
    }
}

// The function returns the byte; if none received, returns NULL
unsigned char uart_read_char(void){
    unsigned char temp;

    // Return NULL if no byte received
    if( (FLAGS & RXFLAG) == 0)
        return NULL;

    // Otherwise, copy the received byte (clears the flag) and return it
    temp = RXBUFFER;
    return temp;
}

////////////////////////////////////////////////////////////////////
// Read a word (2 bytes) from I2C (address, register)
int i2c_read_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int * data) {
    unsigned char byte1, byte2;

    // Initialize the bytes to make sure data is received every time
    byte1 = 111;
    byte2 = 111;

    //********** Write Frame #1 ***************************
    UCB1I2CSA = i2c_address; // Set I2C address

    UCB1IFG &= ~UCTXIFG0;
    UCB1CTLW0 |= UCTR; // Master writes (R/W bit = Write)
    UCB1CTLW0 |= UCTXSTT; // Initiate the Start Signal

    while ((UCB1IFG & UCTXIFG0) ==0) {}

    UCB1TXBUF = i2c_reg; // Byte = register address

    while((UCB1CTLW0 & UCTXSTT)!= 0) {}

    if(( UCB1IFG & UCNACKIFG )!= 0) return -1;

    UCB1CTLW0 &= ~UCTR; // Master reads (R/W bit = Read)
    UCB1CTLW0 |= UCTXSTT; // Initiate a repeated Start Signal
    //****************************************************

    //********** Read Frame #1 ***************************
    while ( (UCB1IFG & UCRXIFG0) == 0) {}
    byte1 = UCB1RXBUF;
    //****************************************************

    //********** Read Frame #2 ***************************
    while((UCB1CTLW0 & UCTXSTT)!=0) {}
    UCB1CTLW0 |= UCTXSTP; // Setup the Stop Signal

    while ( (UCB1IFG & UCRXIFG0) == 0) {}
    byte2 = UCB1RXBUF;

    while ( (UCB1CTLW0 & UCTXSTP) != 0) {}
    //****************************************************

    // Merge the two received bytes
    *data = ( (byte1 << 8) | (byte2 & 0xFF) );
    return 0;
}

////////////////////////////////////////////////////////////////////
// Read a single byte from I2C (address, register)
int i2c_read_word_single(unsigned char i2c_address, unsigned char i2c_reg, unsigned char * data){
    unsigned char byte1;

    // Initialize the bytes to make sure data is received every time
    byte1 = 111;

    //********** Write Frame #1 ***************************
    UCB1I2CSA = i2c_address; // Set I2C address

    UCB1IFG &= ~UCTXIFG0; // Clear ACK flag
    UCB1CTLW0 |= UCTR; // Master writes (R/W bit = Write)
    UCB1CTLW0 |= UCTXSTT; // Initiate the Start Signal

    while ((UCB1IFG & UCTXIFG0) ==0) {}

    UCB1TXBUF = i2c_reg; // Byte = register address

    while((UCB1CTLW0 & UCTXSTT)!= 0) {}

    if(( UCB1IFG & UCNACKIFG )!= 0) return -1;

    UCB1CTLW0 &= ~UCTR; // Master reads (R/W bit = Read)
    UCB1CTLW0 |= UCTXSTT; // Initiate a repeated Start Signal
    //****************************************************

    //********** Read Frame #1 ***************************
    while ( (UCB1IFG & UCRXIFG0) == 0) {}
    byte1 = UCB1RXBUF;
    //****************************************************

    while((UCB1CTLW0 & UCTXSTT)!=0) {}
    UCB1CTLW0 |= UCTXSTP; // Setup the Stop Signal

    while ( (UCB1CTLW0 & UCTXSTP) != 0) {}
    //****************************************************

    *data = byte1;
    return 0;
}

////////////////////////////////////////////////////////////////////
// Write a word (2 bytes) to I2C (address, register)
int i2c_write_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int data) {
    unsigned char byte1, byte2;

    byte1 = (data >> 8) & 0xFF; // MSByte
    byte2 = data & 0xFF; // LSByte

    UCB1I2CSA = i2c_address; // Set I2C address

    UCB1IFG &= ~UCTXIFG0;
    UCB1CTLW0 |= UCTR; // Master writes (R/W bit = Write)
    UCB1CTLW0 |= UCTXSTT; // Initiate the Start Signal

    while ((UCB1IFG & UCTXIFG0) ==0) {}

    UCB1TXBUF = i2c_reg; // Byte = register address

    while((UCB1CTLW0 & UCTXSTT)!=0) {}

    //********** Write Byte #1 ***************************
    UCB1TXBUF = byte1;
    while ( (UCB1IFG & UCTXIFG0) == 0) {}

    //********** Write Byte #2 ***************************
    UCB1TXBUF = byte2;
    while ( (UCB1IFG & UCTXIFG0) == 0) {}

    UCB1CTLW0 |= UCTXSTP;
    while ( (UCB1CTLW0 & UCTXSTP) != 0) {}

    return 0;
}

////////////////////////////////////////////////////////////////////
// Write a single byte to I2C (address, register)
int i2c_write_word_single(unsigned char i2c_address, unsigned char i2c_reg, unsigned char data){
    unsigned char dat = data;

    UCB1I2CSA = i2c_address; // Set I2C address

    UCB1IFG &= ~UCTXIFG0; // Clear ACK flag
    UCB1CTLW0 |= UCTR; // Master writes (R/W bit = Write)
    UCB1CTLW0 |= UCTXSTT; // Initiate the Start Signal

    while ((UCB1IFG & UCTXIFG0) ==0) {}

    UCB1TXBUF = i2c_reg; // Byte = register address

    while((UCB1CTLW0 & UCTXSTT)!=0) {}

    //********** Write Byte #1 ***************************
    UCB1TXBUF = dat;
    while ( (UCB1IFG & UCTXIFG0) == 0) {}

    UCB1CTLW0 |= UCTXSTP; // Set stop bit
    while ( (UCB1CTLW0 & UCTXSTP) != 0) {}

    return 0;
}

// Function that prints 16-bit integer
void uart_write_uint16(unsigned int n){
    unsigned int digits = 10000;
    // Sending 0 digit if n = 0
    if(n == 0){
        uart_write_char('0');
        return;
    }
    // Sending a number greater then 0
    int i = 0;
    // Looping 5 times for each digit in a number up to 65535
    while(n < 65536 && i < 5){
        i++;
        // If the number is less than 10,000
        if((n / digits) == 0){
            digits /= 10;
            continue;
        }
        // Extacting the digits from MSF to LSF and transmitting
        unsigned int tmp = (n / digits) % 10;
        uart_write_char('0' + tmp);
        digits /= 10;
    }
}

// Returning 4 digit hex number as a string
char * decimalToHex(unsigned int data){

    // Digit use to isolate each digit
    int hexDigit = 0x1000;

    // String that hold the hex number
    char hex[7] = "0x";

    // Extracting each digit and converting to hex
    int j = 2;
    while(j < 6){
        unsigned char tmp = (data / hexDigit) % 16;
        if(tmp > 9)
            hex[j] = 'A' + (tmp - 10);
        else
            hex[j] = '0' + tmp;
        hexDigit /= 16;
        j++;
    }
    return hex;
}

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

// Function that initialize gyro sensor, must be called after Initialize_I2C
void Initialize_Gyro() {
    // Setting gyro Full Scale Range = +- 250
    unsigned char gyroReg = 0;
    int flag = 0;
    flag = i2c_read_word_single(GYRO_ADDRESS, 0x1B, &gyroReg); // Reading gyro conf register

    if(flag != 0)
        uart_write_string("transmission fail\n");

    unsigned char dat1 = (gyroReg & 0x07); // Setting up appropriate bits
    i2c_write_word_single(GYRO_ADDRESS, 0x1B, dat1);

    // Disable sleep mode and enabling temperature sensor
    unsigned char powReg = 0;
    unsigned char dat2;
    flag = i2c_read_word_single(GYRO_ADDRESS, 0x6B, &powReg);

    if(flag != 0)
        uart_write_string("transmission fail\n");

    dat2 = powReg & ~0x40;
    i2c_write_word_single(GYRO_ADDRESS, 0x6B, dat2);
}

// Converting a float to a string 2 decimal places
void floatToString(float input, char * output) {
    input = input * 100.0;
    int dec1 = (int)(input) % 10;
    input /= 10.0;
    int dec2 = (int)(input) % 10;
    input /= 10.0;
    sprintf(output, "%d.%d%d",(int)input, dec1, dec2);
}

int main(void){

    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5;  // Enable the GPIO pins

    // Setting up the LED
    P1DIR |= redLED; // Direct pin as output
    P9DIR |= greenLED; // Direct pin as output
    P1OUT &= ~redLED; // Turn LED Off
    P9OUT &= ~greenLED; // Turn LED Off

    // Divert pins to backchannel UART functionality
    // (UCA1TXD same as P3.4) (UCA1RXD same as P3.5)
    // (P3SEL1=00, P3SEL0=11) (P2DIR=xx)
    P3SEL1 &= ~(BIT4|BIT5);
    P3SEL0 |= (BIT4|BIT5);

    // Setting the ACLK clock to 32kHz crystal
    config_ACLK_to_32KHz_crystal();

    // Initialize UART
    Initialize_UART();

    // Initialize I2C
    Initialize_I2C();

    // Initialize Gyro sensor
    Initialize_Gyro();

    unsigned long int i;
    unsigned int value;
    float temperature;
    int temp;

    unsigned int x = 0, y = 0, z = 0;
    int rX = 0, rY = 0, rZ = 0;

    char result[150];
    char tempRes[20]; // String for temperature
    char inc_angle[10];

    // Vector values
    float vecX = 0, vecY = 0, vecZ = 0;

    // Vector magnitude
    float m = 0;

    // Angle of inclination;
    float angle = 0;

    for(;;){
        char * title = "\nSensors testing\n";

        // Reading temperature
        i2c_read_word(GYRO_ADDRESS, 0x41, &value);
        temp = (short)value;
        // Reading x axis value
        i2c_read_word(GYRO_ADDRESS, 0x3B, &x);
        // Reading y axis value
        i2c_read_word(GYRO_ADDRESS, 0x3D, &y);
        // Reading z axis value
        i2c_read_word(GYRO_ADDRESS, 0x3F, &z);

        temperature = ((temp / 340.0) + 36.53);
        floatToString(temperature, tempRes);

        rX = (short)x;
        rY = (short)y;
        rZ = (short)z;

        // Normalize vector g force vector
        vecX = rX / g_VALUE;
        vecY = rY / g_VALUE;
        vecZ = rZ / g_VALUE;

        // Compute vector magnitude
        m = sqrt((vecX * vecX) + (vecY * vecY) + (vecZ * vecZ));

        // Compute angle of inclination relative to z axis
        angle = acos(vecZ / m) * (180.0 / PI);
        floatToString(angle, inc_angle);

//        sprintf(result, "Results from Accel' sensor\nx = %d, y = %d, z = %d\nTemperature in C: %s", vecX, vecY, vecZ, tempRes);
        sprintf(result, "Angle of inclination in degrees: %s\nTemperature in C: %s", inc_angle, tempRes);

        // Transmitting data through UART
        uart_write_string(title);
        uart_write_string(result);
        uart_write_char('\n');
        uart_write_char('\r');

        P1OUT ^= redLED; // Blinking red LED for debugging

        // Delay loop
        for(i = 0; i < 150000; i++) {}
    }
}
