/*
 * semaphore.c
 *
 *  Created on: 13 Mar 2021
 *      Author: kraego
 */
#include <stdbool.h>
#include "semaphore.h"
#include "private/queue.h"
#include "private/atomic.h"
#include "private/scheduler_private.h"

#define MAX_SEMS	(10)

typedef struct {
	semID id;
	uint32_t count;
	t_Queue *blockedThreads;
} t_semaphore;

static uint32_t gSemcount = 0;
static t_semaphore gSemaphores[MAX_SEMS] = { 0 };

/**
 * Creates an counting semaphore
 *
 * @param count of semaphore
 * @param state full or empty
 * @return returns a semaphore id if successful otherwise SEM_ID_INVALID is returned
 */
semID semaphore_create(uint32_t count, sem_state state) {
	if (gSemcount >= MAX_SEMS) {
		return SEM_ID_INVALID;
	}
	uint32_t semId = gSemcount;
	bool blockedQueueSuccess = false;
	gSemcount++;
	gSemaphores[semId].id = semId;
	gSemaphores[semId].count = state == FULL ? MAX_SEMS : 0;

	blockedQueueSuccess = queue_init(&(gSemaphores[semId].blockedThreads) , MAX_SEMS) == 0;

	return blockedQueueSuccess ? semId : SEM_ID_INVALID;
}

/**
 * Try to take semaphore, blocks if no semaphore is available
 *
 * @param sem the id of the semaphore
 */
void semaphore_take(semID sem) {
	ATOMIC_START();

	if (gSemaphores[sem].count > 0) {
		gSemaphores[sem].count--;
		ATOMIC_END();
	} else {
		queue_enqueue(gSemaphores[sem].blockedThreads, gRunningThread);
		ATOMIC_END();
		scheduler_blockThread();
	}
}

/**
 * Give semaphore back
 *
 * @param sem the id of the semaphore
 */
void semaphore_give(semID sem) {
	ATOMIC_START();

	if (gSemaphores[sem].blockedThreads->rear > 0) {
		scheduler_unblockThread(gSemaphores[sem].blockedThreads->queue[0]);
		queue_dequeue(gSemaphores[sem].blockedThreads);
	}
	gSemaphores[sem].count = (gSemaphores[sem].count + 1) % MAX_SEMS;
	ATOMIC_END();
}

/**
 * Delete Semaphore
 *
 * @param sem the id of the semaphore
 *
 * return 0 on success, -1 if there are any blocked threads
 */
uint32_t semaphore_delete(semID sem) {
	ATOMIC_START();

	if (gSemaphores[sem].blockedThreads->rear > 0) {
		return -1;
	}
	gSemaphores[sem].id = SEM_ID_INVALID;
	queue_delete(gSemaphores[sem].blockedThreads);
	ATOMIC_END();
	return 0;
}
