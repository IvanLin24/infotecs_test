#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../App/Inc/exchange.h"
#include "../App/Inc/crc16.h"

static void transfer_blob(exchange_state_t *sender,
                          exchange_state_t *receiver,
                          unsigned *frame_counter)
{
    spi_frame_t frame;

    while (sender->next_sequence < CHUNK_COUNT) {
        exchange_build_frame(sender, &frame);

        if (!exchange_accept_frame(receiver, &frame)) {
            printf("ERROR: data frame %u rejected\n", *frame_counter);
            return;
        }

        sender->next_sequence++;
        (*frame_counter)++;
    }

    protocol_make_done(&frame,
                       sender->tx_blob_id,
                       CHUNK_COUNT,
                       sender->tx_crc);

    if (!exchange_accept_frame(receiver, &frame)) {
        printf("ERROR: DONE frame rejected\n");
        return;
    }
    (*frame_counter)++;
}

static int equal_blob(const uint8_t *a, const uint8_t *b)
{
    return memcmp(a, b, BLOB_SIZE) == 0;
}

static int test_crc16(void)
{
    static const uint8_t data[] = "123456789";
    return crc16_ccitt(data, sizeof(data) - 1u) == 0x29B1u;
}

int main(void)
{
    exchange_state_t master;
    exchange_state_t slave;

    if (!test_crc16()) {
        printf("ERROR: CRC16 test failed\n");
        return 1;
    }

    exchange_init(&master, EXCHANGE_MASTER, 1u, 0x17u, 2u);
    exchange_init(&slave,  EXCHANGE_SLAVE,  2u, 0xA9u, 1u);

    printf("MASTER: blob CRC = 0x%04X\n", master.tx_crc);
    printf("SLAVE : blob CRC = 0x%04X\n", slave.tx_crc);

    if (master.tx_crc == slave.tx_crc ||
        equal_blob(master.tx_blob, slave.tx_blob)) {
        printf("ERROR: test blobs are unexpectedly identical\n");
        return 1;
    }

    unsigned master_frames = 0;
    unsigned slave_frames = 0;

    transfer_blob(&master, &slave, &master_frames);
    transfer_blob(&slave, &master, &slave_frames);

    printf("MASTER: received CRC = 0x%04X\n",
           crc16_ccitt(master.rx_blob, BLOB_SIZE));
    printf("SLAVE : received CRC = 0x%04X\n",
           crc16_ccitt(slave.rx_blob, BLOB_SIZE));

    if (!exchange_finished(&master) ||
        !exchange_finished(&slave) ||
        !equal_blob(master.tx_blob, slave.rx_blob) ||
        !equal_blob(slave.tx_blob, master.rx_blob) ||
        master_frames != CHUNK_COUNT + 1u ||
        slave_frames != CHUNK_COUNT + 1u) {
        printf("RESULT: FAIL\n");
        return 1;
    }

    spi_frame_t bad;
    exchange_state_t check;
    exchange_init(&check, EXCHANGE_SLAVE, 2u, 0x17u, 1u);
    exchange_build_frame(&check, &bad);
    bad.payload[0] ^= 0x01u;

    if (protocol_validate(&bad)) {
        printf("ERROR: CRC failed to detect payload corruption\n");
        return 1;
    }

    exchange_build_frame(&check, &bad);
    bad.sof ^= 0x01u;

    if (protocol_validate(&bad)) {
        printf("ERROR: CRC failed to detect header corruption\n");
        return 1;
    }

    printf("Frames master->slave: %u\n", master_frames);
    printf("Frames slave->master: %u\n", slave_frames);
    printf("RESULT: PASS\n");

    return 0;
}
