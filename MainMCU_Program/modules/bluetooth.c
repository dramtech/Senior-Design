#include <msp430.h>
#include <stdlib.h>
#include <stdio.h>

#define FLAGS UCA0IFG // Contains the transmit & receive flags
#define RXFLAG UCRXIFG // Receive flag
#define TXFLAG UCTXIFG // Transmit flag
#define TXBUFFER UCA0TXBUF // Transmit buffer
#define RXBUFFER UCA0RXBUF // Receive buffer

// 9600 Baud
void init_bluetooth(void)
{
    P4SEL1 &= ~(BIT2 | BIT3);
    P4SEL0 |= (BIT2 | BIT3);

    // Choose SMCLK
    UCA0CTLW0 |= UCSSEL_2;

    // BRCLK = 8MHZ
    UCA0BRW = 52;
    UCA0MCTLW |= (UCOS16 | UCBRF0 | UCBRS0 | UCBRS3 | UCBRS6);

    UCA0CTLW0 &= ~UCSWRST;
}

static void transmit_helper(unsigned char c) {
    while ((FLAGS & TXFLAG) == 0) {}

    TXBUFFER = c;
}

void transmit_data(unsigned char *c) {
    if (!c) return;

    while (*c) {
        transmit_helper(*c);
        c++;
    }
}

unsigned char receive_data(void)
{
    unsigned char value;
    if ((FLAGS & RXFLAG) == 0)
    {
        return NULL;
    }

    value = RXBUFFER;
    return value;
}
