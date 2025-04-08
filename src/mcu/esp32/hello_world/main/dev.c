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