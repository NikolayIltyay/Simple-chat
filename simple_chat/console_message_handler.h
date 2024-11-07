#pragma once
#include "message_types.h"
#include <string>
#include <future>

class ConsoleMessageHandler
{
public:
	ConsoleMessageHandler();

	std::string getInput();
	void outputString(const std::string& string);
	void outputMessage(const UserMessage& message);

private:
	void startReading();

private:
	std::future<std::string> m_future;
};