/*
 * rt_sem.c
 *
 *  Created on: 28 sep 2016
 *      Author: osannolik
 */

#include "rt_sem.h"

extern rt_tcb_t *current_tcb;

void rt_sem_init(rt_sem_t *sem, uint32_t count)
{
  sem->counter = count;
  list_sorted_init((list_sorted_t *) &(sem->blocked));
}

uint32_t rt_sem_take(rt_sem_t *sem, const uint32_t ticks_timeout)
{
  uint32_t sem_taken = RT_OK;

  rt_enter_critical();

  if (sem->counter == 0) {
    // Semaphore is completely taken
    sem_taken = rt_set_current_task_blocked((list_sorted_t *) &(sem->blocked), ticks_timeout); // Taken if unblocked before timeout
  } 

  if (sem_taken == RT_OK) {
    (sem->counter)--;
  }

  rt_exit_critical();

  return sem_taken;
}

uint32_t rt_sem_give(rt_sem_t *sem)
{
  rt_task_t     unblocked_task; 
  uint32_t      task_unblocked = RT_NOK;

  rt_enter_critical();

  if ((sem->counter)++ == 0 && sem->blocked.len > 0) {
    // Unblock the highest prio blocked task
    unblocked_task = (rt_task_t) LIST_LAST_REF(&(sem->blocked));
    rt_set_task_unblocked(unblocked_task);

    task_unblocked = RT_OK;
  }

  rt_exit_critical();

  return task_unblocked;
}