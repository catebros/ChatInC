// este codigo es el cliente que se conecta al servidor y envia mensajes sin UI
// Dirección IPv4. . . . . . . . . . . . . . : hacer ipconfig 
// gcc newclient.c -o newclient.exe -lws2_32
// .\newclient.exe Codigo Nombre

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024
#define PORT 8080

SOCKET client_socket;
int running = 1;

DWORD WINAPI receive_thread(LPVOID lpParam) {
    char buffer[BUFFER_SIZE];
    while (running) {
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Desconectado del servidor\n");
            running = 0;
            break;
        }
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
        fflush(stdout);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <código_sesión> <nickname>\n", argv[0]);
        return 1;
    }

    WSADATA wsa;
    struct sockaddr_in server;
    char message[BUFFER_SIZE];
    
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Error al inicializar Winsock\n");
        return 1;
    }
    
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Error al crear socket\n");
        return 1;
    }
    
    server.sin_addr.s_addr = inet_addr("172.20.10.3");
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    
    if (connect(client_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Error de conexión\n");
        return 1;
    }
    
    printf("Conectado al servidor\n");
    
    sprintf(message, "%s %s\n", argv[1], argv[2]);
    send(client_socket, message, strlen(message), 0);
    
    // Crear thread para recibir mensajes
    HANDLE hThread = CreateThread(NULL, 0, receive_thread, NULL, 0, NULL);
    
    // Loop principal para enviar mensajes
    while (running) {
        if (fgets(message, BUFFER_SIZE, stdin) == NULL) break;
        
        if (strcmp(message, "quit\n") == 0) {
            running = 0;
            break;
        }
        
        if (send(client_socket, message, strlen(message), 0) < 0) {
            printf("Error al enviar mensaje\n");
            break;
        }
    }
    
    closesocket(client_socket);
    WSACleanup();
    
    if (hThread != NULL) {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }
    
    return 0;
}