//Add 'NetCommon' to Build Dependancies
//Add path to \NetCommon in Include Directories

#include <iostream>
#include <net_headers.h>
#include <net_client.h>
#include <Windows.h>

// message types, type specific to net message type
// Should be moved to common header used by client and server
enum class CustomMsgTypes : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage,
};

class CustomClient : public net::client_interface<CustomMsgTypes>
{
public:
	void PingServer()
	{
		net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::ServerPing;
		

		// ...system clock dependant on platform server/client
		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
		msg << timeNow;
		Send(msg);
	}

	void MessageAll()
	{
		net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::MessageAll;
		Send(msg);
	}

};

int main()
{
	CustomClient client;
	client.Connect( "172.24.0.1" , 60000);

	//keystates
	bool key[3] = {false,false,false};
	bool old_key[3] = {false,false,false};


	//Windows specific
	bool bQuit = false;
	while(!bQuit)
	{
		auto fore = GetForegroundWindow();
		auto con = GetConsoleWindow();
		////////
		// (GetForegroundWindow()==GetConsoleWindow())
		{
			key[0] = GetAsyncKeyState('1') & 0x8000;
			key[1] = GetAsyncKeyState('2') & 0x8000;
			key[2] = GetAsyncKeyState('3') & 0x8000;
		}

		if(key[0] && !old_key[0]) 
		{
			client.PingServer();
		}

		if(key[1] && !old_key[1]) 
		{
			client.MessageAll();
		}

		if(key[2] && !old_key[2]) 
		{
			bQuit = true;
		}

		for( int i = 0; i <3; i++) old_key[i] = key[i];
		////////

		if(client.IsConnected())
		{
			if(!client.Incoming().empty())
			{
				auto msg = client.Incoming().pop_front().msg;
				switch(msg.header.id)
				{
				case CustomMsgTypes::ServerPing:	
				{
					std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
					std::chrono::system_clock::time_point timeThen;
					msg >> timeThen;
					std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";					
				}
				break;
				case CustomMsgTypes::ServerMessage:
				{
					// responded to ping request
					uint32_t clientID;
					msg >> clientID;
					std::cout << "Message from client [" << clientID << "]\n";
				}
				break;
				case CustomMsgTypes::ServerAccept:
				{
					// Server has responded to a ping request				
					std::cout << "Server Accepted Connection\n";
				}
				break;

				default:
					break;
				}			
			}
		}
		else
		{
			std::cout << "Server Down\n";
			bQuit = true;
		}
	}

	return 0;
}
