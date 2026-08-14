#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

#define BLOB_SIZE           10240u
#define CHUNK_SIZE          64u
#define CHUNK_COUNT         (BLOB_SIZE / CHUNK_SIZE)

#define FRAME_SOF           0xA5u
#define FRAME_VERSION       1u
#define FRAME_WIRE_SIZE     74u

typedef enum {
    FRAME_DATA = 0x01u,
    FRAME_DONE = 0x03u
} frame_type_t;

#pragma pack(push, 1)
typedef struct {
    uint8_t  sof;
    uint8_t  version;
    uint8_t  type;
    uint8_t  blob_id;
    uint16_t sequence;
    uint8_t  length;
    uint8_t  reserved;
    uint8_t  payload[CHUNK_SIZE];
    uint16_t crc16;
} spi_frame_t;
#pragma pack(pop)

_Static_assert(BLOB_SIZE % CHUNK_SIZE == 0u, "Размер блока должен делиться на размер части без остатка");
_Static_assert(sizeof(spi_frame_t) == FRAME_WIRE_SIZE, "Размер SPI-кадра должен быть ровно 74 байта");

uint16_t protocol_crc(const spi_frame_t *frame);
void protocol_make_data(spi_frame_t *frame,
                        uint8_t blob_id,
                        uint16_t sequence,
                        const uint8_t payload[CHUNK_SIZE]);
void protocol_make_done(spi_frame_t *frame,
                        uint8_t blob_id,
                        uint16_t sequence,
                        uint16_t blob_crc);
int protocol_validate(const spi_frame_t *frame);

#endif
