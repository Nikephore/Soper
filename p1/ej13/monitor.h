/**
 * @file monitor.h
 * @author Luis Miguel Nucifora & Alexis [APELLIDO].
 * @brief Monitor the activity of the miners.
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "pow.h"

/**
 * @brief Verify the solutions found by Miner process
 *
 * @param n_cycles  Number of rounds to mine.
 * @param n_threads Number of threads that will search for the target.
 * @param target    Target of the POW function.
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
void monitor();