#pragma once
#include "net_common.h"
#include "net_message.h"
#include "net_ts_queue.h"
#include "net_connection.h"

#include <memory>

namespace net
{
	template<typename T>
	class net_client
	{
	public:
		net_client(){}

		bool connect(const std::string& host, const uint16_t port)
		{
			try
			{
				asio::ip::tcp::resolver resolver(context_);
				auto endPoints = resolver.resolve(host, std::to_string(port));

				connection_ = std::make_unique<net::connection<T>>(net::connection<T>::owner::client, context_, asio::ip::tcp::socket(context_), queueIn_);
				connection_->connectToServer(endPoints);

				contextThread_ = std::thread([this]()
				{
					context_.run();
				});

			}
			catch (const std::exception& ex)
			{
				std::cerr << "Client exception: " << ex.what() << std::endl;
				return false;
			}

			return true;
		}

		void send(const net::message<T>& message)
		{
			if (isConnected())
				connection_->send(message);
		}

		bool update()
		{
			if (!isConnected())
				return false;

			if (!queueIn_.empty())
			{
				handleInMessage(queueIn_.popFront());
				return true;
			}

			return false;
		}

		void disconnect()
		{
			if (isConnected())
				connection_.disconnect();

			context_.stop();

			if (contextThread_.joinable())
				contextThread_.join();

			connection_.release();
		}

		bool isConnected() const
		{
			if (!connection_)
				return false;

			return connection_->isConnected();
		}

	private:
		virtual void handleInMessage(const net::owned_message<T>& message) = 0;

	private:
		asio::io_context context_;
		std::thread contextThread_;
		std::unique_ptr<net::connection<T>>  connection_;

		net::tsqueue<net::owned_message<T>> queueIn_;
	};
}
