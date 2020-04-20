/*
 * G8RTOS_Scheduler.c
 */

/*********************************************** Dependencies and Externs *************************************************************/


#include "G8RTOS_Scheduler.h"
#include <stdint.h>
#include "msp.h"


#include "G8RTOS_Structures.h"
#include "BSP.h"
#include <stdbool.h>



#define ICSR (*((volatile unsigned int*)(0xe000ed04)))
#define ICSR_PENDSVET (1 << 28);
#define STCSR (*((volatile unsigned int*)(0xe00e010)))
#define STCSR_COUNTFLAG (1 << 16)
static volatile unsigned int ticks_count;

/*
 * G8RTOS_Start exists in asm
 */
extern void G8RTOS_Start();

/* System Core Clock From system_msp432p401r.c */
extern uint32_t SystemCoreClock;

/*
 * Pointer to the currently running Thread Control Block
 */
extern tcb_t * CurrentlyRunningThread;

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Defines ******************************************************************************/

/* Status Register with the Thumb-bit Set */
#define THUMBBIT 0x01000000

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/* Thread Control Blocks
 *	- An array of thread control blocks to hold pertinent information for each thread
 */
tcb_t threadControlBlocks[MAX_THREADS];

/* Thread Stacks
 *	- An array of arrays that will act as invdividual stacks for each thread
 */
int32_t threadStacks[MAX_THREADS][STACKSIZE];

/* Periodic Event Threads
 * - An array of periodic events to hold pertinent information for each thread
 */
ptcb_t Pthread[MAX_PTHREADS];

/*
 * static ID counter for assigning ID's
 *
 */
static uint16_t IDCounter;

/*********************************************** Data Structures Used *****************************************************************/


/*********************************************** Private Variables ********************************************************************/

/*
 * Current Number of Threads currently in the scheduler
 */
static uint32_t NumberOfThreads;

/*
 * Current Number of Periodic Threads currently in the scheduler
 */
static uint32_t NumberOfPthreads;

/*********************************************** Private Variables ********************************************************************/


/*********************************************** Private Functions ********************************************************************/

/*
 * Initializes the Systick and Systick Interrupt
 * The Systick interrupt will be responsible for starting a context switch between threads
 * Param "numCycles": Number of cycles for each systick interrupt
 */
static void InitSysTick(uint32_t numCycles)
{
    SysTick_Config(numCycles);
    SysTick_enableInterrupt();
}

/*
 * Chooses the next thread to run.
 * Look for first unblocked thread
 */
void G8RTOS_Scheduler()
{
    uint8_t nextThreadPriority = 255; // set as max

    tempNextThread = CurrentlyRunningThread->Next_TCB;

    //its not lower / higher priority, dont update the thread to run.
    for(int i=0;i<NumberOfThreads;i++)
    {
        if(!(tempNextThread->Asleep == true) && !(tempNextThread->blocked))
        {
            if(tempNextThread->priority < nextThreadPriority)
            {
            CurrentlyRunningThread = tempNextThread;
            nextThreadPriority = CurrentlyRunningThread->priority;
            }
        }
        tempNextThread = tempNextThread->Next_TCB;
    }


}


/*
 * SysTick Handler
 * The Systick Handler now will increment the system time,
 * set the PendSV flag to start the scheduler,
 * and be responsible for handling sleeping and periodic threads
 */
void SysTick_Handler()
{

            SystemTime++;
            //periodic stuff
            ptcb_t* periodic_ptr = &Pthread[0];
            for(int i=0; i<NumberOfPthreads; i++, periodic_ptr = periodic_ptr->Next_PTCB) // moves to next PTCB at the end always.
            {
                if(periodic_ptr->Execute_Time == SystemTime)
                {
                    periodic_ptr->Execute_Time = periodic_ptr->Period + SystemTime;
                    (*periodic_ptr->Handler)();     //  run Pthread
                }

            }
            //end of periodic stuff


            tcb_t* ptr = CurrentlyRunningThread;
            for(int i=0;i < NumberOfThreads;i++,ptr = ptr->Next_TCB) // moves to next TCB
            {
                if((ptr->Asleep == true) && (ptr->Sleep_Count==SystemTime) ) // if the thread is asleep and its done sleeping
                {
                    ptr->Asleep = false; // wake the thread up
                    ptr->Sleep_Count = 0; // reset the sleep count
                }
            }

            ICSR |= ICSR_PENDSVET;

}

/*********************************************** Private Functions ********************************************************************/


/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
uint32_t SystemTime;

/*********************************************** Public Variables *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Sets variables to an initial state (system time and number of threads)
 * Enables board for highest speed clock and disables watchdog
 */
void G8RTOS_Init()
{
    SystemTime = 0;     //Set system time to 0
    NumberOfThreads = 0;    //Set # of threads to 0
    BSP_InitBoard();        //Initialize board with max frequency

    //ISR's
    // Relocate vector table to SRAM to use aperiodic events
    uint32_t newVTORTable = 0x20000000;
    memcpy((uint32_t *)newVTORTable, (uint32_t *)SCB->VTOR, 57*4);
    // 57 interrupt vectors to copy
    SCB->VTOR = newVTORTable;
}

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes the Systick
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
int G8RTOS_Launch()
{
    CurrentlyRunningThread = &threadControlBlocks[0];

    tempNextThread = CurrentlyRunningThread->Next_TCB;

    for(int threadcounter=0;threadcounter<NumberOfThreads-1;threadcounter++)
    {
        if(tempNextThread->priority < CurrentlyRunningThread->priority)
        {
            CurrentlyRunningThread = tempNextThread;
        }
        else
        {
            tempNextThread = tempNextThread->Next_TCB;
        }
    }

    CurrentlyRunningThread = tempNextThread;


    InitSysTick(ClockSys_GetSysFreq()/1000);     //Initialize Systick

    NVIC_SetPriority (SysTick_IRQn,6);
    NVIC_SetPriority (PendSV_IRQn,7);

    G8RTOS_Start();         //Call G8RTOS_Start

    return 0;
}


/*
 * Adds threads to G8RTOS Scheduler
 * 	- Checks if there are stil available threads to insert to scheduler
 * 	- Initializes the thread control block for the provided thread
 * 	- Initializes the stack for the provided thread to hold a "fake context"
 * 	- Sets stack tcb stack pointer to top of thread stack
 * 	- Sets up the next and previous tcb pointers in a round robin fashion
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
int G8RTOS_AddThread(void (*threadToAdd)(void),uint8_t priority,char * threadname)
{
    //status bit
    int32_t crit = StartCriticalSection(); // so we can add threads while the os is running

    if(NumberOfThreads == MAX_THREADS){
        EndCriticalSection(crit);
        return THREAD_LIMIT_REACHED;
    }

    for(int i=0;i<MAX_THREADS;i++)
    {
        if(!(threadControlBlocks[i].isAlive)) // isalive defaults to false so this works for the first thread.
            if(i==0) // if at the start of the tcb
            {
                //if it is the first thread, it points to itself
                threadControlBlocks[i].Next_TCB = &threadControlBlocks[i];
                threadControlBlocks[i].Prev_TCB = &threadControlBlocks[i];

                threadControlBlocks[i].SP = &threadStacks[i][STACKSIZE-16];

                threadControlBlocks[i].Asleep = false; // has to be awake when we add it
                threadControlBlocks[i].Sleep_Count = 0; // its not sleeping so no sleep count.
                threadControlBlocks[i].isAlive = true; // has to be alive
                threadControlBlocks[i].blocked = 0; // ensures the thread is not blocked on addition
                threadControlBlocks[i].threadID = ((IDCounter++) << 16) | (threadId_t)(&threadControlBlocks[i]); // unique thread ID
                //NumberOfthreads is either my dead thread index, or what it was previously.
                threadStacks[i][STACKSIZE-1] = THUMBBIT;        //THUMBBIT goes to PSR value
                threadStacks[i][STACKSIZE-2] = (uint32_t)(threadToAdd);  //PC value apparently
                //now we are adding priority and threadname fields when adding a thread
                threadControlBlocks[i].priority = priority;
                int p=0;
                while(*threadname)
                {
                    threadControlBlocks[i].threadName[p] = *(threadname);
                    *(threadname)++;
                    p++;
                }
                NumberOfThreads++;
                EndCriticalSection(crit);
                return NO_ERROR;
            }
            else
            {

                threadControlBlocks[i].Prev_TCB = &threadControlBlocks[i-1]; // new thread's prev points to previous element in tcb chain
                threadControlBlocks[i].Next_TCB = threadControlBlocks[i].Prev_TCB->Next_TCB; // new thread's next is now the previous' next

                threadControlBlocks[i].Next_TCB->Prev_TCB = &threadControlBlocks[i]; // the next's previous is now the new thread [insertion]
                threadControlBlocks[i].Prev_TCB->Next_TCB = &threadControlBlocks[i]; // the previous' next is now the new thread [insertion]

                threadControlBlocks[i].SP = &threadStacks[i][STACKSIZE-16];

                threadControlBlocks[i].Asleep = false; // has to be awake when we add it
                threadControlBlocks[i].Sleep_Count = 0; // its not sleeping so no sleep count.
                threadControlBlocks[i].isAlive = true; // has to be alive
                threadControlBlocks[i].blocked = 0; // ensures the thread is not blocked on addition
                threadControlBlocks[i].threadID = ((IDCounter++) << 16) | (threadId_t)(&threadControlBlocks[i]); // unique thread ID
                //NumberOfthreads is either my dead thread index, or what it was previously.
                threadStacks[i][STACKSIZE-1] = THUMBBIT;        //THUMBBIT goes to PSR value
                threadStacks[i][STACKSIZE-2] = (uint32_t)(threadToAdd);  //PC value apparently
                //now we are adding priority and threadname fields when adding a thread
                threadControlBlocks[i].priority = priority;
                int p=0;
                while(*threadname)
                {
                    threadControlBlocks[i].threadName[p] = *(threadname);
                    *(threadname)++;
                    p++;
                }
                NumberOfThreads++;
                EndCriticalSection(crit);
                return NO_ERROR;
            }
    }

    //threads incorrectly alive

    EndCriticalSection(crit);
    return THREADS_INCORRECTLY_ALIVE;

}







/*
 * Adds periodic threads to G8RTOS Scheduler
 * Function will initialize a periodic event struct to represent event.
 * The struct will be added to a linked list of periodic events
 * Param Pthread To Add: void-void function for P thread handler
 * Param period: period of P thread to add
 * Returns: Error code for adding threads
 */
int G8RTOS_AddPeriodicEvent(void (*PthreadToAdd)(void), uint32_t period)
{
    int32_t crit = StartCriticalSection(); // these can run while Os is on
    if(NumberOfPthreads == MAX_PTHREADS){
        return -1;
    }

    Pthread[NumberOfPthreads].Current_Time = SystemTime;
    Pthread[NumberOfPthreads].Execute_Time = SystemTime + 1; // You want it to run for at least 1 ms.
    Pthread[NumberOfPthreads].Period = period;
    Pthread[NumberOfPthreads].Handler = PthreadToAdd;
    NumberOfPthreads++; // if you increment any sooner you'll have to start at 1 and waste memory

    for(int ii = 0; ii < NumberOfPthreads; ii++){
        Pthread[ii].Next_PTCB = &Pthread[(ii+1)%(NumberOfPthreads)];    //Set next TCB
        Pthread[ii].Prev_PTCB = &Pthread[(NumberOfPthreads + ii-1)%(NumberOfPthreads)];    //Set prev TCB
    }
    EndCriticalSection(crit);
    return 0;

}



/*
 *
 * ADD APERIODIC EVENT!!!
 *
 */


sched_ErrCode_t G8RTOS_AddAPeriodicEvent(void(*AthreadToAdd)(void), uint8_t priority,IRQn_Type IRQn)
{
    int32_t crit = StartCriticalSection();
    if(IRQn>PSS_IRQn && IRQn < PORT6_IRQn)
    {
        if(priority>6)
        {
            EndCriticalSection(crit);
            return HWI_PRIORITY_INVALID;

        }
        else
        {
            __NVIC_SetVector(IRQn,(uint32_t)(AthreadToAdd));
            __NVIC_SetPriority(IRQn,priority);
            __NVIC_EnableIRQ(IRQn); // no underscores?
            EndCriticalSection(crit);
            return NO_ERROR;
        }
    }
    else
    {
        EndCriticalSection(crit);
        return IRQn_INVALID;
    }
}

//kill self
sched_ErrCode_t G8RTOS_KillSelf(void)
{
    int32_t crit = StartCriticalSection();
    if(NumberOfThreads==1) // cannot kill the last thread
    {
        return CANNOT_KILL_LAST_THREAD;
    }

    CurrentlyRunningThread->isAlive = false;
    tcb_t * TempPrev, * TempNext;
    TempNext = CurrentlyRunningThread->Next_TCB;
    TempPrev = CurrentlyRunningThread->Prev_TCB;
    TempNext->Prev_TCB = TempPrev;
    TempPrev->Next_TCB = TempNext;

    ICSR |= ICSR_PENDSVET;

    NumberOfThreads--;

    //this class makes me want to (function name)
    EndCriticalSection(crit);
    return NO_ERROR;
}



//kill a thread
sched_ErrCode_t G8RTOS_KillThread(threadId_t killthreadID)
{
    int32_t crit = StartCriticalSection(); // this needs to be able to run while Os is running
    if(NumberOfThreads<=1) // cannot kill the last thread
    {
        return CANNOT_KILL_LAST_THREAD;
    }

    //begin looking for a thread with a matching ID
    bool found_thread = false;
    int thread_index = 0;
    for(int i=0;i<NumberOfThreads;i++)
    {
        if(threadControlBlocks[i].threadID == killthreadID)
        {
            found_thread = true;
            thread_index = i;
        }
    }

    //if the thread wasnt found with that ID, return
    if(found_thread == false)
    {
        return THREAD_DOES_NOT_EXIST;
    }

    //if it got here, it found a thread with that ID to kill and there are more than 1 threads.
    threadControlBlocks[thread_index].isAlive = false;

    tcb_t * TempPrev, * TempNext;
    TempNext = threadControlBlocks[thread_index].Next_TCB;
    TempPrev = threadControlBlocks[thread_index].Prev_TCB;
    TempNext->Prev_TCB = TempPrev;
    TempPrev->Next_TCB = TempNext;



    NumberOfThreads--;

    if(CurrentlyRunningThread->threadID == killthreadID)
    {
        ICSR |= ICSR_PENDSVET;
    }
    EndCriticalSection(crit);
    return NO_ERROR;
}



//get thread id
threadId_t G8RTOS_GetThreadId(void)
{
    return CurrentlyRunningThread->threadID;
}


/*
 * Puts the current thread into a sleep state.
 *  param durationMS: Duration of sleep time in ms
 */
void sleep(uint32_t durationMS)
{
    CurrentlyRunningThread->Asleep = true;
    CurrentlyRunningThread->Sleep_Count = durationMS + SystemTime;
    ICSR |= ICSR_PENDSVET;

}






/*********************************************** Public Functions *********************************************************************/
