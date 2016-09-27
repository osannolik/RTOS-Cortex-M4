/*
 * rt_kernel.c
 *
 *  Created on: 27 aug 2016
 *      Author: osannolik
 */

#include "stm32f4xx_hal.h"

#include "rt_kernel.h"

static uint32_t rt_init_interrupt_prios();
static void rt_error_handler(uint8_t err);
static uint32_t rt_increment_tick();
static void rt_set_task_ready(rt_task_t const task);
static void rt_set_task_ready_next(rt_task_t const task);
static void rt_set_task_delayed(rt_task_t const task, const uint32_t wake_up_tick);

DEFINE_TASK(rt_idle, idle_task, "IDLE", 0, RT_IDLE_TASK_STACK_SIZE);

static rt_tcb_t * volatile current_tcb = NULL;

static volatile uint32_t tick = 0;
static volatile uint32_t ticks_in_suspend = 0;
static volatile uint32_t kernel_suspended = 0;
static volatile uint32_t next_wakeup_tick = RT_FOREVER_TICK;

static volatile list_sorted_t delayed;
static volatile list_sorted_t ready[RT_PRIO_LEVELS];

void rt_idle(void *p)
{
  while (1);
}

void rt_suspend(void)
{
  ++kernel_suspended;
}

void rt_resume(void)
{
  // Note: No check against incorrect nesting of rt_suspend/rt_resume.
  rt_mask_irq();

  --kernel_suspended;

  if (ticks_in_suspend > 0) {
    // Force a yield if the scheduler actually was blocked during the suspension.
    ticks_in_suspend = 0;
    rt_pend_yield();
  }
    
  rt_unmask_irq();
}

uint32_t rt_get_tick(void)
{
  return tick;
}

uint32_t * rt_init_stack(void *code, void * const task_parameters, const uint32_t stack_size, volatile void * stack_data)
{
  uint32_t *stackptr = (uint32_t *) stack_data;
  stackptr = (uint32_t*) &stackptr[stack_size-1];

  // Needs to be 8-byte aligned
  if ((uint32_t) stackptr & 0x04) {
    stackptr--;
  }

  // PSR
  *stackptr-- = (uint32_t) 0x01000000;

  // PC, as PC is loaded on exit from ISR, bit0 must be zero
  *stackptr-- = (uint32_t) code & 0xFFFFFFFE;

  // LR: Use non-floating point popping and use psp when returning
  *stackptr = (uint32_t) 0xFFFFFFFD;

  // R12, R3, R2, R1
  stackptr -= 5;
  
  // R0
  *stackptr-- = (uint32_t) task_parameters;

  *stackptr = (uint32_t) 0xFFFFFFFD;

  // R11, R10, R9, R8, R7, R6, R5, R4
  stackptr -= 8;

  return stackptr;
}

uint32_t rt_create_task(rt_task_t const task, void * const task_parameters)
{
  uint32_t prio = task->priority;

  task->state = UNINITIALIZED;

  if (prio >= RT_PRIO_LEVELS)
    return RT_NOK;

  task->sp = rt_init_stack(task->code_start, task_parameters, task->stack_size, task->sp);

  if (task->sp == NULL)
    return RT_NOK;

  task->state = READY;

  // Add to the ready list that correponds to the task prio
  rt_set_task_ready(task);

  return RT_OK;
}

static void rt_set_task_ready(rt_task_t const task)
{
  uint32_t task_prio = task->priority;

  task->list_item.value = task_prio;
  task->list_item.reference = (void *) task;
  list_sorted_insert((list_sorted_t *) &(ready[task_prio]), &(task->list_item));
}

static void rt_set_task_ready_next(rt_task_t const task)
{
  uint32_t task_prio = task->priority;

  task->list_item.value = task_prio;
  task->list_item.reference = (void *) task;
  list_sorted_iter_insert((list_sorted_t *) &(ready[task_prio]), &(task->list_item));
}

static void rt_set_task_delayed(rt_task_t const task, const uint32_t wake_up_tick)
{
  task->list_item.value = wake_up_tick;
  task->list_item.reference = (void *) task;
  list_sorted_insert((list_sorted_t *) &delayed, &(task->list_item));
}

void rt_periodic_delay(const uint32_t period)
{
  uint32_t task_wakeup_tick;

  rt_mask_irq();

  task_wakeup_tick = current_tcb->delay_woken_tick + period;

  if (task_wakeup_tick < next_wakeup_tick)
    next_wakeup_tick = task_wakeup_tick;

  if (next_wakeup_tick > tick) {
    // Remove from ready list and add to delayed list
    list_sorted_remove(&(current_tcb->list_item));
    rt_set_task_delayed(current_tcb, task_wakeup_tick);
  }

  rt_pend_yield();

  rt_unmask_irq();
}

static uint32_t rt_increment_tick()
{
  // Figure out if context switch is needed and update ready list...

  rt_task_t woken_task;

  if (kernel_suspended) {
    ++ticks_in_suspend;
    return RT_NOK;
  }

  ++tick;

  if (tick >= next_wakeup_tick) {
    // Wake up a delayed task
    woken_task = (rt_task_t) LIST_FIRST_REF(&delayed);
    
    if (list_sorted_remove(&(woken_task->list_item)) > 0) {
      // There are more delayed tasks, so find out when to wake it up
      next_wakeup_tick = LIST_MIN_VALUE(&delayed);
    } else {
      // This was the only task delayed
      next_wakeup_tick = RT_FOREVER_TICK;
    }

    woken_task->delay_woken_tick = tick;

    // Only trig a switch if the woken task has high enough prio
    if (woken_task->priority >= current_tcb->priority) {
      // In such a case, make it the next one up
      rt_set_task_ready_next(woken_task);
      return RT_OK;
    } else {
      rt_set_task_ready(woken_task);
      return RT_NOK;
    }
  }

  // If there are other tasks with the same prio as the current, let them get some cpu time
  if (LIST_LENGTH(&(ready[current_tcb->priority])) > 1)
    return RT_OK;
  else
    return RT_NOK;
}

void rt_switch_task()
{
  uint32_t prio;

  rt_mask_irq();

  // Pick highest prio of the ready tasks
  for (prio = RT_PRIO_LEVELS-1; LIST_LENGTH(&(ready[prio])) == 0; prio--);

  // Get the next reference in the ready list
  current_tcb = (rt_tcb_t *) list_sorted_get_iter_ref((list_sorted_t *) &(ready[prio]));

  rt_unmask_irq();
}

static uint32_t rt_init_interrupt_prios()
{
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4); // No sub-priorities

  NVIC_SetPriority(PendSV_IRQn, RT_KERNEL_IRQ_PRIO);    // Set lowest prio
  NVIC_SetPriority(SysTick_IRQn, RT_KERNEL_IRQ_PRIO);

  return RT_OK;
}

uint32_t rt_init()
{
  uint8_t prio;

  for (prio=0; prio<RT_PRIO_LEVELS; prio++)
    list_sorted_init((list_sorted_t *) &(ready[prio]));

  list_sorted_init((list_sorted_t *) &delayed);

  return RT_OK;
}

void rt_start()
{
  uint32_t prio;

  // Create a kernel idle task with lowest priority
  rt_create_task(&idle_task, NULL);

  // Pick highest prio of the ready tasks as the first to execute
  for (prio=RT_PRIO_LEVELS-1; LIST_LENGTH(&(ready[prio]))==0; prio--) {
    if (prio==0)
      rt_error_handler(RT_ERR_STARTFAILURE); // Found no ready tasks
  }

  current_tcb = (rt_tcb_t *) LIST_MIN_VALUE_REF(&(ready[prio]));

  if (rt_init_interrupt_prios()) {
    // Brace yourselves, the kernel is starting!
    rt_unmask_irq();
    rt_enable_irq();
    __asm volatile (
      " svc 0             \n\t" // Need to be in handler mode to restore correctly => system call
    );
  }

}

void rt_syscall()
{
  __asm volatile (
    " ldr r0, [%[SP]]           \n\t"
    " ldmia r0!, {r4-r11, lr}   \n\t"
    " msr psp, r0               \n\t" // Restore the process stack pointer
    " isb                       \n\t"
    " bx lr                     \n\t"
    : 
    : [SP] "r" (&(current_tcb->sp))
    : 
  );
}

void rt_systick()
{
  HAL_IncTick();

  rt_mask_irq();

  if (RT_OK==rt_increment_tick())
    rt_pend_yield();
  
  rt_unmask_irq();
}

void rt_switch_context()
{
  __asm volatile (
    " mrs r0, psp               \n\t" // psp -> r0
    " isb                       \n\t" // Instruction synchronization barrier
    " tst lr, #0x10             \n\t" // Check LR:
    " it eq                     \n\t" // Push upper floating point regs if used by process
    " vstmdbeq r0!, {s16-s31}   \n\t"
    "                           \n\t"
    " stmdb r0!, {r4-r11, lr}   \n\t" // Push rest of core regs, and specifically lr so that 
    "                           \n\t" // it can be checked by switcher for fp use when restoring
    " str r0, [%[SP]]           \n\t" // Store current stack-pointer to tcb
    " dsb                       \n\t"
    " isb                       \n\t"
    " bl rt_switch_task         \n\t" // Possibly change context to next process
    "                           \n\t"
    :
    : [SP] "r" (&(current_tcb->sp))
    :
  );

  // Split inline assembly to force re-evaluation of current_tcb (updated by context switcher)

  __asm volatile (
    " ldr r0, [%[SP]]           \n\t" // Get stack-pointer from tcb
    " ldmia r0!, {r4-r11, lr}   \n\t" // Pop rest of core regs, and specifically lr
    "                           \n\t"
    " tst lr, #0x10             \n\t" // Check LR: 
    " it eq                     \n\t" // Pop upper floating point regs if used by process
    " vldmiaeq r0!, {s16-s31}   \n\t"
    "                           \n\t"
    " msr psp, r0               \n\t" // Restore the process stack pointer
    " isb                       \n\t"
    " bx lr                     \n\t" // Continue next task
    :
    : [SP] "r" (&(current_tcb->sp))
    :
  );
}

void rt_error_handler(uint8_t err)
{

}
