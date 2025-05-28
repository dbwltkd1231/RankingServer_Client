#include "Client.h"

namespace Network
{
	Client::Client()
	{
		mReceive_HeaderBuffer = new char[sizeof(MessageHeader)];
		mReceive_BodyBuffer = new char[BUFFER_SIZE];

		mSend_HeaderBuffer = new char[sizeof(MessageHeader)];
		mSend_BodyBuffer = new char[BUFFER_SIZE];
	}

	Client::~Client()
	{
		delete[] mReceive_HeaderBuffer;
		delete[] mReceive_BodyBuffer;

		delete[] mSend_HeaderBuffer;
		delete[] mSend_BodyBuffer;
	}

	void Client::Initialize(std::shared_ptr<SOCKET> socket)
	{
		mClientSocket = socket;
	}

	void Client::ConnectEx(LPFN_CONNECTEX& connectEx, sockaddr_in serverAddr, CustomOverlapped& overlapped)
	{
		// 소켓이 유효한지 먼저 확인
		if (!mClientSocket || *mClientSocket == INVALID_SOCKET) {
			std::cerr << "잘못된 소켓입니다!" << std::endl;
			return;
		}

		memset(mReceive_HeaderBuffer, 0, sizeof(MessageHeader));
		memset(mReceive_BodyBuffer, 0, BUFFER_SIZE);
		overlapped.mOperationType = OperationType::OP_ACCEPT;

		MessageHeader newHeader(0, 0, 0);
		std::memcpy(mReceive_HeaderBuffer, &newHeader, sizeof(MessageHeader));
		overlapped.SetHeader(mReceive_HeaderBuffer, sizeof(MessageHeader));
		overlapped.SetBody(mReceive_BodyBuffer, BUFFER_SIZE);
		overlapped.hEvent = NULL;

		BOOL result = connectEx(*mClientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr), NULL, 0, NULL, &overlapped);

		if (!result) {
			int errorCode = WSAGetLastError();
			if (errorCode != WSA_IO_PENDING) {
				std::cerr << "클라이언트 ConnectEx 실패! 오류 코드: " << errorCode << std::endl;

				// 소켓을 안전하게 닫고 정리
				if (*mClientSocket != INVALID_SOCKET) {
					closesocket(*mClientSocket);
					*mClientSocket = INVALID_SOCKET;
				}
				WSACleanup();
				return;
			}
		}

		std::cout << "ConnectEx 성공!" << std::endl;
	}

	void Client::ReceiveReady(CustomOverlapped& overlapped)
	{
		int errorCode;
		int errorCodeSize = sizeof(errorCode);
		getsockopt(*mClientSocket, SOL_SOCKET, SO_ERROR, (char*)&errorCode, &errorCodeSize);
		if (errorCode != 0)
		{
			std::cerr << "Socket error detected: " << errorCode << std::endl;
			return;
		}

		overlapped.mOperationType = OperationType::OP_RECV;

		memset(mReceive_HeaderBuffer, 0, sizeof(MessageHeader));
		memset(mReceive_BodyBuffer, 0, BUFFER_SIZE);

		MessageHeader newHeader(0, 0, 0);
		std::memcpy(mReceive_HeaderBuffer, &newHeader, sizeof(MessageHeader));
		overlapped.SetHeader(mReceive_HeaderBuffer, sizeof(MessageHeader));
		overlapped.SetBody(mReceive_BodyBuffer, BUFFER_SIZE);
		overlapped.hEvent = NULL;

		DWORD flags = 0;
		int result = WSARecv(*mClientSocket, overlapped.wsabuf, 2, nullptr, &flags, &overlapped, nullptr);

		std::string log;
		errorCode = WSAGetLastError();
		if (result == SOCKET_ERROR && errorCode != WSA_IO_PENDING)
		{
			log = "WSARecv 실패! 오류 코드: " + std::to_string(errorCode);
		}
		else
		{
			log = " Socket Receive Ready";
		}
	}

	void Client::Send(CustomOverlapped& overlapped)
	{
		if (*mClientSocket == INVALID_SOCKET)
		{
			return;
		}

		overlapped.mOperationType = OperationType::OP_SEND;
		overlapped.hEvent = NULL;

		if (overlapped.wsabuf[0].buf == nullptr)
		{
			std::cout << "??" << std::endl;
			return;
		}
		DWORD flags = 0;
		int result = WSASend(*mClientSocket, overlapped.wsabuf, 2, nullptr, flags, &overlapped, nullptr);
		int errorCode = WSAGetLastError();
		if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			std::string log = "WSASend 실패! 오류 코드: " + std::to_string(errorCode);

			return;
		}
	}
}