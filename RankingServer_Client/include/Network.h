#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <ws2tcpip.h>

#include "CustomPacket.h"
#include "LockFreeCircleQueue.h"

#define BUFFER_SIZE 1024

namespace Network
{
	class Network 
	{
	public:
		bool mConnected = false;

		Network();
		~Network();

		void Start(std::string ip, int port);
		void Process();
		void Send(uint32_t size);

		char* mSend_HeaderBuffer;
		char* mSend_BodyBuffer;

	private:
		void Work();
		void ReceiveReady();

		HANDLE iocp;
		SOCKET clientSocket;
		CustomOverlapped mRecvContext;
		CustomOverlapped mSendContext;

		char* mReceive_HeaderBuffer;
		char* mReceive_BodyBuffer;
	};
}