/**
 * @file minero.h
 * @author Luis Miguel Nucifora & Alexis [APELLIDO].
 * @brief Manage the threads and find the target of the POW function.
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "pow.h"

typedef struct
{
  int min;
  int max;
  long int target;
  int solution;
} search;

/**
 * @brief Execute all the rounds of a miner process
 *
 * @param n_cycles  Number of rounds to mine.
 * @param n_threads Number of threads that will search for the target.
 * @param target    Target of the POW function.
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
int miner(int n_cycles, int n_threads, int target);