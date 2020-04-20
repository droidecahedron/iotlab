#include "msp.h"
#include "BSP.h"
#include "G8RTOS_Lab3/G8RTOS.h"
#include "G8RTOS_Lab3/G8RTOS_IPC.h"
#include "led/led.h"
#include "Game.h"
#include <stdbool.h>
#include "G8RTOS_Lab3/G8RTOS_Structures.h"


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

//i hate the rona
int johnny_laptop_ip = 0xc0a8000b; // 192.168.0.11
int jovan_laptop_ip = 0x0a000005; // 10.0.0.5



/******global vars******/

GameState_t gamestate;
Ball_t all_the_balls[MAX_NUM_OF_BALLS];
PrevBall_t prev_balls[MAX_NUM_OF_BALLS];

//~~~ DATA STRUCTURES~~~//
//+++ HOST data structs
GeneralPlayerInfo_t general_host_info =
    {
        .currentCenter = PADDLE_X_CENTER,
        .color = LCD_RED,
        .position = BOTTOM,
    };

SpecificPlayerInfo_t specific_host_info;

//... previous host
PrevPlayer_t prevHost = { .Center = PADDLE_X_CENTER, };



//+++ CLIENT data struct
GeneralPlayerInfo_t general_client_info =
    {
        .currentCenter = PADDLE_X_CENTER,
        .color = LCD_BLUE,
        .position = TOP,
    };

SpecificPlayerInfo_t specific_client_info;

//... previous client
PrevPlayer_t prevClient = { .Center = PADDLE_X_CENTER, };




//~~~DATA STRUCTURES~~~//

//... stuff threads need
uint8_t str[64];
bool collision_detected;
uint8_t numBalls; // hee hee haha numb balls.
uint8_t PACKET_RX[16], PACKET_TX[16];


/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame()
{
    initCC3100(role); // button 0 for host, button 1 for client

    specific_client_info.ready = 1;
    SendData(&specific_client_info.ready,jovan_laptop_ip,1); // im ready, tell the host
    while(specific_host_info.acknowledge != 1)
    {
    ReceiveData(&specific_host_info.acknowledge, sizeof(&specific_host_info.acknowledge)); // wait until host acknowledges me
    }

    BITBAND_PERI(P2->OUT,1) ^= 1; // toggle green for established comms
    BITBAND_PERI(P2->OUT,2) ^= 1; // CLIENT IS BLUE
    InitBoardState();
    G8RTOS_AddThread(&ReadJoystickClient, 5, "RD_JOYSTICK");
    G8RTOS_AddThread(&GenerateBall, 6, "GENBALL");
    G8RTOS_AddThread(&SendDataToHost, 1, "IP_DAT_TX");
    G8RTOS_AddThread(&ReceiveDataFromHost, 2, "IP_DAT_RX");
    G8RTOS_AddThread(&DrawObjects,3,"DRAW_OBJ");
    G8RTOS_AddThread(&MoveLEDs, 254, "MOVELEDs");
    G8RTOS_AddThread(&IdleThread, 255, "IDLE");

    G8RTOS_KillSelf();
    while(1); // please help dont happen please

}


/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost()
{
    int butts = 9999;
}


/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost()
{
    int butts = 9999;
}


/*
 * Thread to read client's joystick
 */
void ReadJoystickClient()
{
    int butts = 9999;
}


/*
 * End of game for the client
 */
void EndOfGameClient()
{
    int butts = 9999;
}


/*********************************************** Client Threads *********************************************************************/


/*********************************************** Host Threads *********************************************************************/
/*
 * Thread for the host to create a game
 */
void CreateGame()
{
    initCC3100(role); // button 0 for host, button 1 for client
    while(specific_client_info.ready != 1)
    {
    ReceiveData(&specific_client_info.ready, sizeof(&specific_client_info.ready)); // receive client info
    }

    specific_host_info.acknowledge = 1;
    SendData(&specific_host_info.acknowledge,johnny_laptop_ip,1);


    BITBAND_PERI(P2->OUT,1) ^= 1; // toggle green for established comms
    BITBAND_PERI(P2->OUT,0) ^= 1; // host is red

    InitBoardState();
    G8RTOS_AddThread(&ReadJoystickHost, 5, "RD_JOYSTICK");
    G8RTOS_AddThread(&GenerateBall, 6, "GENBALL");
    G8RTOS_AddThread(&SendDataToClient, 1, "IP_DAT_TX");
    G8RTOS_AddThread(&ReceiveDataFromClient, 2, "IP_DAT_RX");
    G8RTOS_AddThread(&DrawObjects,3,"DRAW_OBJ");
    G8RTOS_AddThread(&MoveLEDs, 254, "MOVELEDs");
    G8RTOS_AddThread(&IdleThread, 255, "IDLE");


    G8RTOS_KillSelf();
    while(1); // please dont happen
}

/*
 * Thread that sends game state to client
 */
void SendDataToClient()
{
    //sends general player info
    int butts = 9999;
}

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient()
{
    int butts = 9999;
}

/*
 * Generate Ball thread
 */
void GenerateBall()
{
    int butts = 9999;
}

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost()
{
    int butts = 9999;
}


/*
 * Thread to move a single ball
 */
void MoveBall()
{
    int butts = 9999;
}

/*
 * End of game for the host
 */
void EndOfGameHost()
{
    int butts = 9999;
}

/*********************************************** Host Threads *********************************************************************/


/*********************************************** Common Threads *********************************************************************/
/*
 * Idle thread
 */
void IdleThread()
{
    while(1);
}



/*
 * Thread to draw all the objects in the game
 */
void DrawObjects()
{
    while(1)
    {
        G8RTOS_WaitSemaphore(&LCDMutex);

        //update players
        UpdatePlayerOnScreen(&prevHost, &gamestate.players[Host]);
        UpdatePlayerOnScreen(&prevClient, &gamestate.players[Client]);

        for(int i=0; i<MAX_NUM_OF_BALLS; i++)
        {
            if(gamestate.balls[i].alive)
            {
                //wait until all previous and current game state variables are ready to be modified
                G8RTOS_WaitSemaphore(&gsMutex_previous);
                G8RTOS_WaitSemaphore(&gsMutex_current);
                UpdateBallOnScreen(&prev_balls[i], &gamestate.balls[i], gamestate.balls[i].color);
                G8RTOS_SignalSemaphore(&gsMutex_current);
                G8RTOS_SignalSemaphore(&gsMutex_previous);
            }

            //if the current ball is DEAD, we un-draw the previous
            if( !(gamestate.balls[i].alive) && prev_balls[i].CenterX != 0 && prev_balls[i].CenterY != 0 )
            {
                LCD_DrawRectangle(prev_balls[i].CenterX - BALL_SIZE_D2, prev_balls[i].CenterX + BALL_SIZE_D2,
                                  prev_balls[i].CenterY - BALL_SIZE_D2, prev_balls[i].CenterY + BALL_SIZE_D2,LCD_BLACK);
                //special for the conditional
                prev_balls[i].CenterX = 0;
                prev_balls[i].CenterY = 0;
            }

        }

        G8RTOS_SignalSemaphore(&LCDMutex);
        sleep(20);
    }
}




/*
 * Thread to update LEDs based on score
 */
void MoveLEDs()
{
    //initialize the scores
    gamestate.LEDScores[Client] = 0;
    gamestate.LEDScores[Host] = 0;
    LP3943_LEDModeSet(RED, gamestate.LEDScores[Host]);
    LP3943_LEDModeSet(BLUE, gamestate.LEDScores[Client]);
    while(1)
    {
        G8RTOS_WaitSemaphore(&gsMutex_LED); // it will wait until the scores are updateable.
        //then update LEDs
        LP3943_LEDModeSet(RED, gamestate.LEDScores[Host]);
        LP3943_LEDModeSet(BLUE, gamestate.LEDScores[Client]);

    }
}

/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/
/*
 * Returns either Host or Client depending on button press
 */
playerType GetPlayerRole()
{

    while( (P4->IN & 1<<4) && (P4->IN & 1<<5) ) // while host and client button arent pressed
    {
        if( !(P4->IN & 1<<4) ) // BUTTON 0 PRESSED
            return Host;
        else if( !(P4->IN & 1<<5) ) // BUTTON 1 PRESSED
            return Client;
    }

    return BAD_ERROR_NO_GOOD; // OOPSIE POOPSIE

}

/*
 * Draw players given center X center coordinate
 */
void DrawPlayer(GeneralPlayerInfo_t * player)
{
    if(player->position == BOTTOM) // this will be red.
    {
        // x0 is the current center - half the paddle, x1 is + half the paddle
        // y0 for the bottom is the bottom paddle edge, y1 is the edge of the arena.
        LCD_DrawRectangle(player->currentCenter - PADDLE_LEN_D2, player->currentCenter + PADDLE_LEN_D2,
                          BOTTOM_PADDLE_EDGE, ARENA_MAX_Y, player->color);
    }
    else // otherwise it's the top (and blue)
    {
        // x0 is the current center-paddle_len/2, x1 is current center+paddle_len/2
        // y0 is the top of the arena, y1 is the edge of the paddle.
        LCD_DrawRectangle(player->currentCenter - PADDLE_LEN_D2, player->currentCenter + PADDLE_LEN_D2,
                          ARENA_MIN_Y, TOP_PADDLE_EDGE, player->color);
    }

}



/*
 * Updates player's paddle based on current and new center
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prevPlayerIn, GeneralPlayerInfo_t * outPlayer)
{
    int y_start, y_end;
    GeneralPlayerInfo_t current_player;

    if(outPlayer->position == BOTTOM) // check if we are updating host
    {
        y_start = BOTTOM_PADDLE_EDGE;
        y_end = ARENA_MAX_Y;
        current_player.currentCenter = general_host_info.currentCenter;
    }
    else // or client
    {
        y_start = ARENA_MIN_Y;
        y_end = TOP_PADDLE_EDGE;
        current_player.currentCenter = general_client_info.currentCenter;
    }

    // check if our new center minus our old center is positive, we are moving to the right
    if((outPlayer->currentCenter - prevPlayerIn->Center) >= 0)
    {
        //undraws from the previous head to new head
        LCD_DrawRectangle(prevPlayerIn->Center - PADDLE_LEN_D2, current_player.currentCenter - PADDLE_LEN_D2,
                          y_start, y_end, LCD_BLACK);
        //"adds" from old tail to new tail
        LCD_DrawRectangle(prevPlayerIn->Center + PADDLE_LEN_D2, current_player.currentCenter + PADDLE_LEN_D2,
                          y_start, y_end, outPlayer->color);

    }
    else // otherwise we are moving to the left [page 9 of lab5 slides]
    {
        // undraw from new tail to old tail
        LCD_DrawRectangle(current_player.currentCenter + PADDLE_LEN_D2, prevPlayerIn->Center + PADDLE_LEN_D2,
                          y_start, y_end, LCD_BLACK);
        LCD_DrawRectangle(current_player.currentCenter - PADDLE_LEN_D2, prevPlayerIn->Center - PADDLE_LEN_D2,
                          y_start, y_end, outPlayer->color);
    }
    //update previous player to be current player for next time.
    prevPlayerIn->Center = current_player.currentCenter;

}



/*
 * Function updates ball position on screen
 */
void UpdateBallOnScreen(PrevBall_t * previousBall, Ball_t * currentBall, uint16_t outColor)
{
    //if i wanted to make a new ball thats a copy of a passed one
    //Ball_t newball = *currentBall;

    // undraw the old ball
    LCD_DrawRectangle(previousBall->CenterX - BALL_SIZE_D2, previousBall->CenterX + BALL_SIZE_D2,
                      previousBall->CenterY - BALL_SIZE_D2, previousBall->CenterY + BALL_SIZE_D2, LCD_BLACK);

    if(currentBall->alive)
    {
        //2 left, 2 right, 2 up, 2 down.
        LCD_DrawRectangle(currentBall->currentCenterX - BALL_SIZE_D2, currentBall->currentCenterX+BALL_SIZE_D2,
                          currentBall->currentCenterY - BALL_SIZE_D2, currentBall->currentCenterY+BALL_SIZE_D2,outColor);
    }

    //undraw if it passes a paddle ?DEBUG may be able to move this
    if( (currentBall->currentCenterY + currentBall->Yvel) < ARENA_MIN_Y ||
        (currentBall->currentCenterY + currentBall->Yvel) > ARENA_MAX_Y)
    {
        //2 left, 2 right, 2 up, 2 down.
        LCD_DrawRectangle(currentBall->currentCenterX - BALL_SIZE_D2, currentBall->currentCenterX+BALL_SIZE_D2,
                        currentBall->currentCenterY - BALL_SIZE_D2, currentBall->currentCenterY+BALL_SIZE_D2,outColor);
    }

    // update previous ball
    previousBall->CenterX = currentBall->currentCenterX;
    previousBall->CenterY = currentBall->currentCenterY;


}

/*
 * Initializes and prints initial game state
 */
void InitBoardState()
{
    LCD_Clear(LCD_BLACK); // clear the lcd
    //the sides of the arena are to be defined by two vertical white lines
    //thickness 2 [note we dont want to be in the arena, hence const +/-]
    LCD_DrawRectangle(ARENA_MIN_X-2, ARENA_MIN_X, ARENA_MIN_Y, ARENA_MAX_Y, LCD_WHITE);
    LCD_DrawRectangle(ARENA_MAX_X+1, ARENA_MAX_X+3, ARENA_MIN_Y, ARENA_MAX_Y, LCD_WHITE);


    // resets game state vars & draws new game
    //reset LED vals and winner
    gamestate.LEDScores[Host] = 0;
    gamestate.LEDScores[Client] = 0;
    gamestate.overallScores[Host] = 0;
    gamestate.overallScores[Client] = 0;
    gamestate.winner = false;
    //update host
    gamestate.players[Host] = general_host_info;
    //redraw players
    DrawPlayer(&gamestate.players[Host]);
    DrawPlayer(&gamestate.players[Client]);

    //print the scores
    snprintf(str,64,"%d",gamestate.overallScores[Host]);
    LCD_Text(0, 220, str, LCD_RED); // host is on the bottom
    snprintf(str,64,"%d",gamestate.overallScores[Client]);
    LCD_Text(0,0,str,LCD_BLUE); // client is on the top
}

/*********************************************** Public Functions *********************************************************************/
