#include "Client.h"

namespace Network
{
	Client::Client()
	{
		mReceiveHeaderBuffer = new char[sizeof(MessageHeader)];
		mReceiveBodyBuffer = new char[BUFFER_SIZE];

		mSendHeaderBuffer = new char[sizeof(MessageHeader)];
		mSendBodyBuffer = new char[BUFFER_SIZE];
	}

	Client::~Client()
	{
		delete[] mReceiveHeaderBuffer;
		delete[] mReceiveBodyBuffer;

		delete[] mSendHeaderBuffer;
		delete[] mSendBodyBuffer;
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

		memset(mReceiveHeaderBuffer, 0, sizeof(MessageHeader));
		memset(mReceiveBodyBuffer, 0, BUFFER_SIZE);
		overlapped.mOperationType = OperationType::OP_ACCEPT;

		MessageHeader newHeader(0, 0, 0);
		std::memcpy(mReceiveHeaderBuffer, &newHeader, sizeof(MessageHeader));
		overlapped.SetHeader(mReceiveHeaderBuffer, sizeof(MessageHeader));
		overlapped.SetBody(mReceiveBodyBuffer, BUFFER_SIZE);
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

		memset(mReceiveHeaderBuffer, 0, sizeof(MessageHeader));
		memset(mReceiveBodyBuffer, 0, BUFFER_SIZE);

		MessageHeader newHeader(0, 0, 0);
		std::memcpy(mReceiveHeaderBuffer, &newHeader, sizeof(MessageHeader));
		overlapped.SetHeader(mReceiveHeaderBuffer, sizeof(MessageHeader));
		overlapped.SetBody(mReceiveBodyBuffer, BUFFER_SIZE);
		overlapped.hEvent = NULL;

		DWORD flags = 0;
		int result = WSARecv(*mClientSocket, overlapped.mWsabuf, 2, nullptr, &flags, &overlapped, nullptr);

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

		if (overlapped.mWsabuf[0].buf == nullptr)
		{
			std::cout << "??" << std::endl;
			return;
		}
		DWORD flags = 0;
		int result = WSASend(*mClientSocket, overlapped.mWsabuf, 2, nullptr, flags, &overlapped, nullptr);
		int errorCode = WSAGetLastError();
		if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			std::string log = "WSASend 실패! 오류 코드: " + std::to_string(errorCode);

			return;
		}
	}
}