#include "msp.h"
#include "Game.h"

/**
 * main.c
 */
void main(void)
{
    G8RTOS_Init();

    LCD_Init(false);

    P2->DIR |= 0x7; //  set the first three pins of P2 to be outputs
    P2->OUT = 0;    //  turn them off

    sl_Start(NULL, NULL, NULL);
	_u8 MAC_ADDR[SL_MAC_ADDR_LEN];
	_u8 MAC_ADDR_LEN = SL_MAC_ADDR_LEN;

	sl_NetCfgGet(SL_MAC_ADDRESS_GET, NULL, &MAC_ADDR_LEN, (_u8 *) MAC_ADDR);

	initCC3100(Client);

	int ip = getLocalIP();
	int myLaptopIP = 0x0A000005;

	char johnnys_mac[6];
    char str[255];

	while (1)
	{
	    SendData(MAC_ADDR, myLaptopIP, MAC_ADDR_LEN);
	    BITBAND_PERI(P2->OUT, 2) ^= 1;

	    DelayMs(5);

	    ReceiveData(johnnys_mac, 6);
        BITBAND_PERI(P2->OUT,1) ^= 1; // toggle green

        if(johnnys_mac[0]==0x38)
        {
            snprintf(str,255,"Johnnys MAC (dec)%d:%d:%d:%d:%d:%d",johnnys_mac[0],johnnys_mac[1],johnnys_mac[2],johnnys_mac[3],johnnys_mac[4],johnnys_mac[5]);
            LCD_Text(10,10,str,LCD_GREEN);
        }

        DelayMs(5);
	}
}
