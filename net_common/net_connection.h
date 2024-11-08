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

		connection(owner type, asio::io_context& asioContext, asio::ip::tcp::socket&& socket, net::tsqueue<net::owned_message<T>>& queueIn)
			:type_(type),
			context_(asioContext),
			socket_(std::move(socket)),
			queueIn_(queueIn)
		{

		}

		uint64_t encrypt(uint64_t nInput)
		{
			uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE;
			out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F) << 4;
			return out ^ 0xC0DEFACE12345678;
		}

		void send(const net::message<T>& message)
		{

			asio::post(context_,
				[this, message]()
			{
				bool bWritingMessage = !queueOut_.empty();
				queueOut_.push_back(message);
				if (!bWritingMessage)
					writeHeader();
			});

		}

		void connectToServer(asio::ip::tcp::resolver::results_type& endpoints)
		{
			if (type_ == owner::server)
				return;

			asio::async_connect(socket_, endpoints,
			[this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
			{
				if (!ec)
				{
					waitForValidationNum();

				}
				else
				{
					std::cout << "Failed to connect" << std::endl;
					disconnect();
				}
			});
		}

		void waitForValidationNum()
		{
			asio::async_read(socket_, asio::buffer(&validationNum, sizeof(validationNum)),
				[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					validationNumEncrypted = encrypt(validationNum);

					asio::async_write(socket_, asio::buffer(&validationNumEncrypted, sizeof(validationNumEncrypted)),
						[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
							readHeader();
						}
						else
						{
							std::cout << "[" << ec.message()/*id_*/ << "] validation reply failed." << std::endl;
							socket_.close();
						}
					});
				}
				else
				{
					std::cout << "[" << ec.message()/*id_*/ << "] validation check failed" << std::endl;
					socket_.close();
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

			requestValidation();
		}

		void requestValidation()
		{
			validationNum = static_cast<uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());
			validationNumEncrypted = encrypt(validationNum);

			asio::async_write(socket_, asio::buffer(&validationNum, sizeof(validationNum)),
				[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					checkValidation();
				}
				else
				{
					std::cout << "[" << ec.message()/*id_*/ << "] validation request failed." << std::endl;
					socket_.close();
				}
			});
		}

		void checkValidation()
		{
			asio::async_read(socket_, asio::buffer(&validationNumToCheck, sizeof(validationNumToCheck)),
				[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					if (validationNumToCheck == validationNumEncrypted)
					{
						std::cout << id_ << " connection - validation passed." << std::endl;
						readHeader();
					}
					else
					{
						std::cout << id_ << " connection - validation failed." << std::endl;
						socket_.close();
					}
				}
				else
				{
					std::cout << "[" << ec.message()/*id_*/ << "] validation check failed" << std::endl;
					socket_.close();
				}
			});
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
			asio::async_read(socket_, asio::buffer(&tempMessage_.header, sizeof(net::message_header<T>)),
				[this](std::error_code ec, std::size_t length)
			{
				if (!ec)
				{
					if (tempMessage_.header.size > 0)
					{
						tempMessage_.body.resize(tempMessage_.header.size);
						readBody();
					}
					else
						AddToIncommingMessageQueue();
				}
				else
				{
					std::cout << "[" << ec.message()/*id_*/ << "] read header failed" << std::endl;
					
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
			asio::async_write(socket_, asio::buffer(&queueOut_.front().header, sizeof(net::message_header<T>)),
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

		uint64_t validationNum{ 0 };
		uint64_t validationNumEncrypted{ 0 };
		uint64_t validationNumToCheck{ 0 };
		//uint64_t validationRequestNum{ 0 };
		//uint64_t validationNumToCheck{ 0 };
		//uint64_t validationNumToSolve{ 0 };

	};
}

