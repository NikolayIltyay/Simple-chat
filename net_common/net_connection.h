#pragma once
#include "net_common.h"
#include "net_message.h"
#include "net_ts_queue.h"

namespace net
{
	template<typename T>
	class connection : public std::enable_shared_from_this<connection<T>>
	{
	public:
		enum class owner {
			server,
			client
		};

		connection(owner type, asio::io_context& asioContext, asio::ip::tcp::socket socket, net::tsqueue<net::owned_message<T>>& queueIn)
			:type_(type),
			context_(asioContext),
			socket_(socket),
			queueIn_(queueIn)
		{

		}

		void send(const net::message<T>& message)
		{

			asio::post(context_,
				[this, message]()
			{
				queueOut_.push_back(message);
				if (queueOut_.empty())
					writeHeader();
			});

		}

		//net::owned_message<T>& getInMessageQueue()
		//{
		//	return queueIn_;
		//}

		void connectToServer(asio::ip::tcp::resolver::results_type& endpoints)
		{
			if (type_ == owner::server)
				return;

			asio::async_connect(socket_, endpoints,
			[this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
			{
				if (!ec)
					readHeader();
				else
				{
					std::cout << "Failed to connect" << std::endl;
					disconnect();
				}
			});
		}

		void connectToClient(uint32_t id)
		{
			if (type_ == owner::client)
				return;

			if (!socket_.is_open())
				return;

			id_ = id;

			readHeader();
		}

		void disconnect()
		{
			if (isConnected())
				asio::post(context_,
					[this]()
			{
				socket_.close();
			});
		}

		bool isConnected() const
		{
			return socket_.is_open();
		}

		uint32_t getID() const
		{
			return this->id_;
		}

	private:

		void readHeader()
		{
			asio::async_read(socket_, asio::buffer(&tempMessage_.header, sizeof(net::message<T>)),
				[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					if (tempMessage_.header.size > 0)
					{
						tempMessage_.body.resize(tempMessage_.header.size);
						AddToIncommingMessageQueue();
					}
					else
						readHeader();
				}
				else
				{
					std::cout << "[" << id_ << "] read header failed" << std::endl;
					socket_.close();
				}
			});
		}

		void readBody()
		{
			asio::async_read(socket_, asio::buffer(tempMessage_.body.data(), tempMessage_.body.size()),
				[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					AddToIncommingMessageQueue();
				}
				else
				{
					std::cout << "[" << id_ << "] read body failed" << std::endl;
					socket_.close();
				}
			});
		}

		void writeHeader()
		{
			asio::async_write(socket_, asio::buffer(&queueOut_.front(), sizeof(net::message_header<T>)),
				[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					if (queueOut_.front().body.size() > 0)
						writeBody();
					else
					{
						queueOut_.popFront();
						if (!queueOut_.empty())
							writeHeader();
					}
				}
				else
				{
					std::cout << "[" << id_ << "] write header failed" << std::endl;
					socket_.close();
				}
			});
		}

		void writeBody()
		{
			asio::async_write(socket_, asio::buffer(queueOut_.front().body.data(), queueOut_.front().body.size()),
				[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					queueOut_.popFront();
					if (!queueOut_.empty())
						writeHeader();
				}
				else
				{
					std::cout << "[" << id_ << "] write body failed" << std::endl;
					socket_.close();
				}
			});
		}

		void AddToIncommingMessageQueue()
		{
			if (type_ == owner::server)
				queueIn_.push_back({ this->shared_from_this(), tempMessage_ });
			else
				queueIn_.push_back({ nullptr , tempMessage_ });

			readHeader();
		}


	private:

		owner type_;
		asio::io_context& context_;
		asio::ip::tcp::socket socket_;

		net::tsqueue<net::message<T>> queueOut_;
		net::tsqueue<net::owned_message<T>>& queueIn_;

		net::message<T> tempMessage_;

		uint32_t id_{ 0 };
	};
}

