#include "msp.h"
#include "BSP.h"
#include "G8RTOS_Lab3/G8RTOS.h"
#include "Threads.h"
#include "uart/myuart.h"
#include "driverlib.h"
#include <stdbool.h>
#include "led/led.h"
#include <LCD.h>


void LCD_DEBUG(void);


void main(void)
{

    G8RTOS_Init();
    LCD_Init(true);

    //init GPIOs for measuring jitter
    P1->DIR = 1;
    P1->OUT = 0;

    P2->DIR |= 0x7; // set the first three pins of P2 to be outputs
    P2->OUT = 0; // turn them off

    G8RTOS_InitSemaphore(&LCDMutex,1);

//    G8RTOS_InitFIFO(BALLFIFO);

    G8RTOS_AddThread(&idleThread,200,"IDLE");
//    G8RTOS_AddThread(&Accel_Read,100,"accel");
//    G8RTOS_AddThread(&wait_for_tap,70,"waitingtap");

    G8RTOS_AddThread(&startgame,10,"start");
    G8RTOS_AddAPeriodicEvent(&LCD_Tap, 2, PORT4_IRQn);


    G8RTOS_Launch();


    PCM_gotoLPM0();
}














