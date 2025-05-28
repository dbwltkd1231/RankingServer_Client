#pragma once
#include "Network.h"

namespace Network
{
	Network::Network()
	{
		// Constructor implementation

	}

	Network::~Network()
	{
		// Destructor implementation

	}

	void Network::Start(std::string ip, int port, int clientCount, int threadCount)
	{
        int contextMax = threadCount * clientCount;
        mOverlappedQueue.Construct(contextMax + 1);

        int contextCount = threadCount;
        for (int i = 0;i < contextMax; ++i)
        {
            CustomOverlapped* context = new CustomOverlapped();
            mOverlappedQueue.push(std::move(context));
        }

        WSADATA wsaData;

        // Winsock 초기화
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup 실패: " << WSAGetLastError() << "\n";
            return;
        }

        SOCKET clientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
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

        // 서버 주소 설정
        sockaddr_in serverAddr = { 0 };
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr);

        iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

        for (int index = 0; index < clientCount; ++index)
        {
            SOCKET newSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
            if (newSocket == INVALID_SOCKET)
            {
                std::cerr << "소켓 생성 실패: " << WSAGetLastError() << "\n";
                continue;
            }
            auto socketSharedPtr = std::make_shared<SOCKET>(newSocket);

            // 로컬 주소 바인딩
            sockaddr_in localAddr = { 0 };
            localAddr.sin_family = AF_INET;
            localAddr.sin_addr.s_addr = INADDR_ANY;
            localAddr.sin_port = 0;  // 자동 할당
            if (bind(*socketSharedPtr, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) 
            {
                std::cerr << "클라이언트 bind 실패 " << WSAGetLastError() << "\n";
                closesocket(*socketSharedPtr);
                continue;
            }

            std::shared_ptr<Client> clientSharedPtr = std::make_shared<Client>();
            clientSharedPtr->Initialize(socketSharedPtr);
            auto context = mOverlappedQueue.pop();
            clientSharedPtr->ConnectEx(ConnectEx, serverAddr, *context);
            mClientMap.insert(std::make_pair((ULONG_PTR)clientSharedPtr.get(), clientSharedPtr));
            mClientVector.push_back(clientSharedPtr);
            CreateIoCompletionPort((HANDLE)*socketSharedPtr, iocp, (ULONG_PTR)clientSharedPtr.get(), threadCount);
        }

        mConnected = true;
	}

    void Network::Process(int threadCount)
    {
        if (!mConnected)
            return;

        for (int i = 0;i < threadCount; ++i)
        {
            std::thread sessionThread(&Network::Work, this);
            sessionThread.detach();
        }

        for (auto& client : mClientMap)
        {
            auto context = mOverlappedQueue.pop();
            client.second->ReceiveReady(*context);
        }
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

                auto finder = mClientMap.find(completionKey);
                if (finder == mClientMap.end())
                {
                    std::string log = "Client Find Fail";
                    std::cout << log << std::endl;
                    continue;
                }
                auto client = finder->second;

                switch (targetOverlapped->mOperationType)
                {
                case OperationType::OP_ACCEPT:
                {
                    targetOverlapped->Clear();
                    mOverlappedQueue.push(std::move(targetOverlapped));
                    break;
                }
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
                        int rank = message_wrapper->ranking();
                        bool inRanking = message_wrapper->in_ranking();

                        std::string log = "[Network] RESPONSE_PLAYER_RANKING - Player ID: " + playerId + ", Score: " + std::to_string(score) + ", In Ranking: " + (inRanking ? "Yes" : "No") +" rank : " + std::to_string(rank);
                        std::cout << log << std::endl;

                        targetOverlapped->Clear();
                        mOverlappedQueue.push(std::move(targetOverlapped));

                        auto context = mOverlappedQueue.pop();
                        client->ReceiveReady(*context);



                        break;
                    }
                    case protocol::MessageContent_RESPONSE_SAVE_SCORE:
                    {
                        auto  RESPONSE_SAVE_SCORE = flatbuffers::GetRoot<protocol::RESPONSE_SAVE_SCORE>(targetOverlapped->wsabuf[1].buf);
                        bool feedback = RESPONSE_SAVE_SCORE->feedback();
                        std::string player_id = RESPONSE_SAVE_SCORE->player_id()->str();
                        std::string feedback_str = (feedback ? "Success" : "Failure");
                        std::string log = "[Network] RESPONSE_SAVE_SCORE - "+ player_id + " Save Score " + feedback_str;

                        std::cout << log << std::endl;

                        targetOverlapped->Clear();
                        mOverlappedQueue.push(std::move(targetOverlapped));

                        auto context = mOverlappedQueue.pop();
                        client->ReceiveReady(*context);
                        break;
                    }
                    default:
                    {
                        std::cout << "[Network] Error Message Type Receive" << std::endl;
                        break;
                    }
                    }
                    break;
                }
                case OperationType::OP_SEND:
                {
                    targetOverlapped->Clear();
                    mOverlappedQueue.push(std::move(targetOverlapped));
                    //std::cout << "Send Success" << std::endl;
                    break;
                }
                default:
                {
                    targetOverlapped->Clear();
                    mOverlappedQueue.push(std::move(targetOverlapped));
                    //std::cout << "??? Success" << std::endl;
                    break;
                }
                }
            }
        }

    }
}