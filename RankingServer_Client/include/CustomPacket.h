#pragma once
#include<memory>

#include<MSWSock.h>
#include <winsock2.h>

#include "flatbuffers/flatbuffers.h"
#include "RANKING_PROTOCOL_generated.h"

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
        WSABUF wsabuf[2];
        SOCKET* socket;
        OperationType mOperationType;

        CustomOverlapped()
        {
            socket = nullptr;
            wsabuf[0].buf = nullptr;
            wsabuf[0].len = 0;
            wsabuf[1].buf = nullptr;
            wsabuf[1].len = 0;
            mOperationType = OperationType::OP_DEFAULT;
            this->hEvent = NULL;
        }

        ~CustomOverlapped()
        {
            socket = nullptr;
            wsabuf[0].buf = nullptr;
            wsabuf[0].len = 0;
            wsabuf[1].buf = nullptr;
            wsabuf[1].len = 0;
            mOperationType = OperationType::OP_DEFAULT;
            this->hEvent = NULL;
        }

        // ���� ������
        CustomOverlapped(const CustomOverlapped& other)
        {
            this->hEvent = other.hEvent;

            if (other.wsabuf[0].len > 0)
            {
                wsabuf[0].buf = other.wsabuf[0].buf;
                wsabuf[0].len = other.wsabuf[0].len;
            }
            else
            {
                wsabuf[0].buf = nullptr;
                wsabuf[0].len = 0;
            }

            if (other.wsabuf[1].len > 0)
            {
                wsabuf[1].buf = other.wsabuf[1].buf;
                wsabuf[1].len = other.wsabuf[1].len;
            }
            else
            {
                wsabuf[1].buf = nullptr;
                wsabuf[1].len = 0;
            }

            mOperationType = other.mOperationType;
        }

        void SetHeader(char* headerBuffer, ULONG headerLen)
        {
            wsabuf[0].buf = headerBuffer;
            wsabuf[0].len = headerLen;
        }

        void SetBody(char* bodyBuffer, ULONG bodyLen)
        {
            wsabuf[1].buf = bodyBuffer;
            wsabuf[1].len = bodyLen;
        }

        void SetOperationType(OperationType operationType)
        {
            mOperationType = operationType;
        }

        void Clear()
        {
            wsabuf[0].buf = nullptr;
            wsabuf[0].len = 0;
            wsabuf[1].buf = nullptr;
            wsabuf[1].len = 0;
            mOperationType = OperationType::OP_DEFAULT;
            this->hEvent = NULL;
        }
    };
}