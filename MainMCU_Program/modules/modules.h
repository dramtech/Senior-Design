// All sensor and modules functions and set up

#include <msp430.h>

#ifndef __MODULES_H__
#define __MODULES_H__

//////////////////////////////////////////////
// Ultrasonic sensor section /////////////////
//////////////////////////////////////////////

extern void us_init();
extern void getDistance(unsigned int * data);

//////////////////////////////////////////////
// Gyro/Accelerometer ////////////////////////
//////////////////////////////////////////////

#define GYRO_ADDRESS 0x68 // MPU6050 I2C address
#define g_VALUE 16384.0 // 1g = 16384 from datasheet
#define PI 3.14159265 // Pi value to calculate angle of inclination

// Registers
#define TEMP_REG 0x41
#define RESET_REG 0x68 // Signal Path Reset register
#define X_VEC 0x3B
#define Y_VEC 0x3D
#define Z_VEC 0x3F

// Values
#define DEVICE_RESET 0x80

extern void getTemp(int * value);
extern void getAngle(float * value);
extern void Initialize_Gyro();

//////////////////////////////////////////////
// Bluetooth ////////////////////////
//////////////////////////////////////////////



#endif
