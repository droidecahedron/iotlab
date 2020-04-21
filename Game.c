#include "msp.h"
#include "BSP.h"
#include "G8RTOS_Lab3/G8RTOS.h"
#include "G8RTOS_Lab3/G8RTOS_IPC.h"
#include "led/led.h"
#include "Game.h"
#include <stdbool.h>
#include "G8RTOS_Lab3/G8RTOS_Structures.h"
#include <math.h>
#include <time.h>


//i hate the rona
int johnny_laptop_ip = 0xc0a8000b; // 192.168.0.11
int jovan_laptop_ip = 0x0a000005; // 10.0.0.5

bool start_again;


/******global vars******/

GameState_t gamestate;
Ball_t all_the_balls[MAX_NUM_OF_BALLS]; // ?DEBUG probably dont need this
PrevBall_t prev_balls[MAX_NUM_OF_BALLS];

//... previous host
PrevPlayer_t prevHost = { .Center = PADDLE_X_CENTER, };

//... previous client
PrevPlayer_t prevClient = { .Center = PADDLE_X_CENTER, };


//~~~DATA STRUCTURES~~~//

//... stuff threads need
uint8_t str[64];
bool collision_detected; // used if a ball collides with a player
uint8_t numBalls; // hee hee haha numb balls.
uint8_t PACKET_RX[16], PACKET_TX[16];
int16_t host_joystick_x, host_joystick_y, client_joystick_x, client_joystick_y = 0;


/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame()
{
    //reinitialize structs
    gamestate.players[Client].color = LCD_BLUE;
    gamestate.players[Client].currentCenter = PADDLE_X_CENTER;
    gamestate.players[Client].position = TOP;
    gamestate.players[Host].color = LCD_RED;
    gamestate.players[Host].currentCenter = PADDLE_X_CENTER;
    gamestate.players[Host].position = BOTTOM;

    initCC3100(role); // button 0 for host, button 1 for client

    gamestate.player[Client].ready = 1;
    while(gamestate.player[Host].acknowledge != 1)
    {
        SendData(&gamestate.player[Client].ready,jovan_laptop_ip,1); // im ready, tell the host
        ReceiveData(&gamestate.player[Host].acknowledge, sizeof(gamestate.player[Host].acknowledge)); // wait until host acknowledges me
    }


    BITBAND_PERI(P2->OUT,1) ^= 1; // toggle green for established comms
    BITBAND_PERI(P2->OUT,2) ^= 1; // CLIENT IS BLUE
    InitBoardState();
    G8RTOS_AddThread(&ReadJoystickClient, 5, "RD_JOYSTICK");
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
    while(1)
    {
        G8RTOS_WaitSemaphore(&ipMutex);
        ReceiveData(&gamestate,sizeof(gamestate));
        G8RTOS_SignalSemaphore(&ipMutex);
        if(gamestate.gameDone)  G8RTOS_AddThread(&EndOfGameClient,0,"CLIENT_ENDGAME");
        sleep(5);
    }
}


/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost()
{
    while(1)
    {
        G8RTOS_WaitSemaphore(&ipMutex);
        SendData(&gamestate,jovan_laptop_ip,sizeof(gamestate));
        G8RTOS_SignalSemaphore(&ipMutex);
        sleep(2);
    }
}


/*
 * Thread to read client's joystick
 */
void ReadJoystickClient()
{
    while(1)
    {
        GetJoystickCoordinates(&client_joystick_x, &client_joystick_y);
        if(client_joystick_x > 3000)  gamestate.player[Client].displacement = -4; // MOVE 4 PIXELS LEFT
        else if(client_joystick_x < -3000)    gamestate.player[Client].displacement = 4; // RIGHT

        gamestate.players[Client].currentCenter += gamestate.player[Client].displacement;
        gamestate.player[Client].displacement = 0;

        //edge cases
        //ensures the center of the paddle is always PADDLE_LEN_D2 less/more than edges of arena
        if(gamestate.players[Client].currentCenter - PADDLE_LEN_D2 <= ARENA_MIN_X)
            gamestate.players[Client].currentCenter = ARENA_MIN_X + PADDLE_LEN_D2;
        if(gamestate.players[Client].currentCenter + PADDLE_LEN_D2 >= ARENA_MAX_X)
            gamestate.players[Client].currentCenter = ARENA_MAX_X - PADDLE_LEN_D2;

        sleep(10);
    }
}


/*
 * End of game for the client
 */
void EndOfGameClient()
{
    //wait for these to be free
    G8RTOS_WaitSemaphore(&ipMutex);
    G8RTOS_WaitSemaphore(&LCDMutex);
    G8RTOS_WaitSemaphore(&gamestateMutex);

    G8RTOS_KillAllThreads(); // kills all but currently running thread

    //re init semaphores
    G8RTOS_InitSemaphore(&ipMutex,1);
    G8RTOS_InitSemaphore(&LCDMutex,1);
    G8RTOS_InitSemaphore(&gamestateMutex,1);
    G8RTOS_InitSemaphore(&LEDMutex,0); // counting semaphore to let the LEDMove thread happen

    if(gamestate.winner == Host)
    {
        LCD_Clear(gamestate.players[Host].color);
        LCD_Text(10,10,"Host Win. Host press B0 to play again",LCD_GRAY);
    }
    else
    {
        LCD_Clear(gamestate.players[Client].color);
        LCD_Text(10,10,"Client Win. Host press B0 to play again",LCD_GRAY);
    }
    //reinitialize structs
    gamestate.players[Client].color = LCD_BLUE;
    gamestate.players[Client].currentCenter = PADDLE_X_CENTER;
    gamestate.players[Client].position = TOP;
    gamestate.player[Host].acknowledge = 0;

    gamestate.player[Client].ready = 1;
    SendData(&gamestate.player[Client].ready,jovan_laptop_ip,1); // im ready, tell the host
    while(gamestate.player[Host].acknowledge != 1)
    {
    ReceiveData(&gamestate.player[Host].acknowledge, sizeof(gamestate.player[Host].acknowledge)); // wait until host acknowledges me
    }

    InitBoardState();
    G8RTOS_AddThread(&ReadJoystickClient, 5, "RD_JOYSTICK");
    G8RTOS_AddThread(&SendDataToHost, 1, "IP_DAT_TX");
    G8RTOS_AddThread(&ReceiveDataFromHost, 2, "IP_DAT_RX");
    G8RTOS_AddThread(&DrawObjects,3,"DRAW_OBJ");
    G8RTOS_AddThread(&MoveLEDs, 254, "MOVELEDs");
    G8RTOS_AddThread(&IdleThread, 255, "IDLE");

    G8RTOS_KillSelf();
    while(1); // please help dont happen please



}


/*********************************************** Client Threads *********************************************************************/


/*********************************************** Host Threads *********************************************************************/
/*
 * Thread for the host to create a game
 */
void CreateGame()
{
    gamestate.players[Host].color = LCD_RED;
    gamestate.players[Host].currentCenter = PADDLE_X_CENTER;
    gamestate.players[Host].position = BOTTOM;
    gamestate.players[Client].color = LCD_BLUE;
    gamestate.players[Client].currentCenter = PADDLE_X_CENTER;
    gamestate.players[Client].position = TOP;
    initCC3100(Host); // button 0 for host, button 1 for client
    while(gamestate.player[Client].ready != 1)
    {
    ReceiveData(&gamestate.player[Client].ready, sizeof(gamestate.player[Client].ready)); // receive client info
    SendData(&gamestate.player[Host].acknowledge,johnny_laptop_ip,1);
    }

    gamestate.player[Host].acknowledge = 1;


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
    while(1)
    {
        G8RTOS_WaitSemaphore(&ipMutex); // wait for ip comms to be cleared
        SendData(&gamestate,johnny_laptop_ip,sizeof(gamestate)); // my msp->my laptop->jovans laptop->jovans msp
        G8RTOS_SignalSemaphore(&ipMutex);
        if(gamestate.gameDone)  G8RTOS_AddThread(&EndOfGameHost,0,"HOST_ENDGAME");
        sleep(5);
    }
}

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient()
{
    while(1)
    {
        G8RTOS_WaitSemaphore(&ipMutex); //?DEBUG maybe gamestate mutex needed
        ReceiveData(&gamestate.players[Client],sizeof(gamestate.players[Client])); // receive client general player info
        G8RTOS_SignalSemaphore(&ipMutex);
        sleep(2);
    }
}

/*
 * Generate Ball thread
 */
void GenerateBall()
{
    while(1)
    {
        if(gamestate.numberOfBalls < MAX_NUM_OF_BALLS)
        {
            G8RTOS_AddThread("MoveBall",1,"MoveBall");
            gamestate.numberOfBalls+=1;
        }
        sleep(500*gamestate.numberOfBalls); // more balls = sleep longer.
    }
}

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost()
{
    while(1)
    {
        GetJoystickCoordinates(&host_joystick_x, &host_joystick_y);
        if(host_joystick_x > 3000)  gamestate.player[Host].displacement = -4; // MOVE 4 PIXELS LEFT
        else if(host_joystick_x < -3000)    gamestate.player[Host].displacement = 4; // RIGHT

        gamestate.players[Host].currentCenter += gamestate.player[Host].displacement;
        gamestate.player[Host].displacement = 0;

        //edge cases
        //ensures the center of the paddle is always PADDLE_LEN_D2 less/more than edges of arena
        if(gamestate.players[Host].currentCenter - PADDLE_LEN_D2 <= ARENA_MIN_X)
            gamestate.players[Host].currentCenter = ARENA_MIN_X + PADDLE_LEN_D2;
        if(gamestate.players[Host].currentCenter + PADDLE_LEN_D2 >= ARENA_MAX_X)
            gamestate.players[Host].currentCenter = ARENA_MAX_X - PADDLE_LEN_D2;

        sleep(10);
    }
}


/*
 * Thread to move a single ball
 */
void MoveBall()
{
    //find a dead ball to revive
    uint8_t ball_index = 0;
    for(uint8_t i=0;i<MAX_NUM_OF_BALLS;i++)
    {
        if( !(gamestate.balls[i].alive) )
        {
            ball_index = i;
            gamestate.balls[i].alive = true; // revive the ball
            break;
        }
    }

    //we need to read/write gamestate
    //put the ball in the middle of the screen
    G8RTOS_WaitSemaphore(&gamestateMutex);
    gamestate.balls[ball_index].currentCenterX = ARENA_MAX_X>>1;
    gamestate.balls[ball_index].currentCenterY = ARENA_MAX_Y>>1;

    //give it random x and yvelocities
    if(SystemTime % 2 == 0)   gamestate.balls[ball_index].Xvel = 1; // if systime is even, right
    else    gamestate.balls[ball_index].Xvel = -1; // if odd, left

    //systime will be different at this point for differing yvels
    if(SystemTime % 2 == 0) gamestate.balls[ball_index].Yvel = 1; // if even, up
    else    gamestate.balls[ball_index].Yvel = -1; // if odd, down.

    //update previous ball struct
    prev_balls[ball_index].CenterX = gamestate.balls[ball_index].currentCenterX;
    prev_balls[ball_index].CenterY = gamestate.balls[ball_index].currentCenterY;

    G8RTOS_SignalSemaphore(&gamestateMutex);

    while(1)
    {
        //update position based on velocity (REQ ACCESS GAMESTATE)
        G8RTOS_WaitSemaphore(&gamestateMutex);
        gamestate.balls[ball_index].currentCenterX += gamestate.balls[ball_index].Xvel;
        gamestate.balls[ball_index].currentCenterY += gamestate.balls[ball_index].Yvel;

        //check if the ball passes a player
        //if it goes past the bottom, it is the client's point //?DEBUG check if balls alive?
        if(gamestate.balls[ball_index].currentCenterY >= ARENA_MAX_Y || gamestate.balls[ball_index].currentCenterY <= ARENA_MIN_Y)
        {
            gamestate.balls[ball_index].alive = false; // kill ball
            gamestate.numberOfBalls -= 1; // update gamestate with new number of balls

            if(gamestate.balls[ball_index].color == LCD_BLUE) // check if it's the client's point
            {
                gamestate.LEDScores[Client] += 1;
                G8RTOS_SignalSemaphore(&LEDMutex); // unblock the moveLED thread
                if(gamestate.LEDScores[Client] == 0xFF) // check if max score for match is reached
                {
                    gamestate.LEDScores[Host] = 0;
                    gamestate.LEDScores[Client] = 0;
                    //update overall score
                    snprintf(str,"%d",gamestate.overallScores[Client]);
                    G8RTOS_WaitSemaphore(&LCDMutex);
                    LCD_Text(0,0,str,LCD_BLACK); // undraw old score
                    gamestate.overallScores[Client] += 1; // increment score
                    snprintf(str,"%d",gamestate.overallScores[Client]); // update string
                    LCD_Text(0,0,str,LCD_BLUE); // draw new score
                    G8RTOS_SignalSemaphore(&LCDMutex);
                    if(gamestate.overallScores[Client] == 10) // first to 10
                    {
                        //?DEBUG GAME ENDING
                        gamestate.gameDone = true;
                        gamestate.winner = Client;
                        G8RTOS_SignalSemaphore(&gamestateMutex); // done accessing gamestate
                        G8RTOS_KillSelf(); // kill self
                    }
                }
            }

            else if(gamestate.balls[ball_index].color == LCD_RED) // check if it's the host's point
            {
                gamestate.LEDScores[Host] += 1;
                G8RTOS_SignalSemaphore(&LEDMutex); // unblock the moveLED thread
                if(gamestate.LEDScores[Host] == 0xFF) // check if max score for match is reached
                {
                    gamestate.LEDScores[Host] = 0;
                    gamestate.LEDScores[Client] = 0;
                    //update overall score
                    snprintf(str,"%d",gamestate.overallScores[Host]);
                    G8RTOS_WaitSemaphore(&LCDMutex);
                    LCD_Text(0,220,str,LCD_BLACK); // undraw old score
                    gamestate.overallScores[Host] += 1; // increment score
                    snprintf(str,"%d",gamestate.overallScores[Client]); // update string
                    LCD_Text(0,220,str,LCD_RED); // draw new score
                    G8RTOS_SignalSemaphore(&LCDMutex);
                    if(gamestate.overallScores[Host] == 10) // first to 10
                    {
                     //?DEBUG GAME ENDING
                     gamestate.gameDone = true;
                     gamestate.winner = Host;
                     G8RTOS_SignalSemaphore(&gamestateMutex);
                     G8RTOS_KillSelf();
                    }
                }
            }
        //if neither of the two conditionals happened, it wasn't a player's ball
        }

        //if its here, it didn't pass a player, check for collisions
        //collision functions check collisions and update velocities/color
        //check if ball hit by any walls
        check_ball_wall_collision(&gamestate.balls[ball_index]);
        //check if ball hits a player
        check_ball_player_collision(&gamestate.balls[ball_index], &gamestate.players[Host]);
        check_ball_player_collision(&gamestate.balls[ball_index], &gamestate.players[Client]);

        G8RTOS_SignalSemaphore(&gamestateMutex); // done accessing gamestate
        sleep(35); // go to bed
    }
}


/*
 * End of game for the host
 */
void EndOfGameHost()
{
    //wait for these to be free
    G8RTOS_WaitSemaphore(&ipMutex);
    G8RTOS_WaitSemaphore(&LCDMutex);
    G8RTOS_WaitSemaphore(&gamestateMutex);

    G8RTOS_KillAllThreads(); // kills all but currently running thread

    //re init semaphores
    G8RTOS_InitSemaphore(&ipMutex,1);
    G8RTOS_InitSemaphore(&LCDMutex,1);
    G8RTOS_InitSemaphore(&gamestateMutex,1);
    G8RTOS_InitSemaphore(&LEDMutex,0); // counting semaphore to let the LEDMove thread happen

    if(gamestate.winner == Host)
    {
        LCD_Clear(gamestate.players[Host].color);
        LCD_Text(10,10,"Host Win. Host press B0 to play again",LCD_GRAY);
    }
    else
    {
        LCD_Clear(gamestate.players[Client].color);
        LCD_Text(10,10,"Client Win. Host press B0 to play again",LCD_GRAY);
    }
    //kill all balls
    for(int i=0;i<MAX_NUM_OF_BALLS;i++)
    {
        gamestate.balls[i].alive = false; // kill every ball to "reset"
    }


    //for aperiodic event
    start_again=false;
    P4->IFG &= ~BIT4;
    P4->IE |= BIT4;
    G8RTOS_AddAPeriodicEvent(&StartGameAgain, 2, PORT4_IRQn);

    while(!start_again); // wait for the button press [FLAG]
    gamestate.gameDone = false;

    gamestate.players[Client].color = LCD_BLUE;
    gamestate.players[Client].currentCenter = PADDLE_X_CENTER;
    gamestate.players[Client].position = TOP;
    gamestate.players[Host].color = LCD_RED;
    gamestate.players[Host].currentCenter = PADDLE_X_CENTER;
    gamestate.players[Host].position = BOTTOM;
    gamestate.player[Client].ready = 0;

    while(gamestate.player[Client].ready != 1)
    {
    ReceiveData(&gamestate.player[Client].ready, sizeof(gamestate.player[Client].ready)); // receive client info
    }
    SendData(&gamestate,johnny_laptop_ip,sizeof(gamestate));

    LCD_Clear(LCD_BLACK);
    InitBoardState();
    G8RTOS_AddThread(&ReadJoystickHost, 5, "RD_JOYSTICK");
    G8RTOS_AddThread(&GenerateBall, 6, "GENBALL");
    G8RTOS_AddThread(&SendDataToClient, 1, "IP_DAT_TX");
    G8RTOS_AddThread(&ReceiveDataFromClient, 2, "IP_DAT_RX");
    G8RTOS_AddThread(&DrawObjects,3,"DRAW_OBJ");
    G8RTOS_AddThread(&MoveLEDs, 254, "MOVELEDs");
    G8RTOS_AddThread(&IdleThread, 255, "IDLE");
    G8RTOS_KillSelf();

    while(1); // this shouldnt happen

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
        G8RTOS_WaitSemaphore(&gamestateMutex);

        //update players
        UpdatePlayerOnScreen(&prevHost, &gamestate.players[Host]);
        UpdatePlayerOnScreen(&prevClient, &gamestate.players[Client]);

        //update all the balls
        for(int i=0; i<MAX_NUM_OF_BALLS; i++)
        {
            if(gamestate.balls[i].alive)
            {
                //wait until all previous and current game state variables are ready to be modified
                G8RTOS_WaitSemaphore(&gamestateMutex);
                UpdateBallOnScreen(&prev_balls[i], &gamestate.balls[i], gamestate.balls[i].color);
                G8RTOS_SignalSemaphore(&gamestateMutex);
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

        G8RTOS_SignalSemaphore(&gamestateMutex);
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
        G8RTOS_WaitSemaphore(&LEDMutex); // it will wait until the scores are updateable. [thread will be blocked]
        //then update LEDs
        LP3943_LEDModeSet(RED, gamestate.LEDScores[Host]);
        LP3943_LEDModeSet(BLUE, gamestate.LEDScores[Client]);
    }
}

void StartGameAgain(void)
{
    P4->IFG &= ~BIT0; // must clear IFG flag

    start_again = true;


    // rest of ISR
    P4->IE &= ~BIT0; // disable the interrupt
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
                          BOTTOM_PADDLE_EDGE, ARENA_MAX_Y-1, player->color);
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
        current_player.currentCenter = gamestate.players[Host].currentCenter;
    }
    else // or client
    {
        y_start = ARENA_MIN_Y;
        y_end = TOP_PADDLE_EDGE;
        current_player.currentCenter = gamestate.players[Client].currentCenter;
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
    //DRAW ARENA WALLS [MIN 38 39 40] WIDTH=3
    LCD_DrawRectangle(ARENA_MIN_X-2, ARENA_MIN_X, ARENA_MIN_Y, ARENA_MAX_Y-1, LCD_WHITE);
    LCD_DrawRectangle(ARENA_MAX_X, ARENA_MAX_X+2, ARENA_MIN_Y, ARENA_MAX_Y-1, LCD_WHITE);


    // resets game state vars & draws new game
    //reset LED vals and winner
    gamestate.LEDScores[Host] = 0;
    gamestate.LEDScores[Client] = 0;
    gamestate.overallScores[Host] = 0;
    gamestate.overallScores[Client] = 0;
    gamestate.winner = false;
    //redraw players
    DrawPlayer(&gamestate.players[Host]);
    DrawPlayer(&gamestate.players[Client]);

    //print the scores
    snprintf(str,64,"%d",gamestate.overallScores[Host]);
    LCD_Text(0, 220, str, LCD_RED); // host is on the bottom
    snprintf(str,64,"%d",gamestate.overallScores[Client]);
    LCD_Text(0,0,str,LCD_BLUE); // client is on the top
}

/*
 *minkowski algorithm to check if a ball needs to bounce off a wall
 *all it does is flip the xvelocity if it collides w/ the wall.
 */
void check_ball_wall_collision(Ball_t *checkball)
{
    //  minkowski algorithm
    int32_t w = 0.5*(3 + BALL_SIZE + 2*WIGGLE_ROOM); // wall width is 3
    int32_t h = 0.5*(ARENA_MAX_Y + BALL_SIZE + 2*WIGGLE_ROOM);
    int32_t dx;
    int32_t dy = (MAX_SCREEN_Y >> 1) - checkball->currentCenterY; // wall y center are the same

    //before assigning dx, where are two walls it can collide with.
    // if its on the left half of the arena
    if(checkball->currentCenterX <= PADDLE_X_CENTER)    dx = ARENA_MIN_X - checkball->currentCenterX;
    // otherwise its on the right half of the arena
    else    dx = ARENA_MAX_X - checkball->currentCenterX;

    if( (abs(dx)<=w) && (abs(dy)) )
    {
        int32_t wy = w*dy;
        int32_t hx = h*dx;
        if( wy>hx )
        {
            //top collision
            if( wy>-hx )    checkball->Xvel *= -1; // flip xvelocity
            // left collision
            else    checkball->Xvel *= -1;
        }
        else
        {
            //right collision
            if(wy > -hx)    checkball->Xvel *= -1;
            // bottom collision
            else    checkball->Xvel *= -1;
        }
    }

}

/*
 *minkowski algorithm to check if the player collides with a ball
 *checks if ball collides with a player
 *updates velocities
 *updates colors
 */
void check_ball_player_collision(Ball_t *checkball, GeneralPlayerInfo_t *checkplayer)
{
    int32_t w = 0.5*(PADDLE_LEN + (WIGGLE_ROOM+2));
    int32_t h = 0.5*(ARENA_MAX_Y + BALL_SIZE + (WIGGLE_ROOM+2));
    int32_t dx, dy;
    //the paddle is 64 units long, 64/3 = 21.33, middle seg is CENTER+/-11[ceil]
    int32_t Lpaddleboundary, Rpaddleboundary;
    Lpaddleboundary = checkplayer->currentCenter - 11;
    Rpaddleboundary = checkplayer->currentCenter + 11;

    dx = checkplayer->currentCenter - checkball->currentCenterX;

    //top and bottom players have different dy
    if(checkplayer->position == TOP)    dy = (TOP_PADDLE_EDGE + PADDLE_WID_D2) - checkball->currentCenterY;
    else    dy = (BOTTOM_PADDLE_EDGE - PADDLE_WID_D2) - checkball->currentCenterY;

    collision_detected = false;

    if(abs(dx) <= w && abs(dy) <= h)
    {
        //collision occured
        collision_detected = true;
        int32_t wy = w*dy;
        int32_t hx = h*dx;

        //top collision cares about paddle. bottom collision cares about paddle
        //left and right know where they're going

        if(wy > hx)//CHECK TOP AND RIGHT COLLISIONS
        {
            if(wy > -hx) // top collision
            {
                if(checkball->currentCenterX >= Lpaddleboundary &&
                        checkball->currentCenterX <= Rpaddleboundary)
                    checkball->Xvel = 0; // if in between two boundaries, its middle, shoot straight
                //if its to the left of the middle +/-11, it goes left or right.
                else if(checkball->currentCenterX <= Lpaddleboundary)
                {
                    if(checkball->Xvel >= 0) // ONLY invert the xvel if it was travelling straight or right
                        checkball->Xvel = (-1)*(SystemTime % MAX_BALL_SPEED + 1);
                }
                //if its here, it's on the right segment
                else if(checkball->Xvel <= 0) // ONLY invert xvel if it was travelling left or straight
                        checkball->Xvel = (-1)*(SystemTime % MAX_BALL_SPEED + 1);

                checkball->Yvel *= -1; // invert y velocity to change dir
                if(checkball->Yvel < 0) checkball->Yvel -= 1; // if the yvelocity is now neg, make it faster in that dir
                else    checkball->Yvel += 1; // otherwise make it faster in the pos dir
                //clip yvel
                if(checkball->Yvel < -MAX_BALL_SPEED)   checkball->Yvel = -MAX_BALL_SPEED;
                if(checkball->Yvel > MAX_BALL_SPEED)    checkball->Yvel = MAX_BALL_SPEED;
            }
            else // left collision
            {
                checkball->Xvel = (SystemTime % MAX_BALL_SPEED + 1); //go towards the right
                checkball->Yvel *= -1; // flip yvel
                if(checkball->Yvel < 0) checkball->Yvel -= 1; //    faster in neg direction
                else    checkball->Yvel += 1;   //  faster in pos dir
                //clip yvel
                if(checkball->Yvel < -MAX_BALL_SPEED)   checkball->Yvel = -MAX_BALL_SPEED;
                if(checkball->Yvel > MAX_BALL_SPEED)    checkball->Yvel = MAX_BALL_SPEED;
            }
        }
        else//CHECK BOTTOM AND LEFT COLLISIONS
        {
            if(wy > -hx) // right collision
            {
                checkball->Xvel = (-1)*(SystemTime % MAX_BALL_SPEED + 1); //go towards the right
                checkball->Yvel *= -1; // flip yvel
                if(checkball->Yvel < 0) checkball->Yvel -= 1; //    faster in neg direction
                else    checkball->Yvel += 1;   //  faster in pos dir
                //clip yvel
                if(checkball->Yvel < -MAX_BALL_SPEED)   checkball->Yvel = -MAX_BALL_SPEED;
                if(checkball->Yvel > MAX_BALL_SPEED)    checkball->Yvel = MAX_BALL_SPEED;
            }
            else // bottom collision
            {
                if(checkball->currentCenterX >= Lpaddleboundary &&
                        checkball->currentCenterX <= Rpaddleboundary)
                    checkball->Xvel = 0; // if in between two boundaries, its middle, shoot straight
                //if its to the left of the middle +/-11, it goes left or right.
                else if(checkball->currentCenterX <= Lpaddleboundary)
                {
                    if(checkball->Xvel >= 0) // ONLY invert the xvel if it was travelling straight or right
                        checkball->Xvel = (-1)*(SystemTime % MAX_BALL_SPEED + 1);
                }
                //if its here, it's on the right segment
                else if(checkball->Xvel <= 0) // ONLY invert xvel if it was travelling left or straight
                        checkball->Xvel = (-1)*(SystemTime % MAX_BALL_SPEED + 1);

                checkball->Yvel *= -1; // invert y velocity to change dir
                if(checkball->Yvel < 0) checkball->Yvel -= 1; // if the yvelocity is now neg, make it faster in that dir
                else    checkball->Yvel += 1; // otherwise make it faster in the pos dir
                //clip yvel
                if(checkball->Yvel < -MAX_BALL_SPEED)   checkball->Yvel = -MAX_BALL_SPEED;
                if(checkball->Yvel > MAX_BALL_SPEED)    checkball->Yvel = MAX_BALL_SPEED;
            }
        }
    }

    //adjust color
    if(collision_detected)  checkball->color = checkplayer->color;
    collision_detected = false;
}

/*********************************************** Public Functions *********************************************************************/
