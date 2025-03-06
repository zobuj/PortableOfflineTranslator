#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdint.h>

#define SOCKET_PATH "/tmp/microphone_socket"
#define FRAME_SIZE 6  // 24-bit Left & 24-bit Right - 6 bytes

#define SAMPLE_RATE 48000   // Sample rate in Hz
#define NUM_CHANNELS 2      // Stereo audio
#define BITS_PER_SAMPLE 24  // 24-bit PCM

int receive_full_frame(int client_sock, uint8_t *buffer, size_t frame_size) {
    size_t received = 0;
    ssize_t bytes;
    while (received < frame_size) {
        bytes = recv(client_sock, buffer + received, frame_size - received, 0);
        if (bytes <= 0) return -1; // Connection closed or error
        received += bytes;
    }
    return 0;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_un addr;
    uint8_t buffer[FRAME_SIZE];

    unlink(SOCKET_PATH);  // Remove previous socket file if it exists

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("Binding failed");
        close(server_sock);
        return EXIT_FAILURE;
    }

    if (listen(server_sock, 1) == -1) {
        perror("Listening failed");
        close(server_sock);
        return EXIT_FAILURE;
    }

    FILE *file = fopen("output/microphone_output.pcm", "wb");
    if (!file) {
        perror("Failed to open PCM file for writing");
        close(server_sock);
        return EXIT_FAILURE;
    }

    printf("Waiting for connection...\n");
    client_sock = accept(server_sock, NULL, NULL);
    if (client_sock == -1) {
        perror("Accept failed");
        close(server_sock);
        fclose(file);
        return EXIT_FAILURE;
    }

    printf("Client connected. Receiving I²S data and writing to PCM file...\n");

    while (1) {
        if (receive_full_frame(client_sock, buffer, FRAME_SIZE) == -1) break;

        // Write interleaved 24-bit stereo PCM (3 bytes per sample)
        fwrite(buffer, 1, FRAME_SIZE, file);

        printf("Received I²S Frame - Left: %02X %02X %02X | Right: %02X %02X %02X\n",
               buffer[0], buffer[1], buffer[2],
               buffer[3], buffer[4], buffer[5]);
    }

    // Cleanup
    close(client_sock);
    close(server_sock);
    fclose(file);
    unlink(SOCKET_PATH);

    printf("I2S Receiver closed. PCM data saved as 'microphone_output.pcm'.\n");
    return EXIT_SUCCESS;
}
