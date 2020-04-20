/*
 * Threads.c
 *
 */

#include "msp.h"
#include "BSP.h"
#include "G8RTOS_Lab3/G8RTOS.h"
#include "G8RTOS_Lab3/G8RTOS_IPC.h"
#include "led/led.h"
#include "Threads.h"
#include <stdbool.h>
#include "G8RTOS_Lab3/G8RTOS_Structures.h"



//globals
typedef struct ball_t ball_t;

struct ball_t
{
    uint16_t xpos;
    uint16_t ypos;
    int16_t xvel;
    int16_t yvel;
    bool alive;
    threadId_t ballthreadID;
    uint16_t color;
    semaphore_t ball_semaphore;
};


#define LCD_WHITE          0xFFFF
#define LCD_BLACK          0x0000
#define LCD_BLUE           0x0197
#define LCD_RED            0xF800
#define LCD_MAGENTA        0xF81F
#define LCD_GREEN          0x07E0
#define LCD_CYAN           0x7FFF
#define LCD_YELLOW         0xFFE0
#define LCD_GRAY           0x2104
#define LCD_PURPLE         0xF11F
#define LCD_ORANGE         0xFD20
#define LCD_PINK           0xfdba
#define LCD_OLIVE          0xdfe4

uint16_t COLOR_MATRIX[] = {LCD_PURPLE,LCD_WHITE,LCD_BLUE,LCD_RED,LCD_MAGENTA,LCD_GREEN,LCD_CYAN,LCD_YELLOW,LCD_PURPLE,LCD_ORANGE,LCD_PINK,LCD_CYAN,LCD_WHITE,LCD_BLUE,LCD_RED,LCD_MAGENTA,LCD_GREEN,LCD_CYAN,LCD_YELLOW,LCD_PURPLE,LCD_ORANGE};



int16_t accelerometerX, accelerometerY;
bool touch_detected = false;
Point posdata={0,0};
int ball_count = 0;
ball_t all_the_balls[20];
bool kill_the_ball = false;
char tempstr[20];


//quiz variables
int16_t x_joystick_pos, y_joystick_pos;
bool snakeflag = false;



void Accel_Read(void){
    while(1){
        G8RTOS_WaitSemaphore(&sensorMutex);
        bmi160_read_accel_x(&accelerometerX);
        bmi160_read_accel_y(&accelerometerY);
        accelerometerY *= -1;
        G8RTOS_SignalSemaphore(&sensorMutex);
        BITBAND_PERI(P2->OUT,1) ^= 1; // TOGGLE GREEN LED
        sleep(100);
    }
}

void wait_for_tap(void){

    BITBAND_PERI(P1->OUT,0) ^= 1; // REED
    kill_the_ball = false;
    G8RTOS_WaitSemaphore(&LCDMutex);
    posdata = TP_ReadXY();//read touch data
    G8RTOS_SignalSemaphore(&LCDMutex);

    int capital_punishment_index = 0;

    //check for existing balls
    for(int i=0;i<=ball_count;i++)
    {
        if(all_the_balls[i].xpos-5 <= posdata.x && all_the_balls[i].xpos+5 >= posdata.x && all_the_balls[i].alive)
        {
            //if its here, its within the x margin
            if(all_the_balls[i].ypos+5 >= posdata.y && all_the_balls[i].ypos-5 <= posdata.x)
            {
                //if its here, its within both margins
                kill_the_ball = true;
                capital_punishment_index = i;
            }
        }
    }

    if(!kill_the_ball) //if there's no ball to kill, add the ball.
    {
        if(ball_count<20)
        {
        uint32_t tempX = posdata.x;
        uint32_t tempY = posdata.y;
        if(posdata.x>=315) tempX = 315;
        if(posdata.y>=235) tempY = 235;
        G8RTOS_WaitSemaphore(&posFIFO);
        writeFIFO(BALLFIFO,tempX); // write pos data to fifo
        writeFIFO(BALLFIFO,tempY);
        G8RTOS_SignalSemaphore(&posFIFO);
        G8RTOS_AddThread(&BallThread,69,"BOLAS");
        }
    }
    else //otherwise kill the ball
    {
        BITBAND_PERI(P2->OUT,0) ^= 1; // redrum
        BITBAND_PERI(P2->OUT,2) ^= 1; // BLOO
        G8RTOS_WaitSemaphore(&LCDMutex); // in case ball was using LCD, wait for it to finish
        LCD_DrawRectangle(all_the_balls[capital_punishment_index].xpos, all_the_balls[capital_punishment_index].xpos+4, all_the_balls[capital_punishment_index].ypos, all_the_balls[capital_punishment_index].ypos+4, LCD_BLACK);
        G8RTOS_KillThread(all_the_balls[capital_punishment_index].ballthreadID); // once mutex grabbed, kill ball
        all_the_balls[capital_punishment_index].alive = false; // omae wa mou shinderu
        G8RTOS_SignalSemaphore(&LCDMutex);
        ball_count--;
    }

        sleep(500);
        P4->IFG &= ~BIT0;
        P4->IE |= BIT0; // re enable interrupts
        G8RTOS_KillSelf();

}


void BallThread(void){
    int i; // crappy alternative to PID
    for(i=0;i<21;i++)
    {
        if(all_the_balls[i].alive == 0)
        {
             all_the_balls[i].alive = true;
             all_the_balls[i].color = COLOR_MATRIX[ball_count];
             G8RTOS_WaitSemaphore(&posFIFO);
             all_the_balls[i].xpos = readFIFO(BALLFIFO);//pull out the x
             all_the_balls[i].ypos = readFIFO(BALLFIFO);//pull out the y
             G8RTOS_SignalSemaphore(&posFIFO);
             all_the_balls[i].xvel = accelerometerX;
             all_the_balls[i].yvel = accelerometerY;
             all_the_balls[i].ballthreadID = G8RTOS_GetThreadId();
             ball_count++;
             break;
        }
    }


    while(1)
    {
        G8RTOS_WaitSemaphore(&LCDMutex);
        //this is a buffered screen. next time it writes it will have new position.
        LCD_DrawRectangle(all_the_balls[i].xpos, all_the_balls[i].xpos+4, all_the_balls[i].ypos, all_the_balls[i].ypos+4, LCD_BLACK);

        //update velocity in case its too small
        //flip yvel because the accel is upside down from our perspective
        if(all_the_balls[i].xvel/1000 < 1 & all_the_balls[i].xvel>0) all_the_balls[i].xvel = 1000;
        if(all_the_balls[i].yvel/1000 < 1 & all_the_balls[i].yvel>0) all_the_balls[i].yvel = 1000;
        if(all_the_balls[i].xvel/1000 > -1 & all_the_balls[i].xvel < 0) all_the_balls[i].xvel = -1000;
        if(all_the_balls[i].yvel/1000 > -1 & all_the_balls[i].yvel < 0) all_the_balls[i].yvel = -1000;

        all_the_balls[i].xpos += all_the_balls[i].xvel/1000;
        all_the_balls[i].ypos += all_the_balls[i].yvel/1000;

        if(all_the_balls[i].xpos>400) all_the_balls[i].xpos = 315; // wrapped negatively
        else if(all_the_balls[i].xpos>=315) all_the_balls[i].xpos = 0; // wrapped positively
        if(all_the_balls[i].ypos>400) all_the_balls[i].ypos = 235; // wrapped negatively
        else if(all_the_balls[i].ypos >=235) all_the_balls[i].ypos = 0; // wrapped positively

        LCD_DrawRectangle(all_the_balls[i].xpos, all_the_balls[i].xpos+4, all_the_balls[i].ypos, all_the_balls[i].ypos+4, all_the_balls[i].color);

        G8RTOS_SignalSemaphore(&LCDMutex);
        sleep(30);


    }

}


//aperiodic thread
void LCD_Tap(void){
    P4->IFG &= ~BIT0; // must clear IFG flag
    // reading PxIV will
    // automatically clear IFG

    // rest of ISR
    P4->IE &= ~BIT0; // disable the interrupt
    G8RTOS_AddThread(&wait_for_tap,130,"w8fortap"); // add the tap thread [demo]
//    snakeflag=true;
}



/*
 *
 * idle thread
 */
void idleThread(void){
    while(1)
    {
    }
}









//quiz threads
void joystick_read(void){
    GetJoystickCoordinates(&x_joystick_pos,&y_joystick_pos); // don't care about the y though
}


























//debug threads


void print_pos(void)
{
    while(1)
    {
        if(touch_detected)
        {
            DelayMs(150);
            if(touch_detected)
            {
                //then its touched for real, not bouncy
                G8RTOS_WaitSemaphore(&LCDMutex); // lock resource
                posdata = TP_ReadXY();
                char pos_string[255];
                snprintf(pos_string, 255, "Xpos is %d, Ypos is %d",posdata.x, posdata.y);
                LCD_Text(50, 50, pos_string, LCD_GREEN);
                DelayMs(1000);
                LCD_Text(50, 50, pos_string, LCD_BLACK);
                G8RTOS_SignalSemaphore(&LCDMutex); // release resource
                touch_detected = false;
            }
        }
        sleep(500);
    }
}


void touch_rect(void)
{
    while(1)
    {
        if(touch_detected)
        {
            DelayMs(500);
            if(touch_detected)
            {

                BITBAND_PERI(P2->OUT,2) ^= 1; // TOGGLE BLOO LED
                //then its touched for real, not bouncy
                G8RTOS_WaitSemaphore(&LCDMutex); // lock resource
                posdata = TP_ReadXY();
                LCD_DrawRectangle(posdata.x, posdata.x+4, posdata.y, posdata.y+4, LCD_PURPLE);
                G8RTOS_SignalSemaphore(&LCDMutex); // release resource
                touch_detected = false;
            }
        }
        sleep(500);
    }
}












