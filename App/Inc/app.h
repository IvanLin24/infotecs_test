#ifndef APP_H
#define APP_H

#include "main.h"
#include "exchange.h"

void app_start(SPI_HandleTypeDef *hspi,
               exchange_role_t role,
               uint8_t blob_id,
               uint8_t seed,
               uint8_t expected_blob_id);

#endif
