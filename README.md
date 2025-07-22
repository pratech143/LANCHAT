# LAN Chat Application with File Sharing

## Overview
This is a console-based LAN chat application implemented in C using Winsock2 for networking on Windows. It supports multiple clients connecting to a central server. Clients can send text messages and share files with others. Each client has a dedicated folder on the server where their received files are saved.

---

## Features
- Multi-client support (configurable maximum clients)
- Text chat between connected clients
- File transfer support (with separate folders for each client)
- Automatic creation of client folders on the server
- Clients must provide a unique username before chatting
- Server broadcasts messages and files to all connected clients except the sender

---



## Compilation Instructions

Open your terminal (e.g., MSYS2 or MinGW) and run the following commands:

### Compile server
```bash
gcc server.c -o server.exe -lws2_32
```

### Compile client
```bash
gcc client.c -o client.exe -lws2_32
```

---

## Running Instructions

### Start the server
```bash
./server.exe
```

### Start the client
```bash
./client.exe
```

### Sending files in the chat
In the client terminal, type:
```bash
sendfile path\to\your\file
```
The file will be saved automatically as:
```
ClientFiles\<YourUsername>\
```
