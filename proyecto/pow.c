/**
 * @file pow.c
 * @author SOPER teaching team.
 * @brief Computation of the POW.
 * @version 1.5
 * @date 2023-02-02
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "pow.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define PRIME POW_LIMIT

#define BIG_X 435679812
#define BIG_Y 100001819

long int pow_hash(long int x)
{
  long int result = (x * BIG_X + BIG_Y) % PRIME;
  return result;
}

