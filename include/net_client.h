#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>

bool sendLine(SOCKET s, const std::string& line);

// Connect without getaddrinfo (compatibility)
bool connectToServer(const std::string& host, int port, SOCKET& outSock);

#endif
