#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024

SOCKET sock;

DWORD WINAPI receive_handler(LPVOID arg) {
    char recv_buf[BUFFER_SIZE + 1];
    int len;

    // Create a "ClientFiles" directory if it doesn't exist
    CreateDirectoryA("ClientFiles", NULL);

    while ((len = recv(sock, recv_buf, BUFFER_SIZE, 0)) > 0) {
        recv_buf[len] = '\0';

        if (strncmp(recv_buf, "FILE:", 5) == 0) {
            // Parse metadata
            char *filename = strtok(recv_buf + 5, ":");
            char *size_str = strtok(NULL, ":");
            long file_size = atol(size_str);

            // Build full path: .\ClientFiles\filename
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "ClientFiles\\%s", filename);

            // Open file for writing
            FILE *fp = fopen(full_path, "wb");
            if (!fp) {
                printf("❌ Failed to create file: %s\n", full_path);
                continue;
            }

            // Receive file contents
            long received = 0;
            while (received < file_size) {
                int chunk = recv(sock, recv_buf, BUFFER_SIZE, 0);
                if (chunk <= 0) break;
                fwrite(recv_buf, 1, chunk, fp);
                received += chunk;
            }

            fclose(fp);
            printf("✅ File received and saved to: %s (%ld bytes)\n", full_path, file_size);
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
    send(sock, header, strlen(header), 0);
    Sleep(100); // small delay

    char buffer[BUFFER_SIZE];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
        send(sock, buffer, bytes_read, 0);
    }

    fclose(fp);
    printf("File sent: %s\n", filename);
}

int main() {
    WSADATA wsa;
    struct sockaddr_in server;
    char buffer[BUFFER_SIZE];

    WSAStartup(MAKEWORD(2, 2), &wsa);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr *)&server, sizeof(server));
    printf("Connected to server. You can start typing messages.\n");
    printf("To send a file, type: sendfile path_to_file\n");

    CreateThread(NULL, 0, receive_handler, NULL, 0, NULL);

    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
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
