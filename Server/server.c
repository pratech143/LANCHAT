#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

typedef struct {
    SOCKET socket;
    char name[50];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
HANDLE clients_mutex;

void broadcast(const char *message, SOCKET sender) {
    WaitForSingleObject(clients_mutex, INFINITE);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].socket != sender) {
            send(clients[i].socket, message, (int)strlen(message), 0);
        }
    }
    ReleaseMutex(clients_mutex);
}

void broadcast_file(const char *filename, long long file_size, char *file_data, SOCKET sender) {
    char header[BUFFER_SIZE];
    snprintf(header, sizeof(header), "FILE:%s:%lld", filename, file_size);

    WaitForSingleObject(clients_mutex, INFINITE);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].socket != sender) {
            send(clients[i].socket, header, (int)strlen(header), 0);
            Sleep(100);
            send(clients[i].socket, file_data, (int)file_size, 0);
        }
    }
    ReleaseMutex(clients_mutex);
}

DWORD WINAPI handle_client(LPVOID arg) {
    SOCKET client_socket = *((SOCKET *)arg);
    free(arg);

    char buffer[BUFFER_SIZE];
    int bytes_read;
    char client_name[50] = "";

    // Receive the client name
    bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_read <= 0) {
        closesocket(client_socket);
        return 0;
    }
    buffer[bytes_read] = '\0';
    strncpy(client_name, buffer, sizeof(client_name) - 1);
    client_name[sizeof(client_name) - 1] = '\0';

    // Check if we can accept this client
    WaitForSingleObject(clients_mutex, INFINITE);
    if (client_count < MAX_CLIENTS) {
        clients[client_count].socket = client_socket;
        strncpy(clients[client_count].name, client_name, sizeof(clients[client_count].name) - 1);
        client_count++;
        ReleaseMutex(clients_mutex);
    } else {
        ReleaseMutex(clients_mutex);
        const char *msg = "Server full. Maximum number of clients reached.\n";
        send(client_socket, msg, (int)strlen(msg), 0);
        closesocket(client_socket);
        return 0;
    }

    // Notify others
    char join_msg[BUFFER_SIZE];
    snprintf(join_msg, sizeof(join_msg), "%s joined the chat.\n", client_name);
    printf("%s", join_msg);
    broadcast(join_msg, client_socket);

    while ((bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[bytes_read] = '\0';

        if (strncmp(buffer, "FILE:", 5) == 0) {
            char *filename = strtok(buffer + 5, ":");
            char *size_str = strtok(NULL, ":");
            long long file_size = _atoi64(size_str);

            if (!filename || file_size <= 0) {
                printf("Invalid file header from %s.\n", client_name);
                break;
            }

            char *file_data = malloc(file_size);
            if (!file_data) {
                printf("Memory allocation failed for file.\n");
                break;
            }

            long long received = 0;
            while (received < file_size) {
                int chunk = recv(client_socket, file_data + received,
                                 (file_size - received > BUFFER_SIZE) ? BUFFER_SIZE : (int)(file_size - received), 0);
                if (chunk <= 0) {
                    free(file_data);
                    goto cleanup;
                }
                received += chunk;
            }

            printf("%s sent file: %s (%lld bytes)\n", client_name, filename, file_size);
            broadcast_file(filename, file_size, file_data, client_socket);
            free(file_data);
        } else {
            char message[BUFFER_SIZE + 100];
            snprintf(message, sizeof(message), "%s: %s\n", client_name, buffer);
            printf("%s", message);
            broadcast(message, client_socket);
        }
    }

cleanup:
    closesocket(client_socket);

    WaitForSingleObject(clients_mutex, INFINITE);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].socket == client_socket) {
            snprintf(buffer, sizeof(buffer), "%s left the chat.\n", clients[i].name);
            printf("%s", buffer);
            broadcast(buffer, client_socket);
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
    SOCKET server_fd;
    struct sockaddr_in server_addr;

    WSAStartup(MAKEWORD(2, 2), &wsa);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
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
