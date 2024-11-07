#pragma once
#include "net_common.h"
#include "net_client.h"
#include "message_types.h"
#include "console_message_handler.h"

struct UserData
{
	std::string userName;
	std::string host;
	uint16_t port{ 0 };
};


class ChatClient : public net::net_client<MessageType>
{
public:
	void sendMessage(const UserMessage& userMessage);

	void handleInput(); 

	void setUserData(const UserData& data);

private:
	virtual void handleInMessage(const net::owned_message<MessageType>& message) override;

private:
	UserData m_userData;
	ConsoleMessageHandler m_console;
};