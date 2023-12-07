/**************************************************************
 * @file lfbb.h
 * @brief A bipartite buffer implementation written in standard
 * c11 suitable for both low-end microcontrollers all the way
 * to HPC machines. Lock-free for single consumer single
 * producer scenarios.
 * @version 1.3.0
 * @date 21. September 2022
 * @author Djordje Nedic
 **************************************************************/

/**************************************************************
 * Copyright (c) 2022 Djordje Nedic
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall
 * be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of LFBB - Lock Free Bipartite Buffer
 *
 * Author:          Djordje Nedic <nedic.djordje2@gmail.com>
 * Version:         1.3.0
 **************************************************************/

/************************** INCLUDE ***************************/
#ifndef LFBB_H
#define LFBB_H

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#ifndef __cplusplus
#include <stdalign.h>
#include <stdatomic.h>
#include <stdbool.h>
#else
#include <atomic>
#define atomic_size_t std::atomic_size_t
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/************************** DEFINE ****************************/

#ifndef LFBB_MULTICORE_HOSTED
#define LFBB_MULTICORE_HOSTED true
#endif

#ifndef LFBB_CACHELINE_LENGTH
#define LFBB_CACHELINE_LENGTH 64U
#endif

/*************************** TYPES ****************************/

typedef struct {
#if LFBB_MULTICORE_HOSTED
    alignas(LFBB_CACHELINE_LENGTH) atomic_size_t r; /**< Read index */
    alignas(LFBB_CACHELINE_LENGTH) atomic_size_t w; /**< Write index */
    alignas(LFBB_CACHELINE_LENGTH)
        atomic_size_t i; /**< Invalidated space index */
#else
    atomic_size_t r; /**< Read index */
    atomic_size_t w; /**< Write index */
    atomic_size_t i; /**< Invalidated space index */
#endif
    size_t size;        /**< Size of the data array */
    uint8_t *data;      /**< Pointer to the data array */
    bool write_wrapped; /**< Write wrapped flag, used only in the producer */
    bool read_wrapped;  /**< Read wrapped flag, used only in the consumer */
} LFBB_Inst_Type;

/******************** FUNCTION PROTOTYPES *********************/

/**
 * @brief Initializes a bipartite buffer instance
 * @param[in] Instance pointer
 * @param[in] Data array pointer
 * @param[in] Size of data array
 * @retval None
 */
void LFBB_Init(LFBB_Inst_Type *inst, uint8_t *data_array, size_t size);

/**
 * @brief Acquires a linear region in the bipartite buffer for writing
 * @param[in] Instance pointer
 * @param[in] Free linear space in the buffer required
 * @retval Pointer to the beginning of the linear space
 */
uint8_t *LFBB_WriteAcquire(LFBB_Inst_Type *inst, size_t free_required);

/**
 * @brief Releases the bipartite buffer after a write
 * @param[in] Instance pointer
 * @param[in] Bytes written to the linear space
 * @retval None
 */
void LFBB_WriteRelease(LFBB_Inst_Type *inst, size_t written);

/**
 * @brief Acquires a linear region in the bipartite buffer for reading
 * @param[in] Instance pointer
 * @param[out] Available linear data in the buffer
 * @retval Pointer to the beginning of the data
 */
uint8_t *LFBB_ReadAcquire(LFBB_Inst_Type *inst, size_t *available);

/**
 * @brief Releases the bipartite buffer after a read
 * @param[in] Instance pointer
 * @param[in] Bytes read from the linear region
 * @retval None
 */
void LFBB_ReadRelease(LFBB_Inst_Type *inst, size_t read);

/*************************** MACRO ****************************/

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

/******************** FUNCTION PROTOTYPES *********************/

static size_t CalcFree(size_t w, size_t r, size_t size);

/******************** EXPORTED FUNCTIONS **********************/

void LFBB_Init(LFBB_Inst_Type *inst, uint8_t *data_array, const size_t size) {
    assert(inst != NULL);
    assert(data_array != NULL);
    assert(size != 0U);

    inst->data = data_array;
    inst->size = size;
    inst->r = 0U;
    inst->w = 0U;
    inst->i = 0U;
    inst->write_wrapped = false;
    inst->read_wrapped = false;
}

uint8_t *LFBB_WriteAcquire(LFBB_Inst_Type *inst, const size_t free_required) {
    assert(inst != NULL);
    assert(inst->data != NULL);

    /* Preload variables with adequate memory ordering */
    const size_t w = atomic_load_explicit(&inst->w, memory_order_relaxed);
    const size_t r = atomic_load_explicit(&inst->r, memory_order_acquire);
    const size_t size = inst->size;

    const size_t free = CalcFree(w, r, size);
    const size_t linear_space = size - r;
    const size_t linear_free = MIN(free, linear_space);

    /* Try to find enough linear space until the end of the buffer */
    if (free_required <= linear_free) {
        return &inst->data[w];
    }

    /* If that doesn't work try from the beginning of the buffer */
    if (free_required <= free - linear_free) {
        inst->write_wrapped = true;
        return &inst->data[0];
    }

    /* Could not find free linear space with required size */
    return NULL;
}

void LFBB_WriteRelease(LFBB_Inst_Type *inst, const size_t written) {
    assert(inst != NULL);
    assert(inst->data != NULL);

    /* Preload variables with adequate memory ordering */
    size_t w = atomic_load_explicit(&inst->w, memory_order_relaxed);
    size_t i = atomic_load_explicit(&inst->i, memory_order_relaxed);

    /* If the write wrapped set the invalidate index and reset write index*/
    if (inst->write_wrapped) {
        inst->write_wrapped = false;
        i = w;
        w = 0U;
    }

    /* Increment the write index and wrap to 0 if needed */
    w += written;
    if (w == inst->size) {
        w = 0U;
    }

    /* If we wrote over invalidated parts of the buffer move the invalidate
     * index
     */
    if (w > i) {
        i = w;
    }

    /* Store the indexes with adequate memory ordering */
    atomic_store_explicit(&inst->i, i, memory_order_release);
    atomic_store_explicit(&inst->w, w, memory_order_release);
}

uint8_t *LFBB_ReadAcquire(LFBB_Inst_Type *inst, size_t *available) {
    assert(inst != NULL);
    assert(inst->data != NULL);
    assert(available != NULL);

    /* Preload variables with adequate memory ordering */
    const size_t w = atomic_load_explicit(&inst->w, memory_order_acquire);
    const size_t i = atomic_load_explicit(&inst->i, memory_order_acquire);
    const size_t r = atomic_load_explicit(&inst->r, memory_order_relaxed);

    /* When read and write indexes are equal, the buffer is empty */
    if (r == w) {
        *available = 0;
        return NULL;
    }

    /* Simplest case, read index is behind the write index */
    if (r < w) {
        *available = w - r;
        return &inst->data[r];
    }

    /* Read index reached the invalidate index, make the read wrap */
    if (r == i) {
        inst->read_wrapped = true;
        *available = w;
        return &inst->data[0];
    }

    /* There is some data until the invalidate index */
    *available = i - r;
    return &inst->data[r];
}

void LFBB_ReadRelease(LFBB_Inst_Type *inst, const size_t read) {
    assert(inst != NULL);
    assert(inst->data != NULL);

    /* Preload variables with adequate memory ordering */
    size_t r = atomic_load_explicit(&inst->r, memory_order_relaxed);

    /* If the read wrapped, overflow the read index */
    if (inst->read_wrapped) {
        inst->read_wrapped = false;
        r = 0U;
    }

    /* Increment the read index and wrap to 0 if needed */
    r += read;
    if (r == inst->size) {
        r = 0U;
    }

    /* Store the indexes with adequate memory ordering */
    atomic_store_explicit(&inst->r, r, memory_order_release);
}

/********************* PRIVATE FUNCTIONS **********************/

static size_t CalcFree(const size_t w, const size_t r, const size_t size) {
    if (r > w) {
        return (r - w) - 1U;
    } else {
        return (size - (w - r)) - 1U;
    }
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LFBB_H */
