#pragma once

#include "CustomPacket.h"



#define BUFFER_SIZE 1024

namespace Business
{
	flatbuffers::FlatBufferBuilder builder;

	static uint32_t Create_Request_Player_Ranking(std::string playerId, char* headerBuffer, char* bodyBuffer)
	{
		auto id = builder.CreateString(playerId);
		builder.Finish(protocol::CreateREQUEST_PLAYER_RANKING(builder, id));
		auto data = builder.GetBufferPointer();
		auto size = builder.GetSize();

		char* message = reinterpret_cast<char*>(data);


		memset(headerBuffer, 0, sizeof(Network::MessageHeader));
		memset(bodyBuffer, 0, BUFFER_SIZE);

		auto header_id = 0;
		auto header_body_size = htonl(size);
		auto header_contents_type = htonl(static_cast<uint32_t>(protocol::MessageContent_REQUEST_PLAYER_RANKING));

		Network::MessageHeader newHeader(header_id, header_body_size, header_contents_type);
		std::memcpy(headerBuffer, &newHeader, sizeof(Network::MessageHeader));
		std::memcpy(bodyBuffer, message, size);

		return size;
	}

	static uint32_t Create_Request_Save_Score(std::string playerId, int score, std::time_t lastUpdate, char* headerBuffer, char* bodyBuffer)
	{
		auto id = builder.CreateString(playerId);
		auto utcTime = static_cast<uint64_t>(lastUpdate);

		builder.Finish(protocol::CreateREQUEST_SAVE_SCORE(builder, id, score, utcTime));
		auto data = builder.GetBufferPointer();
		auto size = builder.GetSize();

		char* message = reinterpret_cast<char*>(data);

		memset(headerBuffer, 0, sizeof(Network::MessageHeader));
		memset(bodyBuffer, 0, BUFFER_SIZE);

		auto header_id = 0;
		auto header_body_size = htonl(size);
		auto header_contents_type = htonl(static_cast<uint32_t>(protocol::MessageContent_REQUEST_SAVE_SCORE));

		Network::MessageHeader newHeader(header_id, header_body_size, header_contents_type);
		std::memcpy(headerBuffer, &newHeader, sizeof(Network::MessageHeader));
		std::memcpy(bodyBuffer, message, size);

		return size;
	}
}