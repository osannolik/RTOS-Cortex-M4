
#include "main.h"

#include "rt_kernel.h"
#include "rt_sem.h"
#include "rt_queue.h"

static void SystemClock_Config(void);
static void Error_Handler(void);

#define RT_SYSTICK_DISABLE do {SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk);} while(0)
#define RT_SYSTICK_ENABLE do {SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;} while(0)

#define TASK_STACK_SIZE 512  

DEFINE_TASK(task_1_fcn, task_1, "T1", 2, TASK_STACK_SIZE);
DEFINE_TASK(task_2_fcn, task_2, "T2", 1, TASK_STACK_SIZE);
DEFINE_TASK(task_3_fcn, task_3, "T3", 3, TASK_STACK_SIZE);

rt_sem_t semaphore;

rt_queue_t queue;

#define N_ITEMS (10)

uint8_t my_item = 0;

uint8_t qBuffer[sizeof(my_item)*N_ITEMS];

static TIM_HandleTypeDef handler_timer;

void init_timer();

int main(void)
{
  HAL_Init();

  /* Configure the system clock to 168 MHz */
  SystemClock_Config();

  debug_init();

  rt_init();

  rt_create_task(&task_1, NULL);
  rt_create_task(&task_2, NULL);
  rt_create_task(&task_3, NULL);

  rt_sem_init(&semaphore, 0);

  rt_queue_init(&queue, qBuffer, sizeof(my_item), N_ITEMS);

  init_timer();
  HAL_TIM_Base_Start_IT(&handler_timer);

  HAL_InitTick(TICK_INT_PRIORITY);
  RT_SYSTICK_ENABLE;
  rt_start();

  while(1);

  return 0;
}

void task_1_fcn(void *p)
{
  uint32_t task_cnt = 0;
  uint8_t sem_taken = 0;
  uint8_t item_pushed = 0;

  while (1) {

    DBG_PAD1_RESET;

    if (rt_sem_take(&semaphore, 10) == RT_OK) {

      if (rt_queue_push(&queue, &my_item, 100) == RT_OK)
        item_pushed = 1;
      else
        item_pushed = 0;

      my_item++;
      DBG_PAD1_SET;
      for (task_cnt=0; task_cnt<5000; task_cnt++);

    }

  }
}

void task_2_fcn(void *p)
{
  uint32_t task_cnt = 0;
  uint8_t  task_unblocked = 0;

  while (1) {

    DBG_PAD2_RESET;

    rt_periodic_delay(5);

    // if (rt_sem_give(&semaphore) == RT_OK)
    //   task_unblocked = 1;
    // else
    //   task_unblocked = 0;

    DBG_PAD2_SET;

    for (task_cnt=0; task_cnt<10000; task_cnt++);

  }
}

// void task_1_fcn(void *p)
// {
//   uint32_t task_cnt = 0;

//   while (1) {

//     DBG_PAD1_RESET;

//     rt_periodic_delay(1);

//     DBG_PAD1_SET;

//     for (task_cnt=0; task_cnt<10000; task_cnt++);

//   }
// }

// void task_2_fcn(void *p)
// {
//   uint32_t task_cnt = 0;

//   while (1) {

//     DBG_PAD2_RESET;

//     rt_periodic_delay(8);

//     DBG_PAD2_SET;

//     for (task_cnt=0; task_cnt<10000; task_cnt++);

//   }
// }

uint8_t my_item_pulled = 0;

void task_3_fcn(void *p)
{
  uint32_t task_cnt = 0;

  while (1) {

    DBG_PAD3_RESET;

    //rt_periodic_delay(50);

    if (rt_queue_pull(&queue, &my_item_pulled, 100) == RT_OK) {
      DBG_PAD3_SET;
      for (task_cnt=0; task_cnt<10000; task_cnt++);
    }
  }
}


void init_timer()
{
  TIM_ClockConfigTypeDef sClockSourceConfig;

  __TIM3_CLK_ENABLE();

  handler_timer.Channel = HAL_TIM_ACTIVE_CHANNEL_1;
  handler_timer.Instance = TIM3;
  handler_timer.Init.CounterMode = TIM_COUNTERMODE_UP;
  handler_timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  handler_timer.Init.Prescaler = 4*64-1;
  handler_timer.Init.Period = 2625;
  HAL_TIM_Base_Init(&handler_timer);

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  sClockSourceConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV8;
  HAL_TIM_ConfigClockSource(&handler_timer, &sClockSourceConfig);

  __HAL_TIM_SET_COUNTER(&handler_timer, 0);

  HAL_NVIC_SetPriority(TIM3_IRQn, 10, 0);
  HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

void TIM3_IRQHandler()
{
  if (__HAL_TIM_GET_ITSTATUS(&handler_timer, TIM_IT_UPDATE) != RESET) {
      __HAL_TIM_CLEAR_FLAG(&handler_timer, TIM_FLAG_UPDATE);

      DBG_PAD5_TOGGLE;

      uint32_t task_unblocked = rt_sem_give_from_isr(&semaphore);

      if (task_unblocked != RT_NOK)
        rt_pend_yield();
      
  }
}



/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 168000000
  *            HCLK(Hz)                       = 168000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8 => 16
  *            PLL_N                          = 336
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  
  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported  */
  if (HAL_GetREVID() == 0x1001)
  {
    /* Enable the Flash prefetch */
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
  }
}

static void Error_Handler(void)
{
  /* User may add here some code to deal with this error */
  while(1)
  {
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
