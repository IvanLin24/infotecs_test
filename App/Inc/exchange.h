#ifndef EXCHANGE_H
#define EXCHANGE_H

#include <stdint.h>
#include "protocol.h"

typedef enum {
    EXCHANGE_MASTER = 0,
    EXCHANGE_SLAVE  = 1
} exchange_role_t;

typedef struct {
    uint8_t tx_blob[BLOB_SIZE];
    uint8_t rx_blob[BLOB_SIZE];

    uint16_t tx_crc;
    uint16_t expected_crc;
    uint16_t next_sequence;
    uint16_t received_chunks;

    exchange_role_t role;
    uint8_t tx_blob_id;
    uint8_t expected_blob_id;
    uint8_t completed;
    uint8_t crc_ok;
    uint8_t done_received;
} exchange_state_t;

void exchange_init(exchange_state_t *state,
                   exchange_role_t role,
                   uint8_t blob_id,
                   uint8_t seed,
                   uint8_t expected_blob_id);

void exchange_build_frame(exchange_state_t *state,
                          spi_frame_t *frame);

int exchange_accept_frame(exchange_state_t *state,
                          const spi_frame_t *frame);

int exchange_finished(const exchange_state_t *state);

#endif
