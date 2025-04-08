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

// Microphone I2S Configuration
#define I2S_SD   10
#define I2S_WS   11
#define I2S_SCK  12
#define I2S_PORT I2S_NUM_0

#define bufferCnt 10 // DMA Buffers
#define bufferLen 1024 // DMA Buffer Size
int16_t sBuffer[bufferLen]; // DMA Buffer - 32-bit size for 24-bit PCM Data

static const char *I2S_MIC_TAG = "I2S_MIC";

void mic_task(void *arg) {
    // I2S Driver Install
    const i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,
        .sample_rate = 44100,
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

    // I2S Set Pin
    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };
    ESP_ERROR_CHECK(i2s_set_pin(I2S_PORT, &pin_config));

    // I2S Start
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
            ESP_LOGE(I2S_MIC_TAG, "i2s_read failed: %s", esp_err_to_name(result));
        }

        // Small delay between prints to prevent flooding
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    xTaskCreatePinnedToCore(mic_task, "mic_task", 10000, NULL, 1, NULL, 1);
}

 // #include <stdio.h>
// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/i2s.h"
// #include "esp_log.h"

// #define I2S_SD   10
// #define I2S_WS   11
// #define I2S_SCK  12
// #define I2S_PORT I2S_NUM_0

// #define bufferCnt 10
// #define bufferLen 1024
// int16_t sBuffer[bufferLen];

// static const char *TAG = "I2S_MIC";

// void i2s_install() {
//     const i2s_config_t i2s_config = {
//         .mode = I2S_MODE_MASTER | I2S_MODE_RX,
//         .sample_rate = 44100,
//         .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
//         .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
//         .communication_format = I2S_COMM_FORMAT_I2S,
//         .intr_alloc_flags = 0,
//         .dma_buf_count = bufferCnt,
//         .dma_buf_len = bufferLen,
//         .use_apll = false,
//         .tx_desc_auto_clear = false,
//         .fixed_mclk = 0
//     };
//     ESP_ERROR_CHECK(i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL));
// }

// void i2s_setpin() {
//     const i2s_pin_config_t pin_config = {
//         .bck_io_num = I2S_SCK,
//         .ws_io_num = I2S_WS,
//         .data_out_num = -1,
//         .data_in_num = I2S_SD
//     };
//     ESP_ERROR_CHECK(i2s_set_pin(I2S_PORT, &pin_config));
// }

// void mic_task(void *arg) {
//     i2s_install();
//     i2s_setpin();
//     i2s_start(I2S_PORT);

//     size_t bytesIn = 0;
//     while (1) {
//         esp_err_t result = i2s_read(I2S_PORT, &sBuffer, bufferLen * sizeof(int16_t), &bytesIn, portMAX_DELAY);
//         if (result == ESP_OK) {
//             int sample_count = bytesIn / sizeof(int16_t);
//             int64_t sum = 0;

//             for (int i = 0; i < sample_count; i++) {
//                 sum += abs(sBuffer[i]);
//             }

//             int average = sum / sample_count;
//             printf("Average amplitude: %d\n", average);
//         } else {
//             ESP_LOGE(TAG, "i2s_read failed: %s", esp_err_to_name(result));
//         }

//         // Small delay between prints to prevent flooding
//         vTaskDelay(100 / portTICK_PERIOD_MS);
//     }
// }

// void app_main(void) {
//     xTaskCreatePinnedToCore(mic_task, "mic_task", 10000, NULL, 1, NULL, 1);
// }

// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_log.h"

// #include "driver/spi_slave.h"
// #include "driver/spi_common.h"
// #include "driver/gpio.h"

// #define TAG "SPI_SLAVE"

// #define SPI_HOST       SPI3_HOST  // VSPI
// #define DMA_CH         SPI_DMA_CH_AUTO
// #define QUEUE_SIZE     3
// #define TRANSFER_SIZE  1024       // bytes

// // GPIOs for SPI3 (VSPI)
// #define PIN_NUM_MOSI   35
// #define PIN_NUM_MISO   37
// #define PIN_NUM_SCLK   36
// #define PIN_NUM_CS     39


// // Test buffer (replace with I2S audio data later)
// static uint8_t tx_buffer[TRANSFER_SIZE];

// // Prepare dummy data (counting pattern)
// void fill_tx_buffer() {
//     for (int i = 0; i < TRANSFER_SIZE; ++i) {
//         tx_buffer[i] = i & 0xFF;
//     }
//     ESP_LOGI(TAG, "tx_buffer[0..3] = %02X %02X %02X %02X",
//         tx_buffer[0], tx_buffer[1], tx_buffer[2], tx_buffer[3]);
    
// }

// void spi_slave_task(void *arg) {
//     esp_err_t ret;

//     // Configure SPI bus
//     spi_bus_config_t bus_cfg = {
//         .mosi_io_num = PIN_NUM_MOSI,
//         .miso_io_num = PIN_NUM_MISO,
//         .sclk_io_num = PIN_NUM_SCLK,
//         .quadwp_io_num = -1,
//         .quadhd_io_num = -1
//     };

//     // Configure SPI slave interface
//     spi_slave_interface_config_t slave_cfg = {
//         .spics_io_num = PIN_NUM_CS,
//         .flags = 0,
//         .queue_size = QUEUE_SIZE,
//         .mode = 0,
//     };

//     ESP_LOGI(TAG, "Initializing SPI slave...");

//     ret = spi_slave_initialize(SPI_HOST, &bus_cfg, &slave_cfg, DMA_CH);
//     if (ret != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to init SPI slave: %s", esp_err_to_name(ret));
//         vTaskDelete(NULL);
//     }

//     ESP_LOGI(TAG, "SPI slave initialized. Ready to transmit data.");

//     while (1) {
//         spi_slave_transaction_t trans = {
//             .length = TRANSFER_SIZE * 8,  // in bits
//             .tx_buffer = tx_buffer,
//             .rx_buffer = NULL
//         };

//         // Fill test data each time (optional)
//         fill_tx_buffer();

//         // Wait for master to initiate transfer
//         ret = spi_slave_transmit(SPI_HOST, &trans, portMAX_DELAY);
//         if (ret != ESP_OK) {
//             ESP_LOGE(TAG, "SPI transfer error: %s", esp_err_to_name(ret));
//         } else {
//             ESP_LOGI(TAG, "Sent %d bytes to master", TRANSFER_SIZE);
//         }
//     }
// }

// void app_main(void) {
//     xTaskCreate(spi_slave_task, "spi_slave_task", 4096, NULL, 5, NULL);
// }
