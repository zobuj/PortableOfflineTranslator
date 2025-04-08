/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "esp_log.h"

#define I2S_SD   10
#define I2S_WS   11
#define I2S_SCK  12
#define I2S_PORT I2S_NUM_0

#define bufferCnt 10
#define bufferLen 1024
int16_t sBuffer[bufferLen];

static const char *TAG = "I2S_MIC";

void i2s_install() {
    const i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,
        // .sample_rate = 44100,
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = bufferCnt,
        .dma_buf_len = bufferLen,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    ESP_ERROR_CHECK(i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL));
}

void i2s_setpin() {
    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };
    ESP_ERROR_CHECK(i2s_set_pin(I2S_PORT, &pin_config));
}

void mic_task(void *arg) {
    i2s_install();
    i2s_setpin();
    i2s_start(I2S_PORT);

    size_t bytesIn = 0;
    while (1) {
        esp_err_t result = i2s_read(I2S_PORT, &sBuffer, bufferLen * sizeof(int16_t), &bytesIn, portMAX_DELAY);
        if (result == ESP_OK) {
            int sample_count = bytesIn / sizeof(int16_t);
            int64_t sum = 0;

            for (int i = 0; i < sample_count; i++) {
                sum += abs(sBuffer[i]);
            }

            int average = sum / sample_count;
            printf("Average amplitude: %d\n", average);
        } else {
            ESP_LOGE(TAG, "i2s_read failed: %s", esp_err_to_name(result));
        }

        // Small delay between prints to prevent flooding
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    xTaskCreatePinnedToCore(mic_task, "mic_task", 10000, NULL, 1, NULL, 1);
}


// #include <string.h>
// #include "driver/spi_common.h"
// #include "driver/spi_slave.h"
// #include "driver/gpio.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_log.h"

// #define RCV_BUFFER_SIZE 32
// #define SEND_BUFFER_SIZE 1024
// #define SPI_HOST HSPI_HOST

// static const char *TAG = "SPI_SLAVE";

// uint8_t audio_buffer[SEND_BUFFER_SIZE]; // This would come from your I2S mic

// void prepare_audio_chunk() {
//     for (int i = 0; i < SEND_BUFFER_SIZE; ++i) {
//         audio_buffer[i] = i & 0xFF;  // Dummy data for now
//     }
// }

// void spi_slave_task(void *arg) {
//     esp_err_t ret;
//     spi_slave_transaction_t t;

//     spi_slave_interface_config_t spi_cfg = {
//         .mode = 0,
//         .spics_io_num = GPIO_NUM_5,
//         .queue_size = 3,
//         .flags = 0,
//     };

//     spi_bus_config_t bus_cfg = {
//         .mosi_io_num = GPIO_NUM_23,
//         .miso_io_num = GPIO_NUM_19,
//         .sclk_io_num = GPIO_NUM_18,
//         .quadwp_io_num = -1,
//         .quadhd_io_num = -1
//     };

//     ret = spi_slave_initialize(SPI_HOST, &bus_cfg, &spi_cfg, SPI_DMA_CH_AUTO);
//     assert(ret == ESP_OK);

//     prepare_audio_chunk();

//     while (1) {
//         memset(&t, 0, sizeof(t));
//         t.length = SEND_BUFFER_SIZE * 8;
//         t.tx_buffer = audio_buffer;

//         ESP_LOGI(TAG, "Waiting for master to read...");
//         ret = spi_slave_transmit(SPI_HOST, &t, portMAX_DELAY);
//         if (ret == ESP_OK) {
//             ESP_LOGI(TAG, "Chunk sent");
//         }
//     }
// }

// void app_main(void) {
//     xTaskCreate(spi_slave_task, "spi_slave_task", 4096, NULL, 5, NULL);
// }
