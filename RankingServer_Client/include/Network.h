#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <thread>


#include "Client.h"
#include "LockFreeCircleQueue.h"



#define BUFFER_SIZE 1024

namespace Network
{
	class Network 
	{
	public:
		bool mConnected = false;
		tbb::concurrent_vector<std::shared_ptr<Client>> mClientVector;
		Utility::LockFreeCircleQueue<CustomOverlapped*> mOverlappedQueue;

		Network();
		~Network();
		void Start(std::string ip, int port, int clientCount, int threadCount);
		void Process(int threadCount);

	private:
		void Work();

		HANDLE iocp;
		tbb::concurrent_map<int, std::shared_ptr<Client>> mClientMap;

	};
}