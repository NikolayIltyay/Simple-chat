#include "net_common.h"
#include "net_connection.h"
#include "net_client.h"
#include "net_server.h"

#include "console_message_handler.h"
#include "message_types.h"
#include "chat_client.h"

#include <chrono>


UserData getUserData(int argc, char* argv[])
{
	if (argc < 4)
		throw std::runtime_error("Invalid argument list");

	UserData data;
	data.userName = argv[1];
	data.host = argv[2];
	data.port = static_cast<uint16_t>(std::stoi(argv[3]));

	return data;
}

int main(int argc, char* argv[])
{
	UserData data;
	try {
		data = getUserData(argc, argv);
	}
	catch (const std::runtime_error& ex)
	{
		std::cerr << ex.what();
		return -1;
	}

	ChatClient client;
	client.setUserData(data);
	client.connect(data.host, data.port);

	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		client.handleInput();
		client.update();
	}

	return 0;
}

