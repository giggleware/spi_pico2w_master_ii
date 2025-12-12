// Copyright (c) 2021 Michael Stoops. All rights reserved.
// Portions copyright (c) 2021 Raspberry Pi (Trading) Ltd.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
// following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
//    disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
//    following disclaimer in the documentation and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
//    products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// SPDX-License-Identifier: BSD-3-Clause
//
// Example of an SPI bus master using the PL022 SPI interface

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/binary_info.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "pico/time.h"
#include "spi_master.h"
#include "wifi_master.h"
#include "http_helper.h"
#include "core_helper.h"
#include <limits.h>

void print_byte_binary(unsigned char byte)
{
    // Determine the number of bits in a byte (usually 8)
    int bits = sizeof(byte) * CHAR_BIT;

    for (int i = bits - 1; i >= 0; i--)
    {
        // Create a mask with a 1 at the i-th bit position
        unsigned char mask = 1U << i;

        // Use bitwise AND to check if the i-th bit is set
        if (byte & mask)
        {
            printf("1");
        }
        else
        {
            printf("0");
        }
    }
    printf("\n"); // Add a newline character at the end
}

void handle_error(const char *module, struct pbuf *p)
{
    printf("Error sending %s\n", module);
    pbuf_free(p);
    p = NULL;
    // You can also add additional error handling here, such as closing the connection
}

// ---------------- Helper: chunked send ----------------
void tcp_send_chunks(struct tcp_pcb *pcb, const char *data)
{
    size_t len = strlen(data);
    size_t offset = 0;
    const size_t chunk_size = 512; // safe chunk for lwIP

    while (offset < len)
    {
        size_t to_send = (len - offset) > chunk_size ? chunk_size : (len - offset);
        err_t w = tcp_write(pcb, data + offset, to_send, TCP_WRITE_FLAG_COPY);
        if (w != ERR_OK)
        {
            // If write fails, try to flush and break
            tcp_output(pcb);
            break;
        }
        tcp_output(pcb);
        offset += to_send;
    }
    // ensure data flushed and close connection
    tcp_close(pcb);
}

void printbuf(uint8_t buf[], size_t len)
{
    size_t i;
    for (i = 0; i < len; ++i)
    {
        if (i % 16 == 15)
            printf("%02x\n", buf[i]);
        else
            printf("%02x ", buf[i]);
    }

    // append trailing newline if there isn't one
    if (i % 16)
    {
        putchar('\n');
    }
}

int main()
{
    // Enable UART so we can print
    stdio_init_all();
    multicore_launch_core1(core1_main);
#if !defined(SPI_PORT) || !defined(PIN_SCK) || !defined(PIN_MOSI) || !defined(PIN_MISO) || !defined(PIN_CS)
#warning spi/spi_master example requires a board with SPI pins
    puts("Default SPI pins were not defined");
#else

    // main loop: poll cyw43 and do SPI at intervals
    absolute_time_t last_spi = get_absolute_time();
    const uint32_t spi_interval_us = 50000; // 50 ms

    spi_setup();

    uint8_t out_buf[BUF_LEN], in_buf[BUF_LEN];

    // Initialize output buffer
    for (size_t i = 0; i < BUF_LEN; ++i)
    {
        // bit-inverted from i. The values should be: {0xff, 0xfe, 0xfd...}
        out_buf[i] = ~i;
    }

    uint16_t i = 0;
    uint16_t new_in[3] = {0x00, 0x00, 0x00};
    uint8_t last_in_buf[3] = {0x00, 0x00, 0x00};
    uint32_t received_value = 0x000000;
    for (size_t i = 0;; ++i) // the main loop for Core0.
    {
        last_in_buf[2] = slave_input;

        // The main transaction:
        received_value = spi_readwrite(last_in_buf, in_buf);
        uint8_t received_data_buf[3];
        // printf("received_data_buf[0] : %02X,\nreceived_data_buf[1] : %02X,\nreceived_data_buf[2] : %02X\n", received_data_buf[0], received_data_buf[1], received_data_buf[2]);
        slave_input = 0;

        uint8_t slave_led_state = (received_value >> 16) & 0xFF;
        uint16_t temp = ((uint16_t)in_buf[1] << 8) | in_buf[0];
        float slave_temp = getTemperature(temp);
        float slave_temp_f = (slave_temp * (9 / 5) + 32); // Converts C to F.

        current_led_byte = in_buf[2];
        current_temp_raw = slave_temp_f;

        // uint32_t value = ((uint32_t)in_buf[2] << 16) | ((uint32_t)in_buf[1] << 8) | (uint32_t)in_buf[0];

        slave_output = in_buf[2];
        i = 0;

        // Sleep for ten seconds so you get a chance to read the output.

        sleep_ms(500); // Master sends a transmission every 1 second.
        // The master can wait one second, or 10 seconds, or one minute. It doesn't care, because it's the boss.
        // The slave has no choice but to wait.
    }
#endif
}