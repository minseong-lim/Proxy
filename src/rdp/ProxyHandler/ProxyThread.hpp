#pragma once
#include <atomic>

/// @brief 각각의 Client 들을 처리하기 위한 Thread Function
/// @param 각각의 스레드로 넘길 인자
/// @return
void ProxyThreadProc(const std::atomic<bool>& bStop, int clientSockDesc, int iClientIndex, int iListenerIndex, ShmAll *shmUtil);