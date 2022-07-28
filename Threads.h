/*
 * Threads.h
 *
 */

#ifndef THREADS_H_
#define THREADS_H_

#include "G8RTOS_Lab3/G8RTOS.h"
#include "LCD/LCD.h"

#define BALLFIFO 0

semaphore_t sensorMutex;
semaphore_t LCDMutex;
semaphore_t posFIFO;


//Background threads
void Accel_Read(void);
void wait_for_tap(void);
void BallThread(void);


//aperiodic thread
void LCD_Tap(void);




//background threads
void startgame(void);
void snake(void);
void joystick_read(void);


//debug
void print_pos(void);
void touch_rect(void);


void idleThread(void);

#endif /* THREADS_H_ */
