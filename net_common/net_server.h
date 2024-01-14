#pragma once
#include "net_common.h"
#include "net_message.h"
#include "net_ts_queue.h"
#include "net_connection.h"

namespace net
{
	template<typename T>
	class server
	{
	public:
		server(uint32_t port) :
			acceptor_(context_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
		{

		}

		virtual ~server()
		{
			stop();
		}

		bool start()
		{
			try
			{
				waitForClientConnection();
				contextThread_ = std::thread([this]()
				{
					context_.run();
				});
			}
			catch (const std::exception& ex)
			{
				std::cout << "[SEVER] exception: " << ex.what() << std::endl;
				return false;
			}

			std::cout << "[SERVER] started!" << std::endl;
			return true;

		}

		void stop()
		{
			context_.stop();
			if (contextThread_.joinable())
				contextThread_.join();

			std::cout << "[SERVER] stopped." << std::endl;
		}


		void sendToClient(net::connection<T>& client, const net::message<T>& message)
		{
			if (client && client->isConnected())
			{
				client->send(message);

			}
			else
			{
				onDisconnect(client);
				client.reset();
				connections_.erase(std::remove(connections_.begin(), connections_.end(), client), connections_.end());
			}
		}

		void sendToAllClients(const net::message<T>& message)
		{
			bool invalidClientExist = false;

			for (auto& client : connections_)
			{
				if (client && client->isConnected())
				{
					client->send(message);
				}
				else
				{
					onDisconnect(client);
					client.reset();
					invalidClientExist = true;

				}
			}
			if (invalidClientExist)
				connections_.erase(std::remove(connections_.begin(), connections_.end(), nullptr), connections_.end());
		}

		void waitForClientConnection()
		{
			acceptor_.async_accept(
				[this](std::error_code ec, asio::ip::tcp::socket socket)
			{
				if (!ec)
				{
					std::cout << "[SERVER] New connection: " << socket.remote_endpoint() << std::endl;
					auto newConnection = std::make_shared<net::connection<T>>(connection<T>::owner::server, context_, std::move(socket), queueIn_);
					if (onNewConnection(newConnection))
					{
						connections_.push_back(newConnection);
						connections_.back()->connectToClient(nIDCounter_++);
						std::cout << "[SERVER] - [" << connections_.back()->getID() << "] connection approved" << std::endl;
					}
					else
					{
						std::cout<<"[SERVER] Connection denied: " << socket.remote_endpoint() << std::endl;
					}
				}
				else
				{
					std::cout << "Connection error: " << ec.message() << std::endl;
				}
				waitForClientConnection();
			});
		}

		void update(size_t maxMessages = -1)
		{
			size_t messageCout = 0;
			while (messageCout < maxMessages && !queueIn_.empty())
			{
				auto msg = queueIn_.popFront();
				onMessage(msg.remote, msg.msg);
				messageCout++;
			}
		}

		virtual bool onNewConnection(std::shared_ptr<connection<T>> client) = 0;

		virtual void onDisconnect(std::shared_ptr<connection<T>> client) = 0;

		virtual bool onMessage(std::shared_ptr<connection<T>> client, const net::message<T>& message) = 0;


	private:
		asio::io_context context_;
		std::thread contextThread_;
		std::vector<std::shared_ptr<net::connection<T>>> connections_;
		net::tsqueue<net::owned_message<T>> queueIn_;

		asio::ip::tcp::acceptor acceptor_;
		uint32_t nIDCounter_{0};
	};
}