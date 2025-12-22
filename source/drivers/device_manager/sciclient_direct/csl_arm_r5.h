    /**
 * @file  csl_arm_r5.h
 *
 * @brief
 *  Header file containing various enumerations, structure definitions and function
 *  declarations for the ARM R5 IP.
 *  \par
 *  ============================================================================
 *  @n   (C) Copyright 2017-2026, Texas Instruments, Inc.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/
#ifndef CSL_ARM_R5_H_
#define CSL_ARM_R5_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \brief Get the number of sets in cache for R5 core
 *
 *  \param None
 *
 *  \return Number of sets in cache
 */
uint32_t CSL_armR5CacheGetNumSets( void );

/**
 *  \brief Get the number of ways in cache for R5 core
 *
 *  \param None
 *
 *  \return Number of ways in cache
 */
uint32_t CSL_armR5CacheGetNumWays( void );

/**
 *  \brief Clean a data cache line by set and way
 *
 *  This function is used to clean a data cache line by set and way.
 *
 *  \param set      [IN]    Indicates the cache set to clean
 *  \param way      [IN]    Indicates the cache way to clean
 *
 *  \return None
 */
void CSL_armR5CacheCleanDcacheSetWay( uint32_t set, uint32_t way );

/* @} */

#ifdef __cplusplus
}
#endif  /* extern "C" */

#endif  /* end of CSL_ARM_R5_H_ definition */

