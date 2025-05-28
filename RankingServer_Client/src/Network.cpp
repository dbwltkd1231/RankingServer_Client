#pragma once
#include "Network.h"

namespace Network
{
	Network::Network()
	{
		// Constructor implementation
        mReceive_HeaderBuffer = new char[sizeof(MessageHeader)];
        mReceive_BodyBuffer = new char[BUFFER_SIZE];

        mSend_HeaderBuffer = new char[sizeof(MessageHeader)];
        mSend_BodyBuffer = new char[BUFFER_SIZE];
	}

	Network::~Network()
	{
		// Destructor implementation
        delete[] mReceive_HeaderBuffer;
        delete[] mReceive_BodyBuffer;

        delete[] mSend_HeaderBuffer;
        delete[] mSend_BodyBuffer;
	}

	void Network::Start(std::string ip, int port)
	{
        WSADATA wsaData;

        // Winsock 초기화
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup 실패: " << WSAGetLastError() << "\n";
            return;
        }

        clientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "소켓 생성 실패: " << WSAGetLastError() << "\n";
            WSACleanup();
            return;
        }

        // 로컬 주소 바인딩
        sockaddr_in localAddr = { 0 };
        localAddr.sin_family = AF_INET;
        localAddr.sin_addr.s_addr = INADDR_ANY;
        localAddr.sin_port = 0;  // 자동 할당
        if (bind(clientSocket, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
            std::cerr << "클라이언트 bind 실패 " << WSAGetLastError() << "\n";
            closesocket(clientSocket);
            WSACleanup();
            return;
        }

        // 서버 주소 설정
        sockaddr_in serverAddr = { 0 };
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

        // ConnectEx 함수 포인터 가져오기
        LPFN_CONNECTEX ConnectEx = NULL;
        GUID connectExGuid = WSAID_CONNECTEX;
        DWORD bytes;
        if (WSAIoctl(clientSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
            &connectExGuid, sizeof(connectExGuid),
            &ConnectEx, sizeof(ConnectEx),
            &bytes, NULL, NULL)) {
            std::cerr << "WSAIoctl 실패: " << WSAGetLastError() << "\n";
            closesocket(clientSocket);
            WSACleanup();
            return;
        }

        iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        CreateIoCompletionPort((HANDLE)clientSocket, iocp, (ULONG_PTR)this, 0);

        // ConnectEx 호출
        ZeroMemory(&mRecvContext, sizeof(mRecvContext));
        mRecvContext.mOperationType = OperationType::OP_ACCEPT;

        if (!ConnectEx(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr), NULL, 0, NULL, &mRecvContext)) {
            if (WSAGetLastError() != WSA_IO_PENDING) {
                std::cerr << "클라이언트 ConnectEx 실패: " << WSAGetLastError() << "\n";
                closesocket(clientSocket);
                WSACleanup();
                return;
            }
        }
        std::cout << "ConnectEx 성공: " << std::endl;
        mConnected = true;
	}

    void Network::ReceiveReady()
    {
        int errorCode;
        int errorCodeSize = sizeof(errorCode);
        getsockopt(clientSocket, SOL_SOCKET, SO_ERROR, (char*)&errorCode, &errorCodeSize);
        if (errorCode != 0)
        {
            std::cerr << "Socket error detected: " << errorCode << std::endl;
            return;
        }

		mRecvContext.mOperationType = OperationType::OP_RECV;

        memset(mReceive_HeaderBuffer, 0, sizeof(MessageHeader));
        memset(mReceive_BodyBuffer, 0, BUFFER_SIZE);

        MessageHeader newHeader(0, 0, 0);
        std::memcpy(mReceive_HeaderBuffer, &newHeader, sizeof(MessageHeader));
        mRecvContext.SetHeader(mReceive_HeaderBuffer, sizeof(MessageHeader));
        mRecvContext.SetBody(mReceive_BodyBuffer, BUFFER_SIZE);
        mRecvContext.hEvent = NULL;

        DWORD flags = 0;
        int result = WSARecv(clientSocket, mRecvContext.wsabuf, 2, nullptr, &flags, &mRecvContext, nullptr);

        std::string log;
        errorCode = WSAGetLastError();
        if (result == SOCKET_ERROR && errorCode != WSA_IO_PENDING)
        {
            log =  "WSARecv 실패! 오류 코드: " + std::to_string(errorCode);
        }
        else
        {
            log = " Socket Receive Ready";
        }
    }

    void Network::Send(uint32_t size)
    {
        if ( clientSocket == INVALID_SOCKET)
        {
            return;
        }

        mSendContext.mOperationType = OperationType::OP_SEND;
        mSendContext.SetHeader(mSend_HeaderBuffer, sizeof(MessageHeader));
        mSendContext.SetBody(mSend_BodyBuffer, size);
        mSendContext.hEvent = NULL;

        DWORD flags = 0;
        int result = WSASend(clientSocket, mSendContext.wsabuf, 2, nullptr, flags, &mSendContext, nullptr);
        int errorCode = WSAGetLastError();
        if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
        {
            std::string log =  "WSASend 실패! 오류 코드: " + std::to_string(errorCode);

            return;
        }
    }

    void Network::Process()
    {
        if (!mConnected)
            return;

        std::thread sessionThread(&Network::Work, this);
		sessionThread.detach();

        ReceiveReady();
    }

    void Network::Work()
    {
        DWORD bytesTransferred;
        ULONG_PTR completionKey;
        CustomOverlapped* oerlapped = nullptr;

        while (mConnected)
        {
            bytesTransferred = 0;
            completionKey = 0;
            oerlapped = nullptr;
            bool result = GetQueuedCompletionStatus(iocp, &bytesTransferred, &completionKey, reinterpret_cast<LPOVERLAPPED*>(&oerlapped), INFINITE);

            if (result)
            {
                auto targetOverlapped = static_cast<CustomOverlapped*>(oerlapped);

                switch (targetOverlapped->mOperationType)
                {
                case OperationType::OP_RECV:
                {
                    MessageHeader* receivedHeader = reinterpret_cast<MessageHeader*>(targetOverlapped->wsabuf[0].buf);

                    auto socket_id = completionKey;
                    auto request_body_size = ntohl(receivedHeader->body_size);
                    auto request_contents_type = ntohl(receivedHeader->contents_type);

                    auto messageType = static_cast<protocol::MessageContent>(request_contents_type);

                    switch (messageType)
                    {
                    case protocol::MessageContent_RESPONSE_PLAYER_RANKING:
                    {
                        auto message_wrapper = flatbuffers::GetRoot<protocol::RESPONSE_PLAYER_RANKING>(targetOverlapped->wsabuf[1].buf);
               
                        std::string playerId = message_wrapper->player_id()->str();
						int score = message_wrapper->score();
						bool inRanking = message_wrapper->in_ranking();

                        std::string log = "Player ID: " + playerId + ", Score: " + std::to_string(score) + ", In Ranking: " + (inRanking ? "Yes" : "No"); + "\n";
						std::cout << log << std::endl;
                        ReceiveReady();
                        break;
                    }
                    case protocol::MessageContent_RESPONSE_SAVE_SCORE:
                    {
                        auto  REQUEST_SAVE_SCORE = flatbuffers::GetRoot<protocol::RESPONSE_SAVE_SCORE>(targetOverlapped->wsabuf[1].buf);
                        bool feedback = REQUEST_SAVE_SCORE->feedback();
                        
                        std::string feedback_str = (feedback ? "Success" : "Failure");
                        std::string log = "Save Score Feedback: " + feedback_str;

						std::cout << log << std::endl;
                        ReceiveReady();
                        break;
                    }
                    default:
                    {
                        break;
                    }
                    }
                    break;
                }
                case OperationType::OP_ACCEPT:
                {
                  
                    break;
                }
                case OperationType::OP_SEND:
                   
                    break;
                default:
                    std::cout << "??? Success" << std::endl;
                    break;
                }
            }
        }

    }
}