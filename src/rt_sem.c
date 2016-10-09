/*
 * rt_sem.c
 *
 *  Created on: 28 sep 2016
 *      Author: osannolik
 */

#include "rt_kernel.h"
#include "rt_sem.h"

void rt_sem_init(rt_sem_t *sem, uint32_t count)
{
  sem->counter = count;
  list_sorted_init((list_sorted_t *) &(sem->blocked));
}

uint32_t rt_sem_take(rt_sem_t *sem, const uint32_t ticks_timeout)
{
  uint32_t sem_taken = RT_OK;
  volatile uint32_t suspend_tick;

  rt_enter_critical();

  if (sem->counter == 0) {
    list_item_t *blocked_list_item = &(current_task->blocked_list_item);

    // Add currently running task to blocked list
    blocked_list_item->value = current_task->priority;
    list_sorted_insert((list_sorted_t *) &(sem->blocked), blocked_list_item);

    // Suspend task for ticks_timeout ticks
    suspend_tick = rt_get_tick();

    rt_list_task_delayed(current_task, suspend_tick+ticks_timeout);

    rt_exit_critical();

    // yields here

    rt_enter_critical();

    // When resumed, check if it was due to timeout
    if (current_task->delay_woken_tick >= suspend_tick+ticks_timeout) {
      sem_taken = RT_NOK;
      list_sorted_remove(blocked_list_item);
    }
  } 

  if (sem_taken == RT_OK)
    (sem->counter)--;

  rt_exit_critical();

  return sem_taken;
}

uint32_t rt_sem_give(rt_sem_t *sem)
{
  rt_task_t unblocked_task; 
  uint32_t  task_unblocked = RT_NOK;

  rt_enter_critical();

  if ((sem->counter)++ == 0 && sem->blocked.len > 0) {
    // Unblock the highest prio blocked task
    unblocked_task = (rt_task_t) LIST_LAST_REF(&(sem->blocked));
    list_sorted_remove((list_item_t *) &(unblocked_task->blocked_list_item));
    rt_list_task_undelayed(unblocked_task);
    rt_list_task_ready_next(unblocked_task);

    task_unblocked = RT_OK;

    if (unblocked_task->priority >= current_task->priority) {
      // Trig a task switch
      rt_pend_yield();
    }
  }

  rt_exit_critical();

  return task_unblocked;
}
