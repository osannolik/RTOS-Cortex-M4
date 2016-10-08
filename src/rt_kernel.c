/*
 * rt_kernel.c
 *
 *  Created on: 27 aug 2016
 *      Author: osannolik
 */

#include "rt_kernel.h"
#include "rt_lists.h"

#include "debug.h"

static uint32_t rt_init_interrupt_prios();
static uint32_t * rt_init_stack(void *code, void * const task_parameters, const uint32_t stack_size, volatile void * stack_data);
static void rt_error_handler(uint8_t err);
static uint32_t rt_increment_tick();


void rt_switch_task();

DEFINE_TASK(rt_idle, idle_task, "IDLE", 0, RT_IDLE_TASK_STACK_SIZE);

rt_tcb_t * volatile current_tcb = NULL;

volatile uint32_t tick = 0;
static volatile uint32_t ticks_in_suspend = 0;
static volatile uint32_t kernel_suspended = 0;
volatile uint32_t next_wakeup_tick = RT_FOREVER_TICK;
rt_task_t volatile next_wakeup_task = NULL;
static volatile uint32_t nest_critical = 0;

extern list_sorted_t delayed[RT_PRIO_LEVELS];
extern list_sorted_t ready[RT_PRIO_LEVELS];


void rt_idle(void *p)
{
  while (1) {
    DBG_PAD4_SET;
  }
}

void rt_enter_critical(void)
{
  rt_mask_irq();
  ++nest_critical;
}

void rt_exit_critical(void)
{
  if(--nest_critical == 0)
    rt_unmask_irq();
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

static uint32_t * rt_init_stack(void *code, void * const task_parameters, const uint32_t stack_size, volatile void * stack_data)
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
  task->list_item.reference = (void *) task;
  task->blocked_list_item.reference = (void *) task;

  // Add to the ready list that correponds to the task prio
  rt_list_task_ready(task);

  return RT_OK;
}

void rt_yield(void)
{
  rt_pend_yield();
  rt_unmask_irq();
}

void rt_periodic_delay(const uint32_t period)
{
  // This does of course not guarantee that the task will execute in the specified period
  // if there are other tasks of higher prio...
  rt_enter_critical();

  uint32_t task_nominal_wakeup_tick = current_tcb->delay_woken_tick + period;

  rt_list_task_delayed(current_tcb, task_nominal_wakeup_tick);

  rt_exit_critical();
}

static uint32_t rt_increment_tick()
{
  // Figure out if context switch is needed and update ready list...

  if (kernel_suspended) {
    ++ticks_in_suspend;
    return RT_NOK;
  }

  ++tick;

  rt_task_t woken_task;
  uint8_t do_context_switch = 0;

  while (tick >= next_wakeup_tick) {
    // Wake up a delayed task
    woken_task = next_wakeup_task;
    
    woken_task->delay_woken_tick = tick;

    rt_list_task_undelayed(woken_task); // Updates next_wakeup_tick and next_wakeup_task

    // Make it the next one up in its prio ready list
    rt_list_task_ready_next(woken_task);

    // Only trig a switch if the woken task has high enough prio
    if (woken_task->priority >= current_tcb->priority)
      do_context_switch = 1;
  }

  // If there are other tasks with the same prio as the current, let them get some cpu time
  if (do_context_switch || LIST_LENGTH(&(ready[current_tcb->priority])) > 1)
    return RT_OK;
  else
    return RT_NOK;
}

void rt_switch_task()
{
  uint8_t prio;

  rt_mask_irq();

  // Pick highest prio of the ready tasks
  for (prio = RT_PRIO_LEVELS-1; LIST_LENGTH(&(ready[prio])) == 0; prio--);

  // TODO: Check if there actually are any ready tasks?

  // Get the next reference in the ready list
  current_tcb = (rt_tcb_t *) list_sorted_get_iter_ref((list_sorted_t *) &(ready[prio]));

  DBG_PAD4_RESET;

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
  rt_lists_delayed_init();
  rt_lists_ready_init();

  return RT_OK;
}

void rt_start()
{
  uint8_t prio;

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

    __asm volatile (" svc 0 " );  // Need to be in handler mode to restore correctly => system call
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
