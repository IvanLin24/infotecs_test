#include "exchange.h"
#include "crc16.h"
#include <string.h>

void exchange_init(exchange_state_t *state,
                   exchange_role_t role,
                   uint8_t blob_id,
                   uint8_t seed,
                   uint8_t expected_blob_id)
{
    memset(state, 0, sizeof(*state));
    state->role = role;
    state->tx_blob_id = blob_id;
    state->expected_blob_id = expected_blob_id;

    for (unsigned i = 0; i < BLOB_SIZE; ++i)
        state->tx_blob[i] = (uint8_t)((i * 37u + seed) & 0xFFu);

    state->tx_crc = crc16_ccitt(state->tx_blob, BLOB_SIZE);
    state->expected_crc = state->tx_crc;
}

void exchange_build_frame(exchange_state_t *state,
                          spi_frame_t *frame)
{
    const uint8_t *payload =
        &state->tx_blob[(size_t)state->next_sequence * CHUNK_SIZE];

    protocol_make_data(frame,
                       state->tx_blob_id,
                       state->next_sequence,
                       payload);
}

int exchange_accept_frame(exchange_state_t *state,
                          const spi_frame_t *frame)
{
    if (!protocol_validate(frame))
        return 0;

    if (frame->blob_id != state->expected_blob_id)
        return 0;

    if (frame->type == FRAME_DATA) {
        if (frame->sequence != state->received_chunks)
            return 0;
        if (state->received_chunks >= CHUNK_COUNT)
            return 0;

        memcpy(&state->rx_blob[(size_t)frame->sequence * CHUNK_SIZE],
               frame->payload,
               CHUNK_SIZE);

        ++state->received_chunks;
        return 1;
    }

    if (frame->type == FRAME_DONE) {
        if (state->received_chunks != CHUNK_COUNT)
            return 0;
        if (frame->sequence != CHUNK_COUNT)
            return 0;

        state->expected_crc =
            ((uint16_t)frame->payload[0] << 8) | frame->payload[1];

        uint16_t received_crc =
            crc16_ccitt(state->rx_blob, BLOB_SIZE);

        state->crc_ok = (received_crc == state->expected_crc);
        state->done_received = 1u;
        state->completed = 1u;
        return 1;
    }

    return 0;
}

int exchange_finished(const exchange_state_t *state)
{
    return state->completed && state->crc_ok;
}
