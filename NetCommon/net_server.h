#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"
#include "net_connection.h"

namespace net
{
	template<typename T>
	class server_interface
	{
	public:
		server_interface(uint16_t port)
			: m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
		{
		
		}

		virtual ~server_interface()
		{
			Stop();
		}

		bool Start()
		{
			try
			{
				//impodtant order of commands
				WaitForClientConnection();

				// Launch the asio context in its own thread
				m_threadContext = std::thread([this](){m_asioContext.run();});			
			}
			catch( std::exception& e)
			{
				// Something prohibited the server from listening 
				std::cerr << "[SERVER] Exception: " << e.what() << "\n";
				return false;
			}

			std::cout << "[SERVER] Started!\n";
			return true;
		}

		void Stop()
		{
			// Request the context to close
			m_asioContext.stop();

			// Tidy up the context thread
			if(m_threadContext.joinable())
			{
				m_threadContext.join();
			}

			std::cout << "[SERVER] Stopped!\n";
		}

		// ASYNC - instruct asio to wait for connection
		void WaitForClientConnection()
		{
			m_asioAcceptor.async_accept(
			[this](std::error_code ec, asio::ip::tcp::socket socket)
			{
				if(!ec)
				{
					// remote_endpoint gives the ip of the client
					std::cout << "[SERVER] New connection: " << socket.remote_endpoint() << "\n";

					// Create a new connection to handle this client 
					std::shared_ptr<connection<T>> newconn = 
						std::make_shared<connection<T>>(connection<T>::owner::server, 
							m_asioContext, std::move(socket), m_qMessagesIn);

					// Server might deny the connection
					if( OnClientConnect(newconn))
					{
						// Connection allowed, so add to container of new connections
						m_deqConnections.push_back(std::move(newconn));

						// Issue a task to the connection's
						// asio context to sit and wait for bytes to arrive!
						m_deqConnections.back()->ConnectToClient(this, nIDCounter++);

						std::cout<< "[" << m_deqConnections.back()->GetID() << "] Connection Approved\n";
					}
					else
					{
						std::cout << "[-----] Connection Denied\n";
					}
				}
				else
				{
					// Error during acceptance
					std::cerr << "[SERVER] New Connection Error: " << ec.message() << "\n";
				}

				// Prime the asio context with more work to wait for another 
				// connection and not end...
				WaitForClientConnection();			
			});
		}
	
		// ASYNC - instruct asio to wait for connection
		void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg)
		{
			if(client && client->IsConnected())
			{
				client->Send(msg);
			}
			else
			{
				//if client disconnected between
				OnClientDisconnect(client);
				client.reset();
				m_deqConnections.erase(std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end());
			}
		}

		// Send message to all clients
		void MessageAllClients(const message<T>& msg, std::shared_ptr<connection<T>> pIgnoreClient = nullptr )
		{
			bool bInvalidClientExist = false;

			for( auto& client : m_deqConnections)
			{
				// Check client is connected
				if(client && client->IsConnected())
				{
					if(client != pIgnoreClient)
					{
						client->Send(msg);		
					}				
				}	
				else
				{
					// The client couldn't be contacted, so assume it has disconencted
					OnClientDisconnect(client);
					client.reset();
					bInvalidClientExist = true;
				}
			}

			if(bInvalidClientExist)
			{
				m_deqConnections.erase(
					m_deqConnections.erase(
						std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end()));							
			}
		}

		void Update(size_t nMaxMessages = -1, bool bWait = false)
		{
			if (bWait) 
			{
				m_qMessagesIn.wait();
			}

			// Let user handle when messages are handled
			// setting size_t to -1, sets it to the max number ofmessages

			size_t nMessageCount = 0;
			while( nMessageCount < nMaxMessages && !m_qMessagesIn.empty())
			{
				// Grab the front message
				auto msg = m_qMessagesIn.pop_front();

				// Pass to message handler
				OnMessage(msg.remote, msg.msg);

				nMessageCount++;
			}		
		}

	public:
		// Called when a client is validated
		virtual void OnClientValidated(std::shared_ptr<connection<T>> client)
		{
		}

	protected:
		// Called when client connects, can veto the connection by returning false
		virtual bool OnClientConnect(std::shared_ptr<connection<T>> client)
		{
			return false;
		}

		// Called when a client appears to disconnected
		virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client)
		{

		}

		// Called when message arrives
		virtual void OnMessage( std::shared_ptr<connection<T>> client, message<T>& msg)
		{
		
		}

	protected:
		// Thread safe queuee of incomming message packets
		tsqueue<owned_message<T>> m_qMessagesIn;

		// Container of active validated connections
		std::deque<std::shared_ptr<connection<T>>> m_deqConnections;

		// Order of declaration is imporant, as its also order of initialisation
		asio::io_context m_asioContext;
		std::thread m_threadContext;

		//the address from whom the server will listen for connections
		// needs an asio context
		asio::ip::tcp::acceptor m_asioAcceptor;

		// Clients will be intentifed in the 'wider system' via an ID
		// clients have unique ips, but we avoid sending those across to other clients
		uint32_t nIDCounter = 10000;

	};
}



