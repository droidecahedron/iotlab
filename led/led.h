/*
 * lcd.h
 *
 *  Created on: Jan 30, 2020
 *      Author: Johnny
 */

#ifndef LED_H_
#define LED_H_


//enums for RGB LEDs
typedef enum device
{
    BLUE = 0,
    GREEN = 1,
    RED = 2,
} unit_design;

//lp3943 colorset
static void LP3943_ColorSet(uint32_t unit, uint32_t PWM_DATA);

//lp3943 ledmodeset
void LP3943_LEDModeSet(uint32_t unit, uint16_t LED_DATA);

//lp3943 init
void init_RGBLEDS();


#endif /* LED_H_ */
