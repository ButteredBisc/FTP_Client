// client.cpp : Used for sending files to server
//

#include <stdio.h>
#include <cmath>
#include <string>
#include <cstring>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#define _WIN32_WINNT 0x501
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#define SCK_VERSION2

using namespace std;

/// **METHODS** ///
int64_t GetFileSize(const std::string& fileName);
int SendBuffer(SOCKET s, const char* buffer, int bufferSize, int chunkSize = 4 * 1024);
int64_t SendFile(SOCKET s, const std::string& fileName, int chunkSize = 64 * 1024);

//int64_t RecvFile(SOCKET s, const std::string& fileName, int chunkSize = 64 * 1024);
//int RecvBuffer(SOCKET s, char* buffer, int bufferSize, int chunkSize = 4 * 1024);


int main(int argc, char** argv)
{
    std::string filepath = "C:\\Users\\longa\\Desktop\\SDP_Demo\\ClientTest\\clientPack.zip";
    const char* port1 = "5050";
    const char* serverIP = "127.0.0.1";
    WSADATA wsaData; //Windows Socket Data
    SOCKET client = INVALID_SOCKET;
    //Sets IPv4 or IPv6, and TCP Protocol
    struct addrinfo hints, * result, * ptr;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;


    // Initialize Winsock version 2.2
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    printf("Client: Winsock DLL status is %s.\n", wsaData.szSystemStatus);

    //REQUEST SERVER IP ON SERVER PORT
    if (getaddrinfo(serverIP, port1, &hints, &result) != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL;ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        client = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (client == INVALID_SOCKET) {
            printf("socket failed with error: %d\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(client, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(client);
            client = INVALID_SOCKET;
            continue;
        }
        break;
    }
    freeaddrinfo(result); //releases struct memory

    //Checks if client connected to server
    if (client == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    //Send file and check if sent
    int64_t rc = SendFile(client, filepath);
    if (rc < 0) {
        std::cout << "Failed to send file: " << rc << std::endl;
    }

    //Close socket and close WSA
    if (closesocket(client) != 0)
        printf("Client: Cannot close \"SendingSocket\" socket. Error code: %d\n", WSAGetLastError());
    else
        printf("Client: Closing \"SendingSocket\" socket...\n");

    // When your application is finished handling the connection, call WSACleanup.
    if (WSACleanup() != 0)
        printf("Client: WSACleanup() failed!...\n");
    else
        printf("Client: WSACleanup() is OK...\n");
    return 0;
}

//Returns file size. Opens file and reads total bits
int64_t GetFileSize(const std::string& fileName) {
    // Uses Microsoft's C-style file API
    FILE* f;
    if (fopen_s(&f, fileName.c_str(), "rb") != 0) {
        return -1;
    }
    _fseeki64(f, 0, SEEK_END);
    const int64_t len = _ftelli64(f);
    _fseeki64(f, 0, SEEK_SET);
    fclose(f);
    return len;
}

///
/// Sends data in buffer until bufferSize value is met
/// Chunk size = 4 * 1024
int SendBuffer(SOCKET s, const char* buffer, int bufferSize, int chunkSize) {

    int i = 0;
    while (i < bufferSize) {
        //std::cout << "Buffer Index: " << i << " Buffer char: " << &buffer[i] << "-DONE" << endl;
        const int l = send(s, &buffer[i], min(chunkSize, bufferSize - i), 0);
        if (l < 0) { return l; }
        i += l;
    }
    return i;
}

///
/// Sends a file
/// returns size of file if success
/// returns -1 if file couldn't be opened for input
/// returns -2 if couldn't send file length properly
/// returns -3 if file couldn't be sent properly
/// chunk size = 64 * 1024;
int64_t SendFile(SOCKET s, const std::string& fileName, int chunkSize) {

    const int64_t fileSize = GetFileSize(fileName);
    if (fileSize < 0) { return -1; }

    std::ifstream file(fileName, std::ifstream::binary);
    if (file.fail()) { return -1; }

    ///Sends the file size needs to be converted on other end
   /* if (SendBuffer(s, reinterpret_cast<const char*>(&fileSize),
        sizeof(fileSize)) != sizeof(fileSize)) {
        return -2;
    }*/

    char* buffer = new char[chunkSize];
    bool errored = false;
    int64_t i = fileSize;
    while (i != 0) {
        //Determines if file is bigger or less than packets and reads that amount in buff to the min value
        const int64_t ssize = min(i, (int64_t)chunkSize);
        if (!file.read(buffer, ssize)) { errored = true; break; }
        const int l = SendBuffer(s, buffer, (int)ssize);
        if (l < 0) { errored = true; break; }
        i -= l;
    }
    delete[] buffer;

    file.close();

    return errored ? -3 : fileSize;
}
/*
///
/// Recieves data in to buffer until bufferSize value is met
/// Chunk size = 4 * 1024
int RecvBuffer(SOCKET s, char* buffer, int bufferSize, int chunkSize) {
    int i = 0;
    while (i < bufferSize) {
        const int l = recv(s, &buffer[i], min(chunkSize, bufferSize - i), 0);
        if (l < 0) { return l; }
        i += l;
    }
    return i;
}

//
// Receives a file
// returns size of file if success
// returns -1 if file couldn't be opened for output
// returns -2 if couldn't receive file length properly
// returns -3 if couldn't receive file properly
// chunk size = 64 * 1024;
//
int64_t RecvFile(SOCKET s, const std::string& fileName, int chunkSize) {
    std::ofstream file(fileName, std::ofstream::binary);
    if (file.fail()) { return -1; }

    int64_t fileSize;
    if (RecvBuffer(s, reinterpret_cast<char*>(&fileSize),
        sizeof(fileSize)) != sizeof(fileSize)) {
        return -2;
    }

    char* buffer = new char[chunkSize];
    bool errored = false;
    int64_t i = fileSize;
    while (i != 0) {
        const int r = RecvBuffer(s, buffer, (int)min(i, (int64_t)chunkSize));
        if ((r < 0) || !file.write(buffer, r)) { errored = true; break; }
        i -= r;
    }
    delete[] buffer;

    file.close();

    return errored ? -3 : fileSize;
}
*/