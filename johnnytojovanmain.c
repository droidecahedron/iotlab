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

    //init GPIOs for measuring jitter
    P1->DIR = 1;
    P1->OUT = 0;

    P2->DIR |= 0x7; // set the first three pins of P2 to be outputs
    P2->OUT = 0; // turn them off


    sl_Start(NULL, NULL, NULL);
    _u8 MAC_ADDR[SL_MAC_ADDR_LEN];
    _u8 MAC_ADDR_LEN = SL_MAC_ADDR_LEN;

    sl_NetCfgGet(SL_MAC_ADDRESS_GET, NULL, &MAC_ADDR_LEN, (_u8 *) MAC_ADDR);

    initCC3100(Host);



//    G8RTOS_AddThread(threadToAdd, priority, threadname)


//    G8RTOS_Launch();

    uint32_t ip = getLocalIP();


    char jovans_mac[6];
    char str[255];
    int mylaptopIP = 0xc0a8000b; // 192.168.0.11
    int butts = 0;

    while(1)
    {
        ReceiveData(jovans_mac, 6);
        BITBAND_PERI(P2->OUT,2) ^= 1; // toggle blue
        if(jovans_mac[0]==0xA8)
        {
            snprintf(str,255,"Jovans MAC (dec)%d:%d:%d:%d:%d:%d",jovans_mac[0],jovans_mac[1],jovans_mac[2],jovans_mac[3],jovans_mac[4],jovans_mac[5]);
            LCD_Text(10,10,str,LCD_GREEN);
            butts=0xFF;
        }

        DelayMs(5);

        SendData(MAC_ADDR,mylaptopIP,6);
        BITBAND_PERI(P2->OUT,1) ^= 1; // toggle green

        DelayMs(5);
    }


    PCM_gotoLPM0();
}







