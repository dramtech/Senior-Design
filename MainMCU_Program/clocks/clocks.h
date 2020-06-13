// Code for programming clock modules

#include <msp430.h>

#ifndef __CLOCKS_H__
#define __CLOCKS_H__

//**********************************
// Configures ACLK to 32 KHz crystal
extern void config_ACLK_to_32KHz_crystal();

// Configure DCO and set DCO to MCLK and SMCLK
extern void config_MCLK_to_16MHz_DCO();


#endif
