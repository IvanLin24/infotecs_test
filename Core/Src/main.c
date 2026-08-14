#include "main.h"
#include "app.h"
#include "task.h"

SPI_HandleTypeDef hspi1;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);

#define APP_ROLE               EXCHANGE_MASTER
#define APP_BLOB_ID            1u
#define APP_SEED               0x17u
#define APP_EXPECTED_BLOB_ID   2u

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_SPI1_Init();

    app_start(&hspi1,
              APP_ROLE,
              APP_BLOB_ID,
              APP_SEED,
              APP_EXPECTED_BLOB_ID);

    vTaskStartScheduler();

    for (;;)
        ;
}

static void SystemClock_Config(void)
{
    /* Здесь должна быть настройка тактирования, сгенерированная STM32CubeMX. */
}

static void MX_GPIO_Init(void)
{
    /* Здесь должна быть настройка GPIO и NSS, сгенерированная STM32CubeMX. */
}

static void MX_DMA_Init(void)
{
    /* Здесь должна быть настройка DMA и необходимых прерываний. */
}

static void MX_SPI1_Init(void)
{
    /* Здесь должна быть настройка SPI для конкретной платы. */
}
