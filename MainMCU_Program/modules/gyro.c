#include <msp430.h>
#include <math.h>
#include "modules.h"

////////////////////////////////////////////////////////////////////
// Read a word (2 bytes) from I2C (address, register)
static int i2c_read_word(unsigned char i2c_address, unsigned char i2c_reg, unsigned int * data) {
    _disable_interrupts();
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

    _enable_interrupts();
    return 0;
}

////////////////////////////////////////////////////////////////////
// Write a single byte to I2C (address, register)
static int i2c_write_byte(unsigned char i2c_address, unsigned char i2c_reg, unsigned char data){
    _disable_interrupts();
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
    _enable_interrupts();
    return 0;
}

////////////////////////////////////////////////////////////////////
// Read a single byte from I2C (address, register)
static int i2c_read_byte(unsigned char i2c_address, unsigned char i2c_reg, unsigned char * data){
    _disable_interrupts();
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
    _enable_interrupts();
    return 0;
}

// Function that initialize gyro sensor, must be called after Initialize_I2C
void Initialize_Gyro() {
    // Reset device
    unsigned char test = 1;
    i2c_write_byte(GYRO_ADDRESS, 0x6B, DEVICE_RESET);
    // Wait for reset to complete
    while(test != 0) {
        i2c_read_byte(GYRO_ADDRESS, 0x6B, &test);
        test &= 0x80;
    }

    // Setting gyro Full Scale Range = +- 250
    unsigned char gyroReg = 0;
    int flag = 0;
    flag = i2c_read_byte(GYRO_ADDRESS, 0x1B, &gyroReg); // Reading gyro conf register

    unsigned char dat1 = (gyroReg & 0x07); // Setting up appropriate bits
    i2c_write_byte(GYRO_ADDRESS, 0x1B, dat1);

    // Disable sleep mode and enabling temperature sensor
    unsigned char powReg = 0;
    unsigned char dat2;
    flag = i2c_read_byte(GYRO_ADDRESS, 0x6B, &powReg);

    dat2 = powReg & ~0x40;
    i2c_write_byte(GYRO_ADDRESS, 0x6B, dat2);
}

// Function will return current outside temperature from sensor
void getTemp(int * value) {
    unsigned int data = 0;
    i2c_read_word(GYRO_ADDRESS, TEMP_REG, &data);

    int temp = (int) data;
    // Formula to calculate temperature from sensor data
    // from datasheet
    temp = (int)round((temp / 340.0) + 36.53);
    *value = temp;
}

// Function will return angle of inclination with respect to world Z axis
void getAngle(float * value) {
    unsigned int x, y, z;
    float vecX, vecY, vecZ;
    float m;

    // Reading x axis value
    i2c_read_word(GYRO_ADDRESS, X_VEC, &x);
    // Reading y axis value
    i2c_read_word(GYRO_ADDRESS, Y_VEC, &y);
    // Reading z axis value
    i2c_read_word(GYRO_ADDRESS, Z_VEC, &z);

    // Normalize g force vector
    vecX = (int)x / g_VALUE;
    vecY = (int)y / g_VALUE;
    vecZ = (int)z / g_VALUE;

    // Compute vector magnitude
    m = sqrt((vecX * vecX) + (vecY * vecY) + (vecZ * vecZ));

    // Compute angle of inclination relative to z axis
    *value = acos(vecZ / m) * (180.0 / PI);

}
