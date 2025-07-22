#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024

SOCKET sock;
char client_name[50];

// Thread function to receive messages and files
DWORD WINAPI receive_handler(LPVOID arg) {
    char recv_buf[BUFFER_SIZE];
    int len;

    char folder_path[MAX_PATH];
    snprintf(folder_path, sizeof(folder_path), "ClientFiles\\%s", client_name);
    CreateDirectoryA("ClientFiles", NULL);
    CreateDirectoryA(folder_path, NULL);

    while (1) {
        len = recv(sock, recv_buf, BUFFER_SIZE - 1, 0);
        if (len <= 0) {
            printf("Disconnected from server.\n");
            exit(0);
        }
        recv_buf[len] = '\0';

        if (strncmp(recv_buf, "FILE:", 5) == 0) {
            // Parse header: FILE:<filename>:<size>
            char *filename = strtok(recv_buf + 5, ":");
            char *size_str = strtok(NULL, ":");
            if (!filename || !size_str) {
                printf("Invalid file header received.\n");
                continue;
            }
            long file_size = atol(size_str);

            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "ClientFiles\\%s\\%s", client_name, filename);

            FILE *fp = fopen(full_path, "wb");
            if (!fp) {
                printf("Failed to create file: %s\n", full_path);
                // Discard file data
                long to_read = file_size;
                while (to_read > 0) {
                    int chunk = recv(sock, recv_buf, (to_read > BUFFER_SIZE) ? BUFFER_SIZE : to_read, 0);
                    if (chunk <= 0) break;
                    to_read -= chunk;
                }
                continue;
            }

            long received = 0;
            while (received < file_size) {
                int chunk = recv(sock, recv_buf, (file_size - received > BUFFER_SIZE) ? BUFFER_SIZE : (file_size - received), 0);
                if (chunk <= 0) {
                    printf("Connection lost while receiving file.\n");
                    fclose(fp);
                    exit(0);
                }
                fwrite(recv_buf, 1, chunk, fp);
                received += chunk;
            }
            fclose(fp);
            printf("File received: %s (%ld bytes)\n", full_path, file_size);
        } else {
            printf("%s\n", recv_buf);
        }
    }
    return 0;
}

void send_file(SOCKET sock, const char *filepath) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        printf("Failed to open file: %s\n", filepath);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    if (file_size <= 0) {
        printf("Invalid file size: %ld\n", file_size);
        fclose(fp);
        return;
    }

    const char *filename = strrchr(filepath, '\\');
    if (!filename) filename = filepath;
    else filename++;

    char header[BUFFER_SIZE];
    snprintf(header, sizeof(header), "FILE:%s:%ld", filename, file_size);
    send(sock, header, (int)strlen(header), 0);
    Sleep(100);

    char buffer[BUFFER_SIZE];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
        if (send(sock, buffer, bytes_read, 0) == SOCKET_ERROR) {
            printf("Failed to send file data.\n");
            break;
        }
    }

    fclose(fp);
    printf("File sent: %s\n", filename);
}

int main() {
    WSADATA wsa;
    struct sockaddr_in server;
    char buffer[BUFFER_SIZE];

    printf("Enter your name: ");
    fgets(client_name, sizeof(client_name), stdin);
    client_name[strcspn(client_name, "\n")] = '\0';

    if (strlen(client_name) == 0) {
        printf("Name cannot be empty.\n");
        return 1;
    }

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Could not create socket.\n");
        WSACleanup();
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Connection failed.\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Send raw client name without any prefix
    send(sock, client_name, (int)strlen(client_name), 0);

    printf("Connected to server as '%s'. You can start typing messages.\n", client_name);
    printf("To send a file, type: sendfile path_to_file\n");

    CreateThread(NULL, 0, receive_handler, NULL, 0, NULL);

    while (1) {
        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strncmp(buffer, "sendfile ", 9) == 0) {
            send_file(sock, buffer + 9);
        } else {
            send(sock, buffer, (int)strlen(buffer), 0);
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
