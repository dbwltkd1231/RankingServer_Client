#pragma once
#include <string>
#include <chrono>
#include <ctime>

#include "Network.h"
#include "Protocol.h"

#define MY_IP "127.0.0.1"
#define MY_PORT_NUM 9090



int main()
{
	int clientCount = 0;
	int threadCount = 10;

	Network::Network network;

	std::cout << "������ Ŭ���̾�Ʈ ������ �Է��ϼ���: \n";
	std::cin >> clientCount;

	std::cout << "���� ������ ������ �Է��ϼ���: \n";
	std::cin >> threadCount;

	network.Start(MY_IP, MY_PORT_NUM, clientCount, threadCount);

	std::string standardId;
	std::cout << "���̵� �Է��ϼ���: \n";
	std::cin >> standardId;

	network.Process(threadCount);

	std::cout << standardId + " Ȯ��. ������û�۾� ����.. \n";
	int interval = 1; // 1�� �������� ��û
	int standardScore = 1000;

	while (network.mConnected)
	{
		std::time_t unixTime = std::time(nullptr);

		for (int i = 0;i < clientCount; ++i)
		{
			auto client = network.mClientVector[i];
			std::string myId = standardId + std::to_string(i + 1);
			int myScore = standardScore + i * 10;

			auto context = network.mOverlappedQueue.pop();
			context->SetHeader(client->mSend_HeaderBuffer, 0);
			context->SetBody(client->mSend_BodyBuffer, 0);

			Business::Create_Request_Save_Score(myId, myScore, unixTime, context);
			client->Send(*context);

			std::string log = "[Mian] " + myId + "�� ������ �����մϴ�. ����: " + std::to_string(myScore);
			std::cout << log << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(interval));
		}

		std::this_thread::sleep_for(std::chrono::seconds(2));

		for (int j = 0;j < clientCount; ++j)
		{
			auto client = network.mClientVector[j];
			std::string myId = standardId + std::to_string(j + 1);

			auto context = network.mOverlappedQueue.pop();
			context->SetHeader(client->mSend_HeaderBuffer, 0);
			context->SetBody(client->mSend_BodyBuffer, 0);

			Business::Create_Request_Player_Ranking(myId, context);
			client->Send(*context);

			std::string log = "[Mian] " + myId + "�� ��ŷ�� Ȯ���մϴ�.";
			std::cout << log << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(interval));
		}

		standardScore += 100;
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
}