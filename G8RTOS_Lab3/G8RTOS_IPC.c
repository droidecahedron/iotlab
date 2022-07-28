/*
 * G8RTOS_IPC.c
 *
 *  Created on: Jan 10, 2017
 *      Author: Daniel Gonzalez
 */
#include <stdint.h>
#include "msp.h"
#include "G8RTOS_IPC.h"
#include "G8RTOS_Semaphores.h"

/*********************************************** Defines ******************************************************************************/

#define FIFOSIZE 16
#define MAX_NUMBER_OF_FIFOS 4




/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/*
 * FIFO struct will hold
 *  - buffer
 *  - head
 *  - tail
 *  - lost data
 *  - current size
 *  - mutex
 */

typedef struct FIFO_t FIFO_t;

struct FIFO_t
{
    int32_t Buffer[FIFOSIZE];
    int32_t* Head;
    int32_t* Tail;
    uint32_t LostData;
    semaphore_t CurrentSize;
    semaphore_t Mutex;
};


/* Array of FIFOS */
static FIFO_t FIFOs[4];

/*********************************************** Data Structures Used *****************************************************************/

/*
 * Initializes FIFO Struct
 */
int G8RTOS_InitFIFO(uint32_t FIFOIndex)
{
    if(FIFOIndex>3) // G8RTos can only hold 4
    {
        return -1;
    }

    //if its here then its possible to add
    FIFOs[FIFOIndex].Head = &FIFOs[FIFOIndex].Buffer[0];
    FIFOs[FIFOIndex].Tail = &FIFOs[FIFOIndex].Buffer[0]; // since there's no data yet, they point to the same place. We will increment tail as we add data.
    FIFOs[FIFOIndex].LostData = 0;
    G8RTOS_InitSemaphore(&FIFOs[FIFOIndex].CurrentSize, 0);
    G8RTOS_InitSemaphore(&FIFOs[FIFOIndex].Mutex, 1);// when you wait, this gets decremented, we dont want to immediately block the thread. [wait_semaphore]

    return 0;
}

/*
 * Reads FIFO
 *  - Waits until CurrentSize semaphore is greater than zero
 *  - Gets data and increments the head ptr (wraps if necessary)
 * Param: "FIFOChoice": chooses which buffer we want to read from
 * Returns: uint32_t Data from FIFO
 */
uint32_t readFIFO(uint32_t FIFOChoice)
{
    uint32_t return_data;

    G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].CurrentSize); // block if empty, until something can write into it
    G8RTOS_WaitSemaphore(&FIFOs[FIFOChoice].Mutex);

    return_data = *(FIFOs[FIFOChoice].Head); // pull the data

    (FIFOs[FIFOChoice].Head) ++; // increment
    if( (FIFOs[FIFOChoice].Head) == &(FIFOs[FIFOChoice].Buffer[FIFOSIZE-1]) ) // check if we are at the end, and wrap
    {
        FIFOs[FIFOChoice].Head = &FIFOs[FIFOChoice].Buffer[0];
    }

    G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].Mutex); // release

    return return_data;

}

/*
 * Writes to FIFO
 *  Writes data to Tail of the buffer if the buffer is not full
 *  Increments tail (wraps if ncessary)
 *  Param "FIFOChoice": chooses which buffer we want to read from
 *        "Data': Data being put into FIFO
 *  Returns: error code for full buffer if unable to write
 */
int writeFIFO(uint32_t FIFOChoice, uint32_t Data)
{
    if(FIFOs[FIFOChoice].CurrentSize == FIFOSIZE)
    {
        FIFOs[FIFOChoice].LostData++; // error
        return -1;
    }

    *(FIFOs[FIFOChoice].Tail) = Data; // store data where the tail pointer is pointing
    (FIFOs[FIFOChoice].Tail) ++; // increment the tail pointer so we point at the correct place on next iteration
    if( FIFOs[FIFOChoice].Tail == &(FIFOs[FIFOChoice].Buffer[FIFOSIZE-1]) ) // if your tail is pointing to the end
    {
        FIFOs[FIFOChoice].Tail = &(FIFOs[FIFOChoice].Buffer[0]); // wrap around (circular buffer)
    }

    G8RTOS_SignalSemaphore(&FIFOs[FIFOChoice].CurrentSize);
    return 0;
}

