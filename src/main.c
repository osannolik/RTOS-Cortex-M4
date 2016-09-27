
#include "main.h"

#include "rt_kernel.h"

static void SystemClock_Config(void);
static void Error_Handler(void);

#define RT_SYSTICK_DISABLE do {SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk);} while(0)
#define RT_SYSTICK_ENABLE do {SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;} while(0)

#define TASK_STACK_SIZE 512  

DEFINE_TASK(task_1_fcn, task_1, "T1", 1, TASK_STACK_SIZE);
DEFINE_TASK(task_2_fcn, task_2, "T2", 0, TASK_STACK_SIZE);

int main(void)
{
  HAL_Init();

  /* Configure the system clock to 168 MHz */
  SystemClock_Config();

  debug_init();

  rt_init();

  uint8_t p1 = 0x22;
  uint8_t p2 = 0x33;

  rt_create_task(&task_1, (void *) &p1);
  rt_create_task(&task_2, (void *) &p2);

  HAL_InitTick(TICK_INT_PRIORITY);
  RT_SYSTICK_ENABLE;
  rt_start();

  float xf = 1.0;
  while(1) {
    xf = 1.1 * xf;
    xf = xf - 10.0;
  }
}

void task_1_fcn(void *p)
{
  uint32_t task_cnt = 0;
  uint8_t i = (uint8_t) *((uint8_t *) p);

  while (1) {
    DBG_LED_RESET;
    rt_periodic_delay(200);
    DBG_LED_SET;

    for (task_cnt=0; task_cnt<10000; task_cnt++);
    i++;

    // rt_suspend();
    // HAL_Delay(500);
    // rt_resume();
  }
}

void task_2_fcn(void *p)
{
  float korv = 0.0;
  float broed = 1.0;
  uint8_t i2 = (uint8_t) *((uint8_t *) p);

  while (1) {
    //DBG_LED_RESET;
    korv = korv + 0.01 - 0.1 * broed;
    broed = 1.1 * korv - 0.01;
    i2++;

    // rt_suspend();
    // HAL_Delay(500);
    // rt_resume();
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
