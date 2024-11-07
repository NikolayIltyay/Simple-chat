#pragma once
#include <vector>
#include <string>

enum class MessageType
{
	UserMessage,
	ServerAccepted,
	ClientDisconnected
};

struct UserMessage
{
	std::string userName;
	uint64_t time;
	std::string message;
};


std::vector<uint8_t> serializeUserMessage(const UserMessage& message);

UserMessage deserializeUserMessage(const std::vector<uint8_t>& buffer);