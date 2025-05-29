#pragma once
#include<memory>

#include<MSWSock.h>
#include <winsock2.h>

#include "flatbuffers/flatbuffers.h"
#include "RANKING_PROTOCOL_generated.h"

#define BUFFER_SIZE 1024

namespace Network
{
    enum OperationType
    {
        OP_DEFAULT = 0,
        OP_ACCEPT = 1,
        OP_RECV = 2,
        OP_SEND = 3,
    };

    struct MessageHeader
    {
        uint32_t socket_id;
        uint32_t body_size;
        uint32_t contents_type;

        MessageHeader(uint32_t socketID, uint32_t bodySize, uint32_t contentsType) : socket_id(socketID), body_size(bodySize), contents_type(contentsType)
        {

        }

        MessageHeader(const MessageHeader& other) : socket_id(other.socket_id), body_size(other.body_size), contents_type(other.contents_type)
        {
        }
    };

    struct CustomOverlapped :OVERLAPPED
    {
        WSABUF mWsabuf[2];
        SOCKET* mSocket;
        OperationType mOperationType;

        CustomOverlapped()
        {
            mSocket = nullptr;
            mWsabuf[0].buf = nullptr;
            mWsabuf[0].len = 0;
            mWsabuf[1].buf = nullptr;
            mWsabuf[1].len = 0;
            mOperationType = OperationType::OP_DEFAULT;
            this->hEvent = NULL;
        }

        ~CustomOverlapped()
        {
            mSocket = nullptr;
            mWsabuf[0].buf = nullptr;
            mWsabuf[0].len = 0;
            mWsabuf[1].buf = nullptr;
            mWsabuf[1].len = 0;
            mOperationType = OperationType::OP_DEFAULT;
            this->hEvent = NULL;
        }

        // 복사 생성자
        CustomOverlapped(const CustomOverlapped& other)
        {
            this->hEvent = other.hEvent;

            if (other.mWsabuf[0].len > 0)
            {
                mWsabuf[0].buf = other.mWsabuf[0].buf;
                mWsabuf[0].len = other.mWsabuf[0].len;
            }
            else
            {
                mWsabuf[0].buf = nullptr;
                mWsabuf[0].len = 0;
            }

            if (other.mWsabuf[1].len > 0)
            {
                mWsabuf[1].buf = other.mWsabuf[1].buf;
                mWsabuf[1].len = other.mWsabuf[1].len;
            }
            else
            {
                mWsabuf[1].buf = nullptr;
                mWsabuf[1].len = 0;
            }

            mOperationType = other.mOperationType;
        }

        void SetHeader(char* headerBuffer, ULONG headerLen)
        {
            mWsabuf[0].buf = headerBuffer;
            mWsabuf[0].len = headerLen;
        }

        void SetBody(char* bodyBuffer, ULONG bodyLen)
        {
            mWsabuf[1].buf = bodyBuffer;
            mWsabuf[1].len = bodyLen;
        }

        void SetOperationType(OperationType operationType)
        {
            mOperationType = operationType;
        }

        void Clear()
        {
            mWsabuf[0].buf = nullptr;
            mWsabuf[0].len = 0;
            mWsabuf[1].buf = nullptr;
            mWsabuf[1].len = 0;
            mOperationType = OperationType::OP_DEFAULT;
            this->hEvent = NULL;
        }
    };
}