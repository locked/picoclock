/* 
 * File:   circularBuffer.h
 * Author: Pascal-Emmanuel Lachance
 *         https://github.com/Raesangur/
 *
 * Created on 1 août 2019, 20:49
 */
#pragma once
#include <stdint.h>

/*****************************************************************************/
/* Typedefs */

typedef struct
{
    uint16_t num;
    uint8_t size;
    void* ptr;
    void* maxPtr;
    void* buffer;
}circularBuffer_t;

/*****************************************************************************/
/* Function declarations */

circularBuffer_t* circularBuffer_create(circularBuffer_t* buff, uint16_t number, uint8_t size);
void circularBuffer_delete(circularBuffer_t* buff);

void circularBuffer_insert(circularBuffer_t* buff, void* data);
void* circularBuffer_current(circularBuffer_t* buff);
void* circularBuffer_getElement(circularBuffer_t* buff, uint16_t num);
