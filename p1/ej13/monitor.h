/**
 * @file monitor.h
 * @author Luis Miguel Nucifora & Alexis Canales Molina.
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
 * @param send      Pipe to send information to the miner.
 * @param recieve   Pipe to recieve information from the miner.
 * @param target    First target of the miner.
 * @return EXIT_SUCCESS or EXIT_FAILURE
 */
void monitor(int send, int recieve, int target);
