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


/**
 * @brief	Test whether the resource has fulfill the need
 * @return	1 when fulfilled; 0 otherwise
 */
static int resource_fulfill(int user)
{
	int i;

	for (i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		if (need[user][i] != 0)
			return 0;
	}

	return 1;
}

/**
 * @brief	Use safety algorithm to check the request
 * @return	0 if success; -1 otherwise
 */
static int request_resource(int user)
{
	int i, j;
	int unfinished;
	int request_data[NUMBER_OF_RESOURCES];
	int safe_test_available[NUMBER_OF_RESOURCES];
	int safe_test_finish[NUMBER_OF_CUSTOMERS];

	/* Generate request data randomly */
	for (i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		if (need[user][i] < available[i])
			request_data[i] = (rand() % (need[user][i] + 1));
		else
			request_data[i] = (rand() % (available[i] + 1));
	}
	printf("\navailable: %2d %2d %2d\n", available[0], available[1], available[2]);
	for (i = 0; i < NUMBER_OF_CUSTOMERS; i++)
		printf("Runner  %d: %2d %2d %2d  |  %2d %2d %2d\n", i, need[i][0], need[i][1], need[i][2], allocation[i][0], allocation[i][1], allocation[i][2]);
	printf("\n>>> Runner %d try to request: %d %d %d\n", user, request_data[0], request_data[1], request_data[2]);

	for (i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		safe_test_available[i] = available[i] - request_data[i];
		need[user][i] -= request_data[i];
	}

	for (i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		/* Set safe state into false */
		safe_test_finish[i] = 0;
	}

	unfinished = NUMBER_OF_CUSTOMERS;
	while (1)
	{
		int curr_unfinished = 0;

		for (i = 0; i < NUMBER_OF_CUSTOMERS && safe_test_finish[i] == 0; i++)
		{
			int is_fulfill = 1;

			for (j = 0; j < NUMBER_OF_RESOURCES; j++)
			{
				if (need[i][j] > safe_test_available[j])
				{
					printf("@@ %d > %d\n", need[i][j], safe_test_available[j]);
					is_fulfill = 0;
				}
			}
			printf("@@ runner %d fulfill? %d\n", i, is_fulfill);

			if (is_fulfill)
			{
				printf("@@ set %d into finish\n", i);
				for (j = 0; j < NUMBER_OF_RESOURCES; j++)
				{
					safe_test_available[j] += maximum[i][j];
				}

				safe_test_finish[i] = 1;
			}
		}

		for (i = 0; i < NUMBER_OF_CUSTOMERS; i++)
		{
			if (safe_test_finish[i] == 0)
				curr_unfinished++;
		}

	printf("@@ unfinish: %d\n", curr_unfinished);
		/* no one can allocate enough resource */
		if (unfinished == curr_unfinished)
			break;

		unfinished = curr_unfinished;
	}

	if (unfinished != 0)
	{
		/* Recover the need value */
		for (i = 0; i < NUMBER_OF_RESOURCES; i++)
		{
			need[user][i] += request_data[i];
		}
		printf(">>> Runner %d request failed\n", user);
		return -1;
	}

	for (i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		allocation[user][i] += request_data[i];
		available[i] -= request_data[i];
	}
	printf(">>> Runner %d request succeded\n", user);

	return 0;
}

static void *banker_runner(void *arg)
{
	int i;
	int result;
	int user_no = *((int *) arg);

	for (i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		maximum[user_no][i] = (rand() % (available[i] + 1));
		need[user_no][i] = maximum[user_no][i];
	}
	printf("Runner %d: %d %d %d\n", user_no, maximum[user_no][0], maximum[user_no][1], maximum[user_no][2]);
	sleep(1);

	while (1)
	{
		/* No need to run anymore. Bye >_* */
		if (resource_fulfill(user_no))
		{
			pthread_mutex_lock(&lock);
			for (i = 0; i < NUMBER_OF_RESOURCES; i++)
			{
				available[i] += maximum[user_no][i];
				maximum[user_no][i] = 0;
				allocation[user_no][i] = 0;
				need[user_no][i] = 0;
			}
			pthread_mutex_unlock(&lock);
			break;
		}

		/* try to acquire resource */
		pthread_mutex_lock(&lock);
		result = request_resource(user_no);
		pthread_mutex_unlock(&lock);
		if (result != 0)
		{
			sleep(2);
		}
	}

	return NULL;
}


int init_resource(int argc, char **argv)
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
	int user_no[NUMBER_OF_CUSTOMERS];
	int i, j;

	srand(time(NULL));

	if (init_resource(argc, argv) != 0)
	{
		return -1;
	}
	printf("system:  %2d %2d %2d\n", available[0], available[1], available[2]);

	pthread_mutex_init(&lock, NULL);

	for (i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		user_no[i] = i;
		pthread_create(&tid[i], NULL, &banker_runner, &user_no[i]);
	}

	for (i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		pthread_join(tid[i], NULL);
	}

	pthread_mutex_destroy(&lock);

	return 0;
}
