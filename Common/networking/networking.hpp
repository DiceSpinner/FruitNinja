#ifndef NETWORKING_H
#define NETWORKING_H
#include <winsock2.h> 
#include <windows.h>    
#include <ws2tcpip.h>

namespace Networking {
	extern WSADATA wsaData;
	void init();
}
#endif