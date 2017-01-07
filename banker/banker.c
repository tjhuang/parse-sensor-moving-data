#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "banker.h"

/**
 * @brief	The current available resource record table
 */
int available[NUMBER_OF_RESOURCES];

/**
 * @brief	The resource acquisition upper-bound of each customer
 */
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

/**
 * @brief	The resource allocated of each customer
 */
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

/**
 * @brief	maximum - allocation
 */
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

/**
 * @brief	Lock of all global arrays
 */
pthread_mutex_t lock;


void show_current_resource(void)
{
	int i, j;

	pthread_mutex_lock(&lock);
	printf("\n####################################################\n");
	printf("System Available: ");
	for (i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		printf("%2d ", available[i]);
	}
	printf("\n");

	printf("                      Need          Allocated       Maximum\n");
	for (i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		printf("Customer %d:         ", i);
		for (j = 0; j < NUMBER_OF_RESOURCES; j++)
		{
			printf("%2d ", need[i][j]);
		}
		printf("       ");
		for (j = 0; j < NUMBER_OF_RESOURCES; j++)
		{
			printf("%2d ", allocation[i][j]);
		}
		printf("      ");
		for (j = 0; j < NUMBER_OF_RESOURCES; j++)
		{
			printf("%2d ", maximum[i][j]);
		}
		printf("\n");
	}
	printf("####################################################\n");
	pthread_mutex_unlock(&lock);
}

/**
 * @brief	Run the safety test algorithm
 * @return	0 is success; -1 otherwise
 */
static int safety_test(int user, int request_data[])
{
	int i, j;
	int unfinished;
	int safe_test_available[NUMBER_OF_RESOURCES];
	int safe_test_finish[NUMBER_OF_CUSTOMERS];
	int safe_test_need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

	// Build a clone of NEED array
	for (i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		for (j = 0; j < NUMBER_OF_RESOURCES; j++)
		{
			safe_test_need[i][j] = need[i][j];
		}
	}

	// Assume the bank promise the request
	for (i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		safe_test_need[user][i] -= request_data[i];
		safe_test_available[i] = available[i] - request_data[i];
	}

	// Initial the safe test to be false
	for (i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		safe_test_finish[i] = 0;
	}

	while (1)
	{
		int finish_in_loop = 0;

		for (i = 0; i < NUMBER_OF_CUSTOMERS && safe_test_finish[i] == 0; i++)
		{
			int is_fulfill = 1;

			for (j = 0; j < NUMBER_OF_RESOURCES; j++)
			{
				if (safe_test_need[i][j] > safe_test_available[j])
					is_fulfill = 0;
			}

			if (is_fulfill)
			{
				for (j = 0; j < NUMBER_OF_RESOURCES; j++)
				{
					safe_test_available[j] += maximum[i][j];
				}
				safe_test_finish[i] = 1;
				finish_in_loop++;
			}
		}

		if (finish_in_loop == 0)
		{
			break;
		}
	}

	for (i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		if (safe_test_finish[i] == 0)
			return -1;
	}

	return 0;
}


/**
 * @brief	Use safety algorithm to check the request
 * @return	0 if success; -1 otherwise
 */
static int request_resources(int user, int request_data[])
{
	int i;

	show_current_resource();

	printf("Customer %d request: ", user);
	for (i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		printf("%2d ", request_data[i]);
	}

	for (i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		if (request_data[i] > need[user][i] 
			|| request_data[i] > available [i])
		{
			printf("  [REJECT]\n");
			return -1;
		}
	}

	pthread_mutex_lock(&lock);
	if (safety_test(user, request_data) != 0)
	{
		printf("  [REJECT]\n");
		pthread_mutex_unlock(&lock);
		return -1;
	}

	for (i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		allocation[user][i] += request_data[i];
		need[user][i] = maximum[user][i] - allocation[user][i];
		available[i] -= request_data[i];
	}
	pthread_mutex_unlock(&lock);
	printf("  [ACCEPT]\n");

	return 0;
}

/**
 * @brief	Release allocated resource
 * @return	0 if success; -1 otherwise
 */
static int release_resources(int user, int release_data[])
{
	int i;

	show_current_resource();

	printf("Customer %d release: ", user);
	for (i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		printf("%2d ", release_data[i]);
	}

	for (i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		if (release_data[i] > allocation[user][i])
		{
			printf("  [REJECT]\n");
			return -1;
		}
	}

	pthread_mutex_lock(&lock);
	for (i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		allocation[user][i] -= release_data[i];
		need[user][i] = maximum[user][i] - allocation[user][i];
		available[i] += release_data[i];
	}
	pthread_mutex_unlock(&lock);
	printf("  [ACCEPT]\n");

	return 0;
}

/**
 * @brief	Thread handler to service each customer's request
 */
static void *banker_runner(void *arg)
{
	int i;
	int result;
	int user = *((int *) arg);
	int request_data[NUMBER_OF_RESOURCES];
	int release_data[NUMBER_OF_RESOURCES];

	/* Generate the maximum resource of customer */
	for (i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		maximum[user][i] = (rand() % (available[i] + 1));
		need[user][i] = maximum[user][i];
	}

	// Wait until every customer choose their maximum
	sleep(1);

	while (1)
	{
		/**
		 * Request resources
		 */
		for (i = 0; i < NUMBER_OF_RESOURCES; i++)
		{
			request_data[i] = (rand() % (need[user][i] + 1));
		}

		if (request_resources(user, request_data) != 0)
		{
			sleep(10);
		}
		sleep(3);

		/**
		 * Release resources
		 */
		for (i = 0; i < NUMBER_OF_RESOURCES; i++)
		{
			release_data[i] = (rand() % (allocation[user][i] + 1));
		}

		if (release_resources(user, release_data) != 0)
		{
			printf("[ERROR] Customer %d WTF! Release resource can be failed?\n", user);
		}
		sleep(3);
	}

	return NULL;
}


static int init_resource(int argc, char **argv)
{
	int i, j;

	if (argc < (NUMBER_OF_RESOURCES + 1))
		return -1;

	/* Configure the total instance count */
	for (i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		available[i] = atoi(argv[i + 1]);
	}

	for (i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		for (j = 0; j < NUMBER_OF_RESOURCES; j++)
		{
			maximum[i][j] = 0;
			allocation[i][j] = 0;
			need[i][j] = 0;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	pthread_t tid[NUMBER_OF_CUSTOMERS];
	int user_id[NUMBER_OF_CUSTOMERS];
	int i, j;

	srand(time(NULL));

	if (init_resource(argc, argv) != 0)
	{
		return -1;
	}

	pthread_mutex_init(&lock, NULL);

	for (i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		user_id[i] = i;
		pthread_create(&tid[i], NULL, &banker_runner, &user_id[i]);
	}

	for (i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		pthread_join(tid[i], NULL);
	}

	pthread_mutex_destroy(&lock);

	return 0;
}
