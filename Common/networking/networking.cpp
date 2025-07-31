#include <stdio.h>
#include "networking.hpp"

WSADATA Networking::wsaData = {};

void Networking::init() {
    int iResult;
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &Networking::wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        exit(1);
    }
    onexit(WSACleanup);
}