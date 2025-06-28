#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

SOCKET clients[MAX_CLIENTS];
int client_count = 0;
HANDLE clients_mutex;

void broadcast(const char *message, SOCKET sender) {
    WaitForSingleObject(clients_mutex, INFINITE);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i] != sender) {
            send(clients[i], message, strlen(message), 0);
        }
    }
    ReleaseMutex(clients_mutex);
}

void broadcast_file(const char *filename, long file_size, char *file_data, SOCKET sender) {
    char header[BUFFER_SIZE];
    snprintf(header, sizeof(header), "FILE:%s:%ld", filename, file_size);

    WaitForSingleObject(clients_mutex, INFINITE);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i] != sender) {
            send(clients[i], header, strlen(header), 0);
            Sleep(100); // small delay to ensure header is read first
            send(clients[i], file_data, file_size, 0);
        }
    }
    ReleaseMutex(clients_mutex);
}

DWORD WINAPI handle_client(LPVOID arg) {
    SOCKET client_socket = *((SOCKET *)arg);
    free(arg);

    char buffer[BUFFER_SIZE];
    int bytes_read;

    WaitForSingleObject(clients_mutex, INFINITE);
    if (client_count < MAX_CLIENTS) {
        clients[client_count++] = client_socket;
    }
    ReleaseMutex(clients_mutex);

    while ((bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[bytes_read] = '\0';

        if (strncmp(buffer, "FILE:", 5) == 0) {
            char *filename = strtok(buffer + 5, ":");
            char *size_str = strtok(NULL, ":");
            long file_size = atol(size_str);

            if (file_size <= 0) {
                printf("Invalid file size from client.\n");
                break;
            }

            char *file_data = malloc(file_size);
            if (!file_data) {
                printf("Memory allocation failed\n");
                break;
            }

            long received = 0;
            while (received < file_size) {
                int chunk = recv(client_socket, file_data + received,
                                 file_size - received > BUFFER_SIZE ? BUFFER_SIZE : file_size - received, 0);
                if (chunk <= 0) {
                    free(file_data);
                    goto cleanup;
                }
                received += chunk;
            }

            printf("Received file from client: %s (%ld bytes)\n", filename, file_size);
            broadcast_file(filename, file_size, file_data, client_socket);
            free(file_data);
        } else {
            char message[BUFFER_SIZE + 50];
            snprintf(message, sizeof(message), "Client %lld: %s", (long long)client_socket, buffer);
            printf("%s\n", message);
            broadcast(message, client_socket);
        }
    }

cleanup:
    closesocket(client_socket);
    WaitForSingleObject(clients_mutex, INFINITE);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i] == client_socket) {
            clients[i] = clients[client_count - 1];
            client_count--;
            break;
        }
    }
    ReleaseMutex(clients_mutex);
    return 0;
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);

    printf("Server started on port %d...\n", PORT);

    clients_mutex = CreateMutex(NULL, FALSE, NULL);

    while (1) {
        SOCKET client_socket = accept(server_fd, NULL, NULL);
        SOCKET *pclient = malloc(sizeof(SOCKET));
        *pclient = client_socket;
        CreateThread(NULL, 0, handle_client, pclient, 0, NULL);
    }

    WSACleanup();
    return 0;
}
