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
#include "driver/spi_slave.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <math.h>

#define PI 3.14159265

// Speaker I2S Configuration
#define SPEAKER_I2S_SD   17
#define SPEAKER_I2S_WS   18
#define SPEAKER_I2S_SCK  8
#define SPEAKER_I2S_PORT I2S_NUM_1

// Microphone I2S Configuration
#define I2S_SD   5
#define I2S_WS   6
#define I2S_SCK  7
#define I2S_PORT I2S_NUM_0

#define bufferCnt 16 // DMA Buffers
#define bufferLen 1024 // DMA Buffer Size
int16_t sBuffer[bufferLen]; // DMA Buffer - 32-bit size for 24-bit PCM Data
int16_t circular_buffer[bufferCnt * bufferLen];
volatile int write_index = 0;
volatile int read_index = 0;
#define rx_buffer_len 1
int16_t rx_buffer[rx_buffer_len];

// SPI Transfer Configuration
#define PIN_NUM_MOSI   35
#define PIN_NUM_MISO   37
#define PIN_NUM_SCLK   36
#define PIN_NUM_CS     39

#define SPI_HOST       SPI3_HOST  // VSPI
#define DMA_CH         SPI_DMA_CH_AUTO
#define QUEUE_SIZE     3
#define TRANSFER_SIZE  1024       // bytes

// GPIO Interrupt Pin
// #define INT_GPIO GPIO_NUM_4 // SPI Transfer Pin
#define INT_GPIO GPIO_NUM_16 // SPI Transfer Pin
#define START_GPIO GPIO_NUM_15

// TAGS
static const char *I2S_MIC_TAG = "I2S_MIC";
static const char *SPI_TRANSFER_TAG = "SPI_SLAVE";
static const char *START_BUTTON_TAG = "START_BUTTON";

typedef enum {
    STATE_INIT,
    STATE_RECORDING,
    STATE_WAIT_RESP
} system_state_t;

volatile system_state_t curr_state = STATE_INIT;

void mic_setup() {
    // I2S Driver Install
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
}

void trigger_transfer_interrupt() {
    gpio_set_level(INT_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(10)); // short pulse
    gpio_set_level(INT_GPIO, 0);
}

void spi_interrupt_pin_setup() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << INT_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(INT_GPIO, 0); // Ensure it's low initially
}

void spi_transfer_setup() {
    spi_interrupt_pin_setup();

    esp_err_t ret;

    // Configure SPI bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    // Configure SPI slave interface
    spi_slave_interface_config_t slave_cfg = {
        .spics_io_num = PIN_NUM_CS,
        .flags = 0,
        .queue_size = QUEUE_SIZE,
        .mode = 0,
    };

    ESP_LOGI(SPI_TRANSFER_TAG, "Initializing SPI slave...");

    ret = spi_slave_initialize(SPI_HOST, &bus_cfg, &slave_cfg, DMA_CH);
    if (ret != ESP_OK) {
        ESP_LOGE(SPI_TRANSFER_TAG, "Failed to init SPI slave: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
    }

    ESP_LOGI(SPI_TRANSFER_TAG, "SPI slave initialized. Ready to transmit data.");

}

void mic_task(void *arg) {

    mic_setup();

    size_t bytesIn = 0;
    while (1) {

        if (curr_state != STATE_RECORDING) {
            vTaskDelay(50 / portTICK_PERIOD_MS);
            continue;
        }

        esp_err_t result = i2s_read(I2S_PORT, &sBuffer, bufferLen * sizeof(int16_t), &bytesIn, portMAX_DELAY);

        if (result == ESP_OK) {
            int next_write = (write_index + 1) % bufferCnt;

            // If buffer is full, drop the oldest block by advancing the read_index
            if (next_write == read_index) {
                ESP_LOGW(I2S_MIC_TAG, "Buffer overflow: overwriting oldest block");
                read_index = (read_index + 1) % bufferCnt;
            }

            // Copy the new audio block into circular buffer
            memcpy(&circular_buffer[write_index * bufferLen], sBuffer, bufferLen * sizeof(int16_t));
            write_index = next_write;

            // // Trigger the SPI master (Raspberry Pi) via interrupt
            // trigger_transfer_interrupt();

            // Optional: log a few samples for debugging
            ESP_LOGI(I2S_MIC_TAG, "Sample[0..4]: %d %d %d %d %d", sBuffer[0], sBuffer[1], sBuffer[2], sBuffer[3], sBuffer[4]);
        } else {
            ESP_LOGE(I2S_MIC_TAG, "i2s_read failed: %s", esp_err_to_name(result));
        }

        // // Small delay between prints to prevent flooding
        // vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void spi_task(void *arg) {
    spi_transfer_setup();

    while (1) {
        if (curr_state == STATE_WAIT_RESP) {
            // If the Pi sends data back, receive and handle it here
            spi_slave_transaction_t recv = {
                .length = rx_buffer_len * sizeof(int16_t) * 8,
                .rx_buffer = rx_buffer,
                .tx_buffer = NULL
            };

            esp_err_t ret = spi_slave_transmit(SPI_HOST, &recv, portMAX_DELAY);
            if (ret == ESP_OK) {
                ESP_LOGI(SPI_TRANSFER_TAG, "Received response from Pi");
                // Process received data
                curr_state = STATE_INIT;  // Reset to init or next logical state

                // Clear circular buffer pointers
                write_index = 0;
                read_index = 0;

            } else if (ret != ESP_OK) {
                ESP_LOGE(SPI_TRANSFER_TAG, "SPI receive error: %s", esp_err_to_name(ret));
            }
        } else if (curr_state == STATE_RECORDING && write_index != read_index) {
            spi_slave_transaction_t trans = {
                .length = bufferLen * sizeof(int16_t) * 8,
                .tx_buffer = &circular_buffer[read_index * bufferLen],
                .rx_buffer = NULL
            };

            esp_err_t ret = spi_slave_transmit(SPI_HOST, &trans, portMAX_DELAY);
            if (ret == ESP_OK) {
                ESP_LOGI(SPI_TRANSFER_TAG, "Sent SPI block");
                read_index = (read_index + 1) % bufferCnt;
            } else {
                ESP_LOGE(SPI_TRANSFER_TAG, "SPI transmit error: %s", esp_err_to_name(ret));
            }
        } else {
            vTaskDelay(10 / portTICK_PERIOD_MS); // Wait a bit if there's no data
        }
    }
}

void start_button_task(void *arg) {
    gpio_set_direction(START_GPIO, GPIO_MODE_INPUT);

    bool last_state = false;
    while (1) {
        bool pressed = gpio_get_level(START_GPIO);

        switch (curr_state) {
            case STATE_INIT:
                if(pressed && !last_state) {
                    curr_state = STATE_RECORDING;
                    // Trigger the SPI master (Raspberry Pi) via interrupt
                    trigger_transfer_interrupt(); // Start Polling
                    ESP_LOGI(START_BUTTON_TAG, "Button pressed, transitioning to RECORDING");
                }
                break;
            case STATE_RECORDING:
                if (!pressed && last_state) {
                    curr_state = STATE_WAIT_RESP;
                    // Trigger the SPI master (Raspberry Pi) via interrupt
                    trigger_transfer_interrupt(); // End Polling
                    ESP_LOGI(START_BUTTON_TAG, "Button released, transitioning to WAIT_RESPONSE");
                }
                break;
            case STATE_WAIT_RESP:
                break;
        }




        last_state = pressed;
        vTaskDelay(50 / portTICK_PERIOD_MS);  // debounce
    }
}


void speaker_setup() {
    // I2S Driver Install
    const i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = 44100,
        // .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    ESP_ERROR_CHECK(i2s_driver_install(SPEAKER_I2S_PORT, &i2s_config, 0, NULL));

    // I2S Set Pin
    const i2s_pin_config_t pin_config = {
        .bck_io_num = SPEAKER_I2S_SCK,
        .ws_io_num = SPEAKER_I2S_WS,
        .data_out_num = SPEAKER_I2S_SD,
        .data_in_num = -1
    };
    ESP_ERROR_CHECK(i2s_set_pin(SPEAKER_I2S_PORT, &pin_config));

    // I2S Start
    i2s_start(SPEAKER_I2S_PORT);
}

void speaker_task(void *arg) {
    
    const int sample_rate = 44100;
    const int freq = 440; // A4 note
    const int duration_sec = 2;
    const int samples = sample_rate * duration_sec;
    const float amplitude = 16000.0f;  // Max 32767 for int16_t
    size_t bytes_written;

    int16_t *sine_wave = (int16_t *)malloc(samples * sizeof(int16_t));
    if (!sine_wave) {
        ESP_LOGE("SPEAKER", "Failed to allocate memory for sine wave");
        vTaskDelete(NULL);
        return;
    }

    for (int i = 0; i < samples; i++) {
        sine_wave[i] = (int16_t)(amplitude * sin(2 * PI * freq * i / sample_rate));
    }

    ESP_LOGI("SPEAKER", "Writing sine wave to speaker...");
    i2s_write(SPEAKER_I2S_PORT, sine_wave, samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    ESP_LOGI("SPEAKER", "Done writing. Bytes written: %d", bytes_written);

    free(sine_wave);
    vTaskDelete(NULL);
}

void app_main(void) {
    // speaker_setup();
    // xTaskCreatePinnedToCore(speaker_task, "speaker_task", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(start_button_task, "start_button_task", 2048, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(mic_task, "mic_task", 8192, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(spi_task, "spi_task", 8192, NULL, 5, NULL, 0);

}

