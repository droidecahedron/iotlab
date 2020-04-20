/*
 * G8RTOS_Semaphores.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include <stdint.h>
#include "msp.h"
#include "G8RTOS_Semaphores.h"
#include "G8RTOS_CriticalSection.h"
#include "G8RTOS_Structures.h"

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes a semaphore to a given value
 * Param "s": Pointer to semaphore
 * Param "value": Value to initialize semaphore to
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_InitSemaphore(semaphore_t *s, int32_t value)
{
    int32_t crit = StartCriticalSection();
    *s = value;
    EndCriticalSection(crit);
}

/*
 * No longer waits for semaphore
 *  - Decrements semaphore
 *  - Blocks thread is sempahore is unavalible
 * Param "s": Pointer to semaphore to wait on
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_WaitSemaphore(semaphore_t *s)
{
    int32_t crit = StartCriticalSection();
    (*s)--; // pulling it to say that we want to use (grabbing)


    if((*s) < 0) // check if the semaphore is already in use (has not been released by signal)
    {
        CurrentlyRunningThread->blocked = s; // blocks it so two threads can't use this semaphore


        //yield to allow another thread to run
        //StartContextSwitch();
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; // [relinquishing control]


        // guarantee we stay in this block
        EndCriticalSection(crit);

    }
    else // otherwise we aren't negative yet, we end the critical section. nothing is trying to grab the semaphore
    {
        EndCriticalSection(crit);
    }
}

/*
 * Signals the completion of the usage of a semaphore
 *  - Increments the semaphore value by 1
 *  - Unblocks any threads waiting on that semaphore
 * Param "s": Pointer to semaphore to be signaled
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_SignalSemaphore(semaphore_t *s)
{
    int32_t crit = StartCriticalSection();
    (*s)++; // increment semaphore to release control

    if((*s) <= 0) // check to see "who goes next"
    {
        tcb_t* ptr = CurrentlyRunningThread->Next_TCB;
        while(ptr->blocked != s) // look to see who's blocked
        {
            ptr = ptr->Next_TCB; // move on to next control block [keep looping through]
        }

        ptr->blocked = 0; // then unblock the semaphore for that thread to give them a turn
    }

    EndCriticalSection(crit);
}

/*********************************************** Public Functions *********************************************************************/


