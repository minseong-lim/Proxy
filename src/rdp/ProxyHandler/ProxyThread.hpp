#pragma once
#include <atomic>

/// @brief ������ Client ���� ó���ϱ� ���� Thread Function
/// @param ������ ������� �ѱ� ����
/// @return
void ProxyThreadProc(const std::atomic<bool>& bStop, int clientSockDesc, int iClientIndex, int iListenerIndex, ShmAll *shmUtil);