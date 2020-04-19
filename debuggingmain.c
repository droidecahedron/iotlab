#include "msp.h"
#include "BSP.h"
#include "G8RTOS_Lab3/G8RTOS.h"
#include "Game.h"
#include "uart/myuart.h"
#include "driverlib.h"
#include <stdbool.h>
#include "led/led.h"
#include <LCD.h>


void main(void)
{

    G8RTOS_Init();
    LCD_Init(false);

    //GPIO inits
    //LEDs
    P1->DIR = 1;
    P1->OUT = 0;
    P2->DIR |= 0x7; // set the first three pins of P2 to be outputs
    P2->OUT = 0; // turn them off

    //button 0
    P4->DIR &= ~BIT4; // its an input
    P4->REN |= BIT4; // Pull-up resister
    P4->OUT |= BIT4; // Sets res to pull-up

    // get the MAC address
    GET_MAC_ADDR;

    initCC3100(Host);



//    G8RTOS_AddThread(threadToAdd, priority, threadname)


//    G8RTOS_Launch();

    PCM_gotoLPM0();
}







