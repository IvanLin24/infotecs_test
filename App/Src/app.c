#include "app.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include <string.h>

typedef struct {
    SPI_HandleTypeDef *hspi;
    SemaphoreHandle_t dma_done;
    volatile uint8_t dma_error;
} spi_context_t;

static spi_context_t g_spi;
static QueueHandle_t g_requests;
static exchange_state_t g_exchange;

static void dma_complete_from_isr(void)
{
    BaseType_t higher = pdFALSE;
    xSemaphoreGiveFromISR(g_spi.dma_done, &higher);
    portYIELD_FROM_ISR(higher);
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == g_spi.hspi)
        dma_complete_from_isr();
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == g_spi.hspi) {
        g_spi.dma_error = 1u;
        dma_complete_from_isr();
    }
}

static int spi_transfer_async(uint8_t *tx, uint8_t *rx)
{
    g_spi.dma_error = 0u;

    while (xSemaphoreTake(g_spi.dma_done, 0u) == pdTRUE)
        ;

    if (HAL_SPI_TransmitReceive_DMA(g_spi.hspi,
                                    tx, rx,
                                    FRAME_WIRE_SIZE) != HAL_OK)
        return -1;

    if (xSemaphoreTake(g_spi.dma_done,
                       pdMS_TO_TICKS(1000)) != pdTRUE) {
        (void)HAL_SPI_Abort(g_spi.hspi);
        return -2;
    }

    return g_spi.dma_error ? -3 : 0;
}

static void scheduler_task(void *argument)
{
    (void)argument;

    const TickType_t period =
        g_exchange.role == EXCHANGE_MASTER
            ? pdMS_TO_TICKS(200)
            : pdMS_TO_TICKS(333);
    const uint8_t request = 1u;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        xQueueSend(g_requests, &request, portMAX_DELAY);
        vTaskDelayUntil(&last_wake, period);
    }
}

static void spi_owner_task(void *argument)
{
    (void)argument;

    uint8_t request;
    spi_frame_t tx;
    spi_frame_t rx;

    for (;;) {
        if (xQueueReceive(g_requests, &request, portMAX_DELAY) != pdTRUE)
            continue;

        if (g_exchange.completed)
            continue;

        exchange_build_frame(&g_exchange, &tx);
        memset(&rx, 0, sizeof(rx));

        if (spi_transfer_async((uint8_t *)&tx, (uint8_t *)&rx) != 0)
            continue;

        if (exchange_accept_frame(&g_exchange, &rx))
            ++g_exchange.next_sequence;

        if (g_exchange.next_sequence == CHUNK_COUNT &&
            !g_exchange.completed) {
            protocol_make_done(&tx,
                               g_exchange.tx_blob_id,
                               CHUNK_COUNT,
                               g_exchange.tx_crc);
            memset(&rx, 0, sizeof(rx));

            if (spi_transfer_async((uint8_t *)&tx, (uint8_t *)&rx) == 0)
                (void)exchange_accept_frame(&g_exchange, &rx);
        }
    }
}

void app_start(SPI_HandleTypeDef *hspi,
               exchange_role_t role,
               uint8_t blob_id,
               uint8_t seed,
               uint8_t expected_blob_id)
{
    g_spi.hspi = hspi;
    g_spi.dma_done = xSemaphoreCreateBinary();
    g_spi.dma_error = 0u;

    g_requests = xQueueCreate(8, sizeof(uint8_t));

    exchange_init(&g_exchange,
                  role,
                  blob_id,
                  seed,
                  expected_blob_id);

    xTaskCreate(scheduler_task, "schedule", 512, NULL, 2, NULL);
    xTaskCreate(spi_owner_task, "spi", 1024, NULL, 3, NULL);
}
