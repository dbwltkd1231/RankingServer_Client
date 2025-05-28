#pragma once
#include <iostream>
#include<memory>

#define NOMINMAX
#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <ws2tcpip.h>

#include <oneapi/tbb/concurrent_map.h>
#include <oneapi/tbb/concurrent_vector.h>

#include "CustomPacket.h"
#include "LockFreeCircleQueue.h"

#define BUFFER_SIZE 1024

namespace Network
{
	class Client
	{
	public:
		Client();
		~Client();
		void Initialize(std::shared_ptr<SOCKET>);
		void ConnectEx(LPFN_CONNECTEX& connectEx, sockaddr_in serverAddr, CustomOverlapped& overlapped);
		void ReceiveReady(CustomOverlapped& overlapped);
		void Send(CustomOverlapped& overlapped);
		char* mSend_HeaderBuffer;
		char* mSend_BodyBuffer;

	private:
		std::shared_ptr<SOCKET> mClientSocket;
		Utility::LockFreeCircleQueue<CustomOverlapped*> mOverlappedQueue;

		char* mReceive_HeaderBuffer;
		char* mReceive_BodyBuffer;
	};
}