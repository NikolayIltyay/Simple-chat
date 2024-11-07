#include "message_types.h"

#include <stdexcept>
#include <cassert>

std::vector<uint8_t> serializeUserMessage(const UserMessage& message)
{
	std::vector<uint8_t> buffer;

	assert(std::numeric_limits<uint8_t>::max() + 1 >= message.userName.size());

	uint8_t nameSize = static_cast<uint8_t>(message.userName.size());

	buffer.resize(sizeof(uint8_t) + message.userName.size()
		+ sizeof(uint64_t) + message.message.size());

	size_t currentSize = 0;
	std::memcpy(buffer.data() + currentSize, &nameSize, sizeof(nameSize));
	currentSize += sizeof(nameSize);

	std::memcpy(buffer.data() + currentSize, message.userName.data(), message.userName.size());
	currentSize += message.userName.size();

	std::memcpy(buffer.data() + currentSize, &message.time, sizeof(message.time));
	currentSize += sizeof(message.time);

	std::memcpy(buffer.data() + currentSize, message.message.data(), message.message.size());

	return buffer;
}

UserMessage deserializeUserMessage(const std::vector<uint8_t>& buffer)
{
	static auto minSize = sizeof(uint8_t) + sizeof(uint64_t);
	if (buffer.size() < minSize)
		throw std::runtime_error("Invalid user message buffer.");

	UserMessage message;

	size_t currentSize = 0;

	uint8_t nameSize = 0;
	std::memcpy(&nameSize + currentSize, buffer.data(), sizeof(nameSize));
	currentSize += sizeof(nameSize);

	if (nameSize)
	{
		message.userName.resize(nameSize);
		std::memcpy((void*)message.userName.data(), buffer.data() + currentSize, message.userName.size());
		currentSize += message.userName.size();
	}

	std::memcpy(&message.time, buffer.data() + currentSize, sizeof(message.time));
	currentSize += sizeof(message.time);

	message.message.resize(buffer.size() - currentSize);
	std::memcpy((void*)message.message.data(), buffer.data() + currentSize, message.message.size());

	return message;
}