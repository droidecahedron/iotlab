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
    P4->OUT |= BIT4; // Sets res to pull-up
    P4->DIR &= ~BIT4; // its an input
    P4->REN |= BIT4; // Pull-up resister
    //button 1
    P4->OUT |= BIT5;
    P4->DIR &= ~BIT5;
    P4->REN |= BIT5;

//    // get the MAC address
//    GET_MAC_ADDR;
//    initCC3100(Host);

    G8RTOS_InitSemaphore(&joystickMutex,1);
    G8RTOS_InitSemaphore(&LCDMutex,1);
    G8RTOS_InitSemaphore(&sendMutex,1);
    G8RTOS_InitSemaphore(&receiveMutex,1);
    //gamestate semaphores
    G8RTOS_InitSemaphore(&gsMutex_current,1);
    G8RTOS_InitSemaphore(&gsMutex_previous,1);

    //LED score mutex
    //.. if your current LED scores dont match your previous LED scores, signal
    //.. the moveLED thread will wait until this condition is true.
    //.. since moveLED doesn't sleep, we just lock it until the signal comes.
    G8RTOS_InitSemaphore(&gsMutex_LED,0);

    LCD_Text(50,50,"B0 for host, B1 for client",LCD_GREEN);
    role = GetPlayerRole();
    LCD_Text(50,50,"B0 for host, B1 for client",LCD_BLACK);

    if(role==Host) G8RTOS_AddThread(&CreateGame,0,"MAKEGAME");

    else G8RTOS_AddThread(&JoinGame,0,"JOINGAME");

    G8RTOS_Launch();

    //please dont happen
    //this cant happen
    //if it does, i'll call G8RTOS_KillSelf();
    //give me mercy from the RONA
    //big sorry
    while(1)
    {
        //this is my default handler
        int butts = 9999;
    }
}







