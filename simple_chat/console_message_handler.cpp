#include "console_message_handler.h"
#include <iostream>
#include <optional>
#include <chrono>

using namespace std::chrono_literals;

namespace
{
	std::string getTimeString(uint64_t secondsSinceEpoch)
	{
		auto timePoint = std::chrono::system_clock::from_time_t(static_cast<std::time_t>(secondsSinceEpoch));

		std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
		std::tm localTime;
		localtime_s(&localTime, &time);

		int hours = localTime.tm_hour;
		int minutes = localTime.tm_min;

		std::stringstream ss;
		ss << hours << ":";
		if (minutes < 10)
			ss << "0";
		ss << minutes;

		return ss.str();
	}
}

ConsoleMessageHandler::ConsoleMessageHandler()
{
	startReading();
}

void ConsoleMessageHandler::startReading()
{
	m_future = std::async([]() {
		std::string str;
		std::getline(std::cin, str);
		return str;
	});
}

std::string ConsoleMessageHandler::getInput()
{
	if (m_future.wait_for(0s) == std::future_status::timeout)
		return "";

	auto input = m_future.get();

	startReading();

	return input;
}

void ConsoleMessageHandler::outputString(const std::string& message)
{
	std::cout << message << std::endl;
}

void ConsoleMessageHandler::outputMessage(const UserMessage& message)
{
	std::cout << message.userName
		<< " [" << getTimeString(message.time) << "]: "
		<< message.message << std::endl;
}