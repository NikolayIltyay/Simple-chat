#pragma once

#include "net_common.h"
#include <memory>

namespace net
{
	template<typename T>
	struct is_pointer { static const bool value = false; };

	template<typename T>
	struct is_pointer<T*> { static const bool value = true; };

	template<typename T>
	struct message_header
	{
		T id;
		uint32_t size;
	};

	template<typename T>
	struct message
	{
		message_header<T> header{};
		std::vector<uint8_t> body;

		size_t size() const
		{
			return body.size();
		}

		friend std::ostream operator<<(std::ostream& os, const message<T>& msg)
		{
			os << "ID:" << int(msg.header.id) << " Size:" << msg.header.size;
			return os;
		}

		template<typename DataType>
		message<T>& operator <<(const DataType& data)
		{
			static_assert(std::is_standard_layout<DataType>::value, "Data is not standart layout.");
			static_assert(!std::is_pointer<DataType>::value, "Pointer data is not allowed.");

			auto prevSize = body.size();
			body.resize(prevSize + sizeof(DataType));
			std::memcpy(body.data() + prevSize, reinterpret_cast<void const*>(&data), sizeof(DataType));

			header.size = size();

			return *this;
		}

		template<typename DataType>
		message<T>& operator >>(DataType& data)
		{
			static_assert(std::is_standard_layout<DataType>::value, "Data is not standart layout.");
			static_assert(!std::is_pointer<DataType>::value, "Pointer data is not allowed.");

			auto pos = body.size() - sizeof(DataType);
			std::memcpy(reinterpret_cast<void*>(&data), body.data() + pos, sizeof(DataType));
			body.resize(pos);

			header.size = size();

			return *this;
		}

		message<T>& operator <<(const std::string& str)
		{
			auto prevSize = body.size();
			body.resize(prevSize + str.size());
			std::memcpy(body.data() + prevSize, reinterpret_cast<void const*>(str.data()), str.size());

			header.size = size();

			return *this;
		}

		message<T>& operator >>(std::string& str)
		{
			str.resize(body.size());
			std::memcpy(reinterpret_cast<void*>(const_cast<char*>(str.data())), reinterpret_cast<const void*>(body.data()), body.size());
			body.clear();

			header.size = size();

			return *this;
		}
	};

	template                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   <typename T>
	class connection;

	template<typename T>
	struct owned_message
	{
		std::shared_ptr<connection<T>> remote = nullptr;
		message<T> msg;

		friend std::ostream& operator<<(std::ostream& os, const owned_message<T>& msg)
		{
			os << msg.msg;
			return os;
		}
	};
}
