#include "chat_server.h"
#include "message_types.h"
#include <chrono>


int main()
{
	chat_server<MessageType> server(60000);
	server.start();

	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		server.update();
	}

	return 0;
}
