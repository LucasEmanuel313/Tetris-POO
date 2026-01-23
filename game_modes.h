#ifndef GAME_MODES_H
#define GAME_MODES_H

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>

void runSinglePlayer();
void runMultiplayerClient(SOCKET sock);

#endif
