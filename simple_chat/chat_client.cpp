#include "chat_client.h"


void ChatClient::sendMessage(const UserMessage& userMessage)
{
	net::message<MessageType> message;
	message.header.id = MessageType::UserMessage;
	message << serializeUserMessage(userMessage);

	send(message);
}

void ChatClient::handleInMessage(const net::owned_message<MessageType>& message)
{
	switch (message.msg.header.id)
	{
		case MessageType::UserMessage:
		{
			auto userMessage = deserializeUserMessage(message.msg.body);
			m_console.outputMessage(userMessage);
			break;
		}
		case MessageType::ServerAccepted:
		{
			m_console.outputString("Server accepted connection.");
			break;
		}
	}
}

void ChatClient::handleInput()
{
	auto input = m_console.getInput();
	if (input.empty())
		return;

	UserMessage userMessage;
	userMessage.message = input;
	userMessage.userName = m_userData.userName;
	userMessage.time = 0;

	sendMessage(userMessage);

}

void ChatClient::setUserData(const UserData& data)
{
	m_userData = data;
}

