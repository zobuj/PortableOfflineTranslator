#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>

#define SOCKET_PATH "/tmp/translate_socket"
#define RPI5_I2C 0x00

void get_language_config(char *source_lang, char *dest_lang) {
    FILE *file = fopen("../pcm_generator/config.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    char line[100];
    char lang1[50] = {0};  // Initialize to ensure null-termination
    char lang2[50] = {0};

    // Read one line from the file
    if (fgets(line, sizeof(line), file) != NULL) {
        // Remove newline if present
        line[strcspn(line, "\n")] = 0;

        // Extract two words
        if (sscanf(line, "%49s %49s", lang1, lang2) == 2) {
            // Ensure null termination in case of overflow
            lang1[49] = '\0';
            lang2[49] = '\0';

            // Copy extracted words safely
            strncpy(source_lang, lang1, 49);
            source_lang[49] = '\0';  // Ensure null termination

            strncpy(dest_lang, lang2, 49);
            dest_lang[49] = '\0';  // Ensure null termination
        } else {
            printf("Invalid format in file. Expected two words.\n");
        }
    } else {
        printf("Error reading the file.\n");
    }

    fclose(file);
}

void HAL_I2C_Transmit(int sock, uint8_t DevAddress, uint8_t *pData, uint16_t Size) {
    uint8_t buffer[256];
    // buffer[0] = DevAddress; // First byte = Device address
    memcpy(&buffer[0], pData, Size);

    send(sock, buffer, Size + 1, 0);
    printf("MCU: Sent data to I2C device 0x%X\n", DevAddress);
}

void HAL_I2C_Receive(int sock, uint8_t *pData, uint16_t Size) {
    recv(sock, pData, Size, 0);
    printf("MCU: Received response from I2C device: ");
    for (int i = 0; i < Size; i++) {
        printf("0x%X ", pData[i]);
    }
    printf("\n");
}

int main() {
    int sock;
    struct sockaddr_un addr;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed!");
        return 1;
    }

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("Connect error");
        return 1;
    }

    uint8_t start_translation_sequence[] = {0xFF};
    char source_language[50];
    char dest_language[50];

    get_language_config(source_language, dest_language);

    printf("Source Language: %s\n", source_language);
    printf("Destination Language: %s\n", dest_language);
    printf("Length of Destination Language: %lu\n", strlen(dest_language));

    uint8_t response[1];

    printf("MCU: Starting data transmission...\n");
    HAL_I2C_Transmit(sock, RPI5_I2C, start_translation_sequence, sizeof(start_translation_sequence));  // Send Start Sequence
    HAL_I2C_Receive(sock, response, sizeof(response)); // Receive response
    
    if(response[0] != 0xFF) {close(sock); return 0;}
    printf("Raspberry Pi Acknowledged Start Sequence.\n");
    
    HAL_I2C_Transmit(sock, RPI5_I2C, (uint8_t *)source_language, strlen(source_language)+1);  // Send Source Language
    HAL_I2C_Receive(sock, response, sizeof(response)); // Receive Response
    
    if(response[0] != 0xFF) {close(sock); return 0;}
    printf("Raspberry Pi Acknowledged Source Language.\n");
    
    HAL_I2C_Transmit(sock, RPI5_I2C, (uint8_t *)dest_language, strlen(dest_language)+1);  // Send Destination Language
    HAL_I2C_Receive(sock, response, sizeof(response)); // Receive Response
    
    if(response[0] != 0xFF) {close(sock); return 0;}
    printf("Raspberry Pi Acknowledged Destination Language.\n");
    

    
    close(sock);
    printf("MCU: Data transmission ended.\n");

    return 0;
}
