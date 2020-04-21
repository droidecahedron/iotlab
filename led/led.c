/*
 * lcd.c
 *
 *  Created on: Jan 30, 2020
 *      Author: Johnny
 */

#include "msp.h"
#include "led.h"

#define BYTE(x,data) (data>>((x-1)*8) & 0x00FF)

//lp3943 colorset
static void LP3943_ColorSet(uint32_t unit, uint32_t PWM_DATA)
{

}

//lp3943 ledmodeset
void LP3943_LEDModeSet(uint32_t unit, uint16_t LED_DATA)
{
    //operating modes are on, off, pwm1, pwm2
    //UNIT 0 Red
    //UNIT 1 Blue
    //Unit 2 Green

    //Registers written to are 0x6 to 0x9 ls0-3
    //LS0 : LED0-3 selector
    //LS1 : LED4-7 selector
    //LS2 : LED8-11 selector
    //LS3 : LED12-15 selector


    //flip input data so its not upside down (0x8C -> 0x31)
    uint16_t flipped_data = 0;
    for(int i = 0; i <= 15; i++) // 15 is number of bits to reverse
    {
         if((LED_DATA & (1 << i)))
             flipped_data |= 1 << (15 - i);
    }

    //turn data into needed 01's to turn on each led (0xFFFF -> 0x55555555)
    uint32_t decoded_data = 0;
    for(int i=15;i>=0;i--)
    {
        if((flipped_data>>i) & 0x1) // check for nonzero in that bit position
        {
            decoded_data |= 1 << (2*i); // shifting 0x01 over i times
        }
    }

    //load the address
    UCB2I2CSA = unit + 0x60;


    for(int i=0;i<4;i++)
    {
    //START CONDITION
    UCB2CTLW0 |= EUSCI_B_CTLW0_TXSTT | EUSCI_B_CTLW0_TR;
    while((UCB2CTLW0 & UCTXSTT)); //  page 801 [wait for it to be ready]

    //transmit register address
    UCB2TXBUF = 0x0009-i;   //6,7,8,9
    while(!(UCB2IFG & EUSCI_B_IFG_TXIFG0)); // wait for transmission to finish

    //transmit LED data for 15-12
    UCB2TXBUF = (uint8_t)(BYTE(4-i,decoded_data));
    while(!(UCB2IFG & EUSCI_B_IFG_TXIFG0)); // wait for transmission to finish

    //STOP CONDITION
    UCB2CTLW0 |= EUSCI_B_CTLW0_TXSTP;
    while((UCB2CTLW0 & EUSCI_B_CTLW0_TXSTP)); // wait for stop condition
    }

}

//lp3943 init
void init_RGBLEDS()
{
    uint16_t UNIT_OFF = 0x0000;
    uint16_t UNIT_ON = 0xFFFF;

    //software reset enable
    UCB2CTLW0 = UCSWRST;

    //initialize i2c master
    // MASTER MODE, I2C MODE, SYNCHRONOUS, SMCLK,  TRANSMITTER
    UCB2CTLW0 |= (EUSCI_B_CTLW0_MST | EUSCI_B_CTLW0_MODE_3 | EUSCI_B_CTLW0_SYNC | EUSCI_B_CTLW0_UCSSEL_3 | EUSCI_B_CTLW0_TR);

    //set the Fclk as 400 khz
    uint32_t clkspeed = CS_getSMCLK();
    clkspeed = clkspeed / 400000.0;
    UCB2BRW = clkspeed;

    //set pins as i2c mode
    //LED SCL is P3.7, LED SDA is 3.6
    P3SEL0 |= (BIT7 | BIT6);
    P3SEL1 &= ~(BIT7 | BIT6);



    //bitwise and all bits except ucswrst
    UCB2CTLW0 &= ~UCSWRST;

//    LP3943_LEDModeSet(RED, UNIT_OFF);
//    LP3943_LEDModeSet(GREEN, UNIT_OFF);
//    LP3943_LEDModeSet(BLUE, UNIT_OFF);


}


