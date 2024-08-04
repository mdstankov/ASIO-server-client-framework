#include <iostream>
#include <net_full.h>

// Should be moved to common header used by client and server
enum class CustomMsgTypes : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage,
};

class CustomServer : public net::server_interface<CustomMsgTypes>
{
public:
	CustomServer(uint16_t nPort): net::server_interface<CustomMsgTypes>(nPort)
	{
	
	}
protected:
	virtual bool OnClientConnect(std::shared_ptr<net::connection<CustomMsgTypes>> client)
	{
		net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::ServerAccept;
		client->Send(msg);

		return true;
	}

	// Called when a client appears to disconnected
	virtual void OnClientDisconnect(std::shared_ptr<net::connection<CustomMsgTypes>> client)
	{
		std::cout << "Removing client [" << client->GetID() << "]\n";
	}

	// Called when message arrives
	virtual void OnMessage( std::shared_ptr<net::connection<CustomMsgTypes>> client, net::message<CustomMsgTypes>& msg)
	{
		switch (msg.header.id)
		{
		case CustomMsgTypes::ServerPing:
		{		
			// Bounce message back to client
			std::cout<< "[" << client->GetID() << "]: Server Ping from " <<"\n";
			client->Send(msg);
		}
		break;

		case CustomMsgTypes::MessageAll:
		{		
			// Message other clients 
			std::cout<< "[" << client->GetID() << "]: Message to All " <<"\n";
			net::message<CustomMsgTypes> msgB;
			msgB.header.id = CustomMsgTypes::ServerMessage;
			msgB << client->GetID();
			//Ignore client sending the message to all
			MessageAllClients(msgB, client);

		}
		break;

		default:
			break;
		}

	}
};


int main()
{
	CustomServer server(60000);
	server.Start();
	while(1)
	{
		server.Update(-1, true);
	}

	return 0;
}