#pragma once
#include <net_server.h>
#include "message_types.h"

#include <iostream>

template<typename T>
class chat_server : public net::server<T>
{
public:
	chat_server(uint32_t port):
		net::server<T>(port)
	{

	}

	virtual bool onNewConnection(std::shared_ptr<net::connection<T>> client) override
	{
		if (!client->isConnected())
			return false;

		net::message<MessageType> msg;
		msg.header.id = MessageType::ServerAccepted;
		client->send(msg);
		return true;
	}

	virtual void onDisconnect(std::shared_ptr<net::connection<T>> client) override
	{
		std::cout << "Client " << client->getID() << " is diconnected" << std::endl;
	}

	virtual bool onMessage(std::shared_ptr<net::connection<T>> client, const net::message<T>& message) override
	{
		if (message.header.id != MessageType::UserMessage)
			return false;

		net::server<T>::sendToAllClients(message, client);
		return true;
	}

};