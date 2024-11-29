// gcc newserver.c -o oldserver.exe -lws2_32
// .\newserver.exe

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024
#define PORT 8080

typedef struct {
    SOCKET socket;
    char nickname[20];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
char session_code[20];

void generate_session_code() {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    srand((unsigned int)time(NULL));
    
    for (int i = 0; i < 6; i++) {
        session_code[i] = charset[rand() % strlen(charset)];
    }
    session_code[6] = '\0';
}

void broadcast_message(char* message, SOCKET exclude_socket) {
    char formatted_message[BUFFER_SIZE];
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket != exclude_socket) {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
}

void handle_client_message(char* buffer, SOCKET sender_socket) {
    char nickname[20] = "";
    // Encontrar el nickname del remitente
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket == sender_socket) {
            strcpy(nickname, clients[i].nickname);
            break;
        }
    }
    
    // Formatear el mensaje con el nickname
    char formatted_message[BUFFER_SIZE];
    snprintf(formatted_message, BUFFER_SIZE, "%s: %s", nickname, buffer);
    printf("%s", formatted_message); // Mostrar en el servidor
    broadcast_message(formatted_message, sender_socket); // Enviar a otros clientes
}

void remove_client(SOCKET socket) {
    int pos = -1;
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket == socket) {
            pos = i;
            break;
        }
    }
    
    if (pos >= 0) {
        char message[100];
        sprintf(message, "Usuario %s ha salido del chat\n", clients[pos].nickname);
        broadcast_message(message, socket);
        
        closesocket(socket);
        
        for (int i = pos; i < client_count - 1; i++) {
            clients[i] = clients[i + 1];
        }
        client_count--;
        
        printf("%s", message);
    }
}

DWORD WINAPI handle_client(LPVOID lpParam) {
    SOCKET client_socket = (SOCKET)lpParam;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while (1) {
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            remove_client(client_socket);
            break;
        }
        
        buffer[bytes_received] = '\0';
        handle_client_message(buffer, client_socket);
    }
    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server, client;
    int c = sizeof(struct sockaddr_in);
    char buffer[BUFFER_SIZE];
    
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Error al inicializar Winsock\n");
        return 1;
    }
    
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Error al crear socket\n");
        return 1;
    }
    
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Error en bind\n");
        return 1;
    }
    
    listen(server_socket, 3);
    
    generate_session_code();
    printf("Servidor iniciado en puerto %d\nCódigo de sesión: %s\n", PORT, session_code);
    
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client, &c);
        if (client_socket == INVALID_SOCKET) {
            printf("Error al aceptar conexión\n");
            continue;
        }
        
        if (client_count >= MAX_CLIENTS) {
            send(client_socket, "Sala llena\n", 10, 0);
            closesocket(client_socket);
            continue;
        }
        
        recv(client_socket, buffer, BUFFER_SIZE, 0);
        buffer[strcspn(buffer, "\n")] = 0;
        
        char received_code[20];
        char nickname[20];
        sscanf(buffer, "%s %s", received_code, nickname);
        
        if (strcmp(received_code, session_code) != 0) {
            send(client_socket, "Código de sesión inválido\n", 26, 0);
            closesocket(client_socket);
            continue;
        }
        
        clients[client_count].socket = client_socket;
        strncpy(clients[client_count].nickname, nickname, sizeof(clients[client_count].nickname) - 1);
        
        char message[100];
        sprintf(message, "Usuario %s se ha unido al chat\n", nickname);
        printf("%s", message);
        broadcast_message(message, client_socket);
        
        client_count++;
        
        char welcome[100];
        sprintf(welcome, "Bienvenido %s!\n", nickname);
        send(client_socket, welcome, strlen(welcome), 0);

        // Crear thread para manejar al cliente
        CreateThread(NULL, 0, handle_client, (LPVOID)client_socket, 0, NULL);
    }
    
    closesocket(server_socket);
    WSACleanup();
    return 0;
}