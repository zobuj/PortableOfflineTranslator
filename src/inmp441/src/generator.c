#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

#define SOCKET_PATH "/tmp/microphone_socket"
#define FRAME_SIZE 6  // 24-bit Left & 24-bit Right - 6 bytes
#define SAMPLE_RATE 48000  // Sample rate in Hz
#define BUFFER_SIZE 6  // 6 bytes per stereo frame

void send_i2s_sample(int socket_fd, int32_t left_sample, int32_t right_sample) {
    uint8_t buffer[FRAME_SIZE];

    // Convert to 24-bit little-endian format
    buffer[0] = left_sample & 0xFF;
    buffer[1] = (left_sample >> 8) & 0xFF;
    buffer[2] = (left_sample >> 16) & 0xFF;

    buffer[3] = right_sample & 0xFF;
    buffer[4] = (right_sample >> 8) & 0xFF;
    buffer[5] = (right_sample >> 16) & 0xFF;

    send(socket_fd, buffer, FRAME_SIZE, 0);
}

int main() {

    // Open PCM file
    FILE *pcm_file = fopen("input/input_24bit_stereo.pcm", "rb");
    if (!pcm_file) {
        perror("Failed to open PCM file");
        return EXIT_FAILURE;
    }

    // Create and connect to the UNIX socket
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un server_addr;

    if (sock_fd == -1) {
        perror("Socket creation failed");
        fclose(pcm_file);
        return EXIT_FAILURE;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection to server failed");
        close(sock_fd);
        fclose(pcm_file);
        return EXIT_FAILURE;
    }

    printf("Streaming PCM data...\n");

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 20833;  // 1/48000 sec in nanoseconds (48 kHz)

    uint8_t buffer[BUFFER_SIZE];

    while (fread(buffer, 1, BUFFER_SIZE, pcm_file) == BUFFER_SIZE) {
        // Convert little-endian 24-bit PCM to 32-bit signed int
        int32_t left_sample = (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
        int32_t right_sample = (buffer[5] << 16) | (buffer[4] << 8) | buffer[3];

        // Convert to signed 32-bit (handle negative values)
        if (left_sample & 0x800000) left_sample |= 0xFF000000;
        if (right_sample & 0x800000) right_sample |= 0xFF000000;

        send_i2s_sample(sock_fd, left_sample, right_sample);

        nanosleep(&ts, NULL);  // Simulating 48kHz sample rate
    }

    printf("Finished streaming PCM data.\n");

    fclose(pcm_file);
    close(sock_fd);
    return EXIT_SUCCESS;
}
