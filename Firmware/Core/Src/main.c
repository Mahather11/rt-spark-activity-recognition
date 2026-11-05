/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cmsis_os.h"
#include <stdio.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// LSM6DSL Register Addresses and I2C Address
#define LSM6DSL_ADDR      (0x6A << 1)
#define LSM6DSL_ADDR_ALT  (0x6B << 1)
#define LSM6DSL_WHO_AM_I  0x0F
#define LSM6DSL_CTRL1_XL  0x10
#define LSM6DSL_CTRL2_G   0x11
#define LSM6DSL_OUTX_L_G  0x22
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;
I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi2;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* USER CODE BEGIN PV */
UART_HandleTypeDef huart1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_I2C2_Init(void);
static void MX_I2C1_Init(void);
void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void MX_USART1_UART_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

int _write(int file, char *ptr, int len)
{
  (void)file;
  HAL_UART_Transmit(&huart1, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
  return len;
}

// --- LSM6DSL Sensor Functions ---
static uint16_t lsm6dsl_i2c_addr;
static I2C_HandleTypeDef *lsm6dsl_i2c;

static bool LSM6DSL_FindOnBus(I2C_HandleTypeDef *i2c,
                              uint16_t *foundAddress,
                              bool *sawBusy)
{
    uint8_t data;
    const uint16_t addresses[] = { LSM6DSL_ADDR, LSM6DSL_ADDR_ALT };

    for (size_t i = 0U; i < (sizeof(addresses) / sizeof(addresses[0])); i++) {
        HAL_StatusTypeDef status;

        data = 0U;
        status = HAL_I2C_Mem_Read(i2c, addresses[i], LSM6DSL_WHO_AM_I,
                                  I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
        printf("LSM6DSL WHO_AM_I at 0x%02X: status=%d value=0x%02X\r\n",
               (unsigned int)(addresses[i] >> 1), (int)status, data);
        if (status == HAL_BUSY) {
            *sawBusy = true;
        }
        if ((status == HAL_OK) && (data == 0x6A)) {
            *foundAddress = addresses[i];
            return true;
        }
    }
    return false;
}

static HAL_StatusTypeDef LSM6DSL_ConfigureOnBus(I2C_HandleTypeDef *i2c,
                                                 uint16_t address)
{
    uint8_t data = 0x40U;
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Write(i2c, address, LSM6DSL_CTRL1_XL,
                               I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
    if (status != HAL_OK) {
        return status;
    }

    data = 0x40U;
    return HAL_I2C_Mem_Write(i2c, address, LSM6DSL_CTRL2_G,
                              I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
}

HAL_StatusTypeDef LSM6DSL_Init(void) {
    uint16_t address = 0U;
    bool sawBusy = false;
    HAL_StatusTypeDef status;

    lsm6dsl_i2c = &hi2c2;
    if (!LSM6DSL_FindOnBus(lsm6dsl_i2c, &address, &sawBusy) && sawBusy) {
        printf("I2C2 is busy; trying the sensor on I2C1 PB8/PB9\r\n");
        MX_I2C1_Init();
        sawBusy = false;
        if (!LSM6DSL_FindOnBus(&hi2c1, &address, &sawBusy)) {
            lsm6dsl_i2c = NULL;
            return HAL_ERROR;
        }
        lsm6dsl_i2c = &hi2c1;
    }
    if (address == 0U) {
        return HAL_ERROR;
    }

    lsm6dsl_i2c_addr = address;
    printf("LSM6DSL found at 0x%02X\r\n", (unsigned int)(address >> 1));

    status = LSM6DSL_ConfigureOnBus(lsm6dsl_i2c, lsm6dsl_i2c_addr);
    if ((status == HAL_BUSY) && (lsm6dsl_i2c == &hi2c2)) {
        printf("I2C2 became busy during sensor setup; trying I2C1 PB8/PB9\r\n");
        MX_I2C1_Init();
        address = 0U;
        sawBusy = false;
        if (!LSM6DSL_FindOnBus(&hi2c1, &address, &sawBusy)) {
            lsm6dsl_i2c = NULL;
            lsm6dsl_i2c_addr = 0U;
            return HAL_ERROR;
        }
        lsm6dsl_i2c = &hi2c1;
        lsm6dsl_i2c_addr = address;
        status = LSM6DSL_ConfigureOnBus(lsm6dsl_i2c, lsm6dsl_i2c_addr);
    }
    if (status != HAL_OK) {
        return status;
    }
    return HAL_OK;
}

HAL_StatusTypeDef LSM6DSL_ReadData(float *ax, float *ay, float *az,
                                   float *gx, float *gy, float *gz) {
    uint8_t buffer[12];
    int16_t raw_x, raw_y, raw_z;
    
    // Read all 12 bytes (6 Gyro, 6 Accel) starting from Gyro X_L
    if ((lsm6dsl_i2c == NULL) || (lsm6dsl_i2c_addr == 0U) ||
        (HAL_I2C_Mem_Read(lsm6dsl_i2c, lsm6dsl_i2c_addr, LSM6DSL_OUTX_L_G,
                          I2C_MEMADD_SIZE_8BIT, buffer, sizeof(buffer), 100) != HAL_OK)) {
        return HAL_ERROR;
    }
    
    // Parse Gyro (convert to degrees per second)
    raw_x = (int16_t)((buffer[1] << 8) | buffer[0]);
    raw_y = (int16_t)((buffer[3] << 8) | buffer[2]);
    raw_z = (int16_t)((buffer[5] << 8) | buffer[4]);
    *gx = raw_x * 0.00875f;
    *gy = raw_y * 0.00875f;
    *gz = raw_z * 0.00875f;
    
    // Parse Accel (convert to m/s^2)
    raw_x = (int16_t)((buffer[7] << 8) | buffer[6]);
    raw_y = (int16_t)((buffer[9] << 8) | buffer[8]);
    raw_z = (int16_t)((buffer[11] << 8) | buffer[10]);
    *ax = raw_x * 0.000061f * 9.80665f;
    *ay = raw_y * 0.000061f * 9.80665f;
    *az = raw_z * 0.000061f * 9.80665f;
    return HAL_OK;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_I2C2_Init();
  
  /* USER CODE BEGIN 2 */
  MX_USART1_UART_Init();
  printf("\r\n--- RT-SPARK BOOTED ---\r\n");
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* Create the thread(s) */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  while (1)
  {
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C2 Initialization Function
  * @retval None
  */
static void MX_I2C2_Init(void)
{
  __HAL_RCC_I2C2_FORCE_RESET();
  __HAL_RCC_I2C2_RELEASE_RESET();

  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_I2C1_Init(void)
{
  __HAL_RCC_I2C1_FORCE_RESET();
  __HAL_RCC_I2C1_RELEASE_RESET();

  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  char jsonBuffer[150];
  uint32_t sample = 0U;

  /* Infinite loop */
  for(;;)
  {
    const float pi = 3.14159265f;
    const uint32_t cycleSamples = 20U;
    const bool running = ((sample / cycleSamples) % 2U) != 0U;
    const float frequencyHz = running ? 2.5f : 1.8f;
    const float amplitude = running ? 1.5f : 0.8f;
    const float phase = 2.0f * pi * frequencyHz * ((float)sample);
    float ax = amplitude * sinf(phase);
    float ay = 0.15f * sinf(phase * 0.5f + pi * 0.25f);
    float az = 9.80665f + 0.25f * cosf(phase);
    float gx = 0.0f;
    float gy = 0.0f;
    float gz = (running ? 2.0f : 1.0f) * cosf(phase);

    printf("[HEARTBEAT] Board is alive. Sensor status: synthetic\r\n");
    snprintf(jsonBuffer, sizeof(jsonBuffer),
        "{\"accel_x\":%.2f, \"accel_y\":%.2f, \"accel_z\":%.2f, \"gyro_x\":%.2f, \"gyro_y\":%.2f, \"gyro_z\":%.2f}",
        ax, ay, az, gx, gy, gz);
    printf("JSON:%s\r\n", jsonBuffer);

    sample++;
    osDelay(1000U);
  }
  /* USER CODE END 5 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
