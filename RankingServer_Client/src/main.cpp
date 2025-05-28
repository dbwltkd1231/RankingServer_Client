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
	Network::Network network;
	network.Start(MY_IP, MY_PORT_NUM);
	network.Process();


	std::string myId;
	std::cout << "���̵� �Է��ϼ���: \n";
	std::cin >> myId;

	std::cout << myId + " Ȯ�� ������û�۾� ����.. \n";
	int interval = 3; // 3�� �������� ��û
	int myScore = 500;

	while (network.mConnected)
	{
		std::time_t unixTime = std::time(nullptr);
		auto size_2 = Business::Create_Request_Save_Score(myId, myScore, unixTime, network.mSend_HeaderBuffer, network.mSend_BodyBuffer);
		std::cout << myId << "�� ������ �����մϴ�. ����: " << myScore << "\n";
		network.Send(size_2);
		myScore += 100;

		std::this_thread::sleep_for(std::chrono::seconds(interval));

		auto size_1 = Business::Create_Request_Player_Ranking(myId, network.mSend_HeaderBuffer, network.mSend_BodyBuffer);
		std::cout << myId << "�� ��ŷ�� Ȯ���մϴ�.\n";
		network.Send(size_1);

		std::this_thread::sleep_for(std::chrono::seconds(interval));
	}

}