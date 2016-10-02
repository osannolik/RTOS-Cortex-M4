/*
 * rt_kernel.h
 *
 *  Created on: 27 aug 2016
 *      Author: osannolik
 */

#ifndef RT_KERNEL_H_
#define RT_KERNEL_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"

#include "list_sorted.h"
#include "rt_sem.h"

#define ALWAYS_INLINE inline __attribute__((always_inline))

#define rt_systick              SysTick_Handler
#define rt_switch_context       PendSV_Handler
#define rt_syscall              SVC_Handler
//#define rt_enter_critical       rt_mask_irq
//#define rt_exit_critical        rt_unmask_irq

#define RT_KERNEL_IRQ_PRIO      (0x0F)
#define RT_MASK_IRQ_PRIO        (0x06<<4)
#define RT_IDLE_TASK_STACK_SIZE (512)
#define RT_PRIO_LEVELS          (4)

#define RT_FOREVER_TICK         (0xFFFFFFFF)

#define DEFINE_TASK(fcn, handle, name, prio, stack_size) \
  void fcn(void *p);\
  uint32_t handle ## _stack[stack_size];\
  rt_tcb_t handle = TCB_INIT(handle ## _stack, fcn, name, prio, stack_size);


enum {
  UNINITIALIZED = 'U',
  READY         = 'R',
  EXECUTING     = 'X',
  DELAYED       = 'D',
  BLOCKED       = 'B'
};

enum {
  RT_NOK = 0,
  RT_OK
};

enum {
  RT_ERR_STARTFAILURE = 0
};

typedef struct {
  volatile void *sp;
  void *code_start;
  const char *task_name;
  uint32_t priority;
  uint32_t base_prio;
  volatile char state;
  uint32_t stack_size;
  uint32_t delay_woken_tick;
  list_item_t list_item;
  list_item_t blocked_list_item;
} rt_tcb_t;

#define TCB_INIT(sp, fcn, name, prio, stack_size) {sp, fcn, name, prio, prio, UNINITIALIZED, stack_size, 0, LIST_ITEM_INIT, LIST_ITEM_INIT}

typedef uint8_t rt_status_t;
typedef rt_tcb_t* rt_task_t;

void rt_systick();
void rt_switch_context() __attribute__((naked));
void rt_syscall()        __attribute__((naked));

void rt_yield(void);
uint32_t rt_set_current_task_blocked(list_sorted_t *blocked_list, uint32_t ticks_timeout);
void rt_set_task_unblocked(rt_task_t const task);
void rt_enter_critical(void);
void rt_exit_critical(void);
void rt_suspend(void);
void rt_resume(void);
uint32_t rt_get_tick(void);
void rt_periodic_delay(const uint32_t period);
uint32_t rt_create_task(rt_task_t const task, void * const task_parameters);
void rt_start();
uint32_t rt_init();


ALWAYS_INLINE static void rt_pend_yield()
{
  SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
  __asm volatile (
    " dsb    \n\t"
    " isb    \n\t"
  );
}

ALWAYS_INLINE static void rt_enable_irq(void)
{
  __asm volatile (
    " cpsie if    \n\t"
  );
}

ALWAYS_INLINE static void rt_disable_irq(void)
{
  __asm volatile (
    " cpsid if    \n\t"
  );
}

ALWAYS_INLINE static void rt_mask_irq(void)
{
  uint32_t tmp = RT_MASK_IRQ_PRIO;

  __asm volatile (

    " msr basepri, %0  \n\t"
    :
    : "r" (tmp)
  );
}

ALWAYS_INLINE static void rt_unmask_irq(void)
{
  uint32_t tmp = 0;

  __asm volatile (

    " msr basepri, %0  \n\t"
    :
    : "r" (tmp)
  );
}

#endif /* RT_KERNEL_H_ */
