#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/translate_socket"
#define RPI5_I2C 0x00


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
    char source_language[] = "English\0";
    char dest_language[] = "Spanish\0";
    uint8_t response[1];

    printf("MCU: Starting data transmission...\n");
    HAL_I2C_Transmit(sock, RPI5_I2C, start_translation_sequence, sizeof(start_translation_sequence));  // Send Start Sequence
    HAL_I2C_Receive(sock, response, sizeof(response)); // Receive response
    
    if(response[0] != 0xFF) {close(sock); return 0;}
    printf("Raspberry Pi Acknowledged Start Sequence.\n");
    
    HAL_I2C_Transmit(sock, RPI5_I2C, (uint8_t *)source_language, strlen(source_language));  // Send Source Language
    HAL_I2C_Receive(sock, response, sizeof(response)); // Receive Response
    
    if(response[0] != 0xFF) {close(sock); return 0;}
    printf("Raspberry Pi Acknowledged Source Language.\n");
    
    HAL_I2C_Transmit(sock, RPI5_I2C, (uint8_t *)dest_language, strlen(dest_language));  // Send Destination Language
    HAL_I2C_Receive(sock, response, sizeof(response)); // Receive Response
    
    if(response[0] != 0xFF) {close(sock); return 0;}
    printf("Raspberry Pi Acknowledged Destination Language.\n");
    

    
    close(sock);
    printf("MCU: Data transmission ended.\n");

    return 0;
}
