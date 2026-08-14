#include "protocol.h"
#include "crc16.h"
#include <string.h>

uint16_t protocol_crc(const spi_frame_t *frame)
{
    return crc16_ccitt((const uint8_t *)frame, offsetof(spi_frame_t, crc16));
}

void protocol_make_data(spi_frame_t *frame,
                        uint8_t blob_id,
                        uint16_t sequence,
                        const uint8_t payload[CHUNK_SIZE])
{
    memset(frame, 0, sizeof(*frame));
    frame->sof = FRAME_SOF;
    frame->version = FRAME_VERSION;
    frame->type = FRAME_DATA;
    frame->blob_id = blob_id;
    frame->sequence = sequence;
    frame->length = CHUNK_SIZE;
    memcpy(frame->payload, payload, CHUNK_SIZE);
    frame->crc16 = protocol_crc(frame);
}

void protocol_make_done(spi_frame_t *frame,
                        uint8_t blob_id,
                        uint16_t sequence,
                        uint16_t blob_crc)
{
    memset(frame, 0, sizeof(*frame));
    frame->sof = FRAME_SOF;
    frame->version = FRAME_VERSION;
    frame->type = FRAME_DONE;
    frame->blob_id = blob_id;
    frame->sequence = sequence;
    frame->length = 2u;
    frame->payload[0] = (uint8_t)(blob_crc >> 8);
    frame->payload[1] = (uint8_t)(blob_crc & 0xFFu);
    frame->crc16 = protocol_crc(frame);
}

int protocol_validate(const spi_frame_t *frame)
{
    if (frame->sof != FRAME_SOF) return 0;
    if (frame->version != FRAME_VERSION) return 0;
    if (frame->reserved != 0u) return 0;
    if (frame->type != FRAME_DATA && frame->type != FRAME_DONE) return 0;
    if (frame->type == FRAME_DATA && frame->length != CHUNK_SIZE) return 0;
    if (frame->type == FRAME_DONE && frame->length != 2u) return 0;
    return frame->crc16 == protocol_crc(frame);
}
