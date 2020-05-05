#include <msp430.h> 
#include <stdlib.h>
#include <stdio.h>

#define FLAGS UCA0IFG // Contains the transmit & receive flags
#define RXFLAG UCRXIFG // Receive flag
#define TXFLAG UCTXIFG // Transmit flag
#define TXBUFFER UCA0TXBUF // Transmit buffer
#define RXBUFFER UCA0RXBUF // Receive buffer

#define REDLED BIT0

/*
 * UART CONFIGURATION:
 * USC1_AO
 * TXD -> P4.2
 * RXD -> P4.3
 */

/**
 * main.c
 */

// Function prototypes:
void Initialize_UART(void);
void Transmit(unsigned char c);
void Toggle(unsigned char c);
unsigned char Recieve(void);

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	PM5CTL0 &= ~LOCKLPM5;

	P1DIR |= REDLED;
	P1OUT &= ~REDLED;

	Initialize_UART();

	unsigned char c;

	// Echoes received data.
	for (;;)
	{
	    c = Recieve();

	    Transmit(c);
	}


	return 0;
}

void Initialize_UART(void)
{
    // Configure pins to UART
    //P2SEL1 &= ~(BIT0 | BIT1);
    //P2SEL0 |= (BIT0 | BIT1);

    P4SEL1 &= ~(BIT2 | BIT3);
    P4SEL0 |= (BIT2 | BIT3);

    UCA0CTLW0 |= UCSSEL_2;

    UCA0BRW = 6;
    UCA0MCTLW |= (UCOS16 | UCBRF0 | UCBRF2 | UCBRF3 | UCBRS5 | UCBRS1);

    UCA0CTLW0 &= ~UCSWRST;
}

void Transmit(unsigned char c)
{
    while ((FLAGS & TXFLAG) == 0) {}

    TXBUFFER = c;
}

unsigned char Recieve(void)
{
    unsigned char value;
    if ((FLAGS & RXFLAG) == 0)
    {
        return NULL;
    }

    value = RXBUFFER;

    // Toggle REDLED based on value.
    Toggle(value);

    return value;
}


void Toggle(unsigned char data)
{
    if (data == 'A')
    {
        P1OUT |= REDLED;
    }
    else if (data == 'B')
    {
        P1OUT &= ~REDLED;
    }
}
