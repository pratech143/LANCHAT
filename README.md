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

## Folder Structure
LanChat/

├── client.c # Client source code
├── server.c # Server source code
├── ClientFiles/ # Created by client automatically; stores received files in subfolders by username
└── README.md # Project documentation

## Compilation Instructions

Open your terminal (e.g., MSYS2 or MinGW) and run the following commands:

### Commands for running the chat

```bash

Compile server
gcc server.c -o server.exe -lws2_32

Compile Client
gcc client.c -o client.exe -lws2_32

Start the Server 
./server

Start the client 
./client

Send the file in the chat 
sendfile path\to\your\file

 It will be saved as ClientFiles\<YourUsername>\
