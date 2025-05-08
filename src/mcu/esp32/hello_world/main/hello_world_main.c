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

#define STATUS_LED_GPIO GPIO_NUM_13

#define PI 3.14

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
int16_t sBuffer[bufferLen]; // DMA Buffer - 32-bit size for 16-bit PCM Data
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
#define TRANSFER_SIZE  1024

// GPIO Interrupt Pin
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

void speaker_setup() {
    const i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = 16000,
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


    const i2s_pin_config_t pin_config = {
        .bck_io_num = SPEAKER_I2S_SCK,
        .ws_io_num = SPEAKER_I2S_WS,
        .data_out_num = SPEAKER_I2S_SD,
        .data_in_num = -1
    };
    ESP_ERROR_CHECK(i2s_set_pin(SPEAKER_I2S_PORT, &pin_config));

    i2s_start(SPEAKER_I2S_PORT);
}

void mic_setup() {

    const i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,
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

    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };
    ESP_ERROR_CHECK(i2s_set_pin(I2S_PORT, &pin_config));

    i2s_start(I2S_PORT);
}

void trigger_transfer_interrupt() {
    gpio_set_level(INT_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
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
    gpio_set_level(INT_GPIO, 0);
}

void spi_transfer_setup() {
    spi_interrupt_pin_setup();

    esp_err_t ret;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

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

            // buffer overflow
            if (next_write == read_index) {
                ESP_LOGW(I2S_MIC_TAG, "Buffer overflow: overwriting oldest block");
                read_index = (read_index + 1) % bufferCnt;
            }

            // Copy the audio block into circular buffer
            memcpy(&circular_buffer[write_index * bufferLen], sBuffer, bufferLen * sizeof(int16_t));
            write_index = next_write;

            ESP_LOGI(I2S_MIC_TAG, "Sample[0..4]: %d %d %d %d %d", sBuffer[0], sBuffer[1], sBuffer[2], sBuffer[3], sBuffer[4]);
        } else {
            ESP_LOGE(I2S_MIC_TAG, "i2s_read failed: %s", esp_err_to_name(result));
        }
    }
}

void spi_task(void *arg) {
    spi_transfer_setup();

    while (1) {
        if (curr_state == STATE_WAIT_RESP) {

            ESP_LOGI(SPI_TRANSFER_TAG, "Receiving translated PCM from Raspberry Pi...");

            // Buffer to receive PCM data from Pi
            uint8_t rx_chunk[TRANSFER_SIZE*2];
            size_t bytes_written;

            while (1) {
                spi_slave_transaction_t trans = {
                    .length = TRANSFER_SIZE * 2 * 8,
                    .rx_buffer = rx_chunk,
                    .tx_buffer = NULL
                };

                esp_err_t ret = spi_slave_transmit(SPI_HOST, &trans, portMAX_DELAY);
                if (ret == ESP_OK) {
                    ESP_LOGI(SPI_TRANSFER_TAG, "Received data from Pi");
                    uint16_t end_signal;
                    memcpy(&end_signal, rx_chunk, sizeof(uint16_t));
                    if (end_signal == 0x1234) {
                        ESP_LOGI(SPI_TRANSFER_TAG, "End of PCM stream received.");
                        break;
                    }

                    // Write audio data to speaker
                    ESP_LOGI("SPEAKER", "Writing PCM data to speaker...");
                    i2s_write(SPEAKER_I2S_PORT, rx_chunk, TRANSFER_SIZE*2, &bytes_written, portMAX_DELAY);
                } else {
                    ESP_LOGE(SPI_TRANSFER_TAG, "SPI receive error: %s", esp_err_to_name(ret));
                    break;
                }
            }

            curr_state = STATE_INIT;
            gpio_set_level(STATUS_LED_GPIO, 0);
            write_index = 0;
            read_index = 0;
            ESP_LOGI(SPI_TRANSFER_TAG, "Finished playback. Resetting state.");

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
            vTaskDelay(10 / portTICK_PERIOD_MS); 
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
                    // Trigger interrupt to start polling
                    trigger_transfer_interrupt();
                    ESP_LOGI(START_BUTTON_TAG, "Button pressed, transitioning to RECORDING");
                }
                break;
            case STATE_RECORDING:
                if (!pressed && last_state) {
                    curr_state = STATE_WAIT_RESP;
                    gpio_set_level(STATUS_LED_GPIO, 1);
                    // Trigger interrupt to end polling
                    trigger_transfer_interrupt();
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

void speaker_task(void *arg) {
    
    const int sample_rate = 44100;
    const int freq = 440;
    const int duration_sec = 2;
    const int samples = sample_rate * duration_sec;
    const float amplitude = 16000.0f;
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

void loopback_task(void *arg) {
    mic_setup();
    speaker_setup();

    size_t bytes_read, bytes_written;
    int16_t mic_buf[bufferLen];

    ESP_LOGI("LOOPBACK", "Starting mic-to-speaker passthrough...");

    while (1) {
        esp_err_t ret_read = i2s_read(I2S_PORT, mic_buf, bufferLen * sizeof(int16_t), &bytes_read, portMAX_DELAY);
        if (ret_read != ESP_OK) {
            ESP_LOGE("LOOPBACK", "Mic read error: %s", esp_err_to_name(ret_read));
            continue;
        }

        esp_err_t ret_write = i2s_write(SPEAKER_I2S_PORT, mic_buf, bytes_read, &bytes_written, portMAX_DELAY);
        if (ret_write != ESP_OK) {
            ESP_LOGE("LOOPBACK", "Speaker write error: %s", esp_err_to_name(ret_write));
        }
    }
}

void app_main(void) {
    speaker_setup();

    gpio_config_t status_gpio_cfg = {
        .pin_bit_mask = (1ULL << STATUS_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&status_gpio_cfg);
    
    gpio_set_level(STATUS_LED_GPIO, 0);
    

    // USE FOR TESTING
    // xTaskCreatePinnedToCore(speaker_task, "speaker_task", 4096, NULL, 5, NULL, 1);
    // xTaskCreatePinnedToCore(loopback_task, "loopback_task", 8192, NULL, 5, NULL, 1);

    xTaskCreatePinnedToCore(start_button_task, "start_button_task", 2048, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(mic_task, "mic_task", 8192, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(spi_task, "spi_task", 8192, NULL, 5, NULL, 0);

}

/*

VIN - 3.3 V
GND - GND
SD - 3.3 V
GAIN - GND
DIN - GPIO 17
BCK - GPIO 8
LRC - GPIO 18

*/