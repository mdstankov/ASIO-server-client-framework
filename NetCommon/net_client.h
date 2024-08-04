#pragma once

#include "net_common.h"
#include "net_message.h"
#include "net_tsqueue.h"
#include "net_connection.h"

namespace net
{
	template<typename T> 
	class client_interface
	{
	public:
		client_interface() 
		{}
			
		virtual ~client_interface()
		{
			// If the client is destroyed, always try to disconnect from server
			Disconnect();
		}
	public:
		// Connect to server with hostname/ip-address and port
		bool Connect( const std::string& host, const uint16_t port)
		{
			try
			{
				// Resolve hostname/ip-adress into tangiable physical address
				// can throw exception
				asio::ip::tcp::resolver resolver(m_context);
				asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

				// Create connection
				m_connection = std::make_unique<connection<T>>(
					connection<T>::owner::client,
					m_context,
					asio::ip::tcp::socket(m_context), m_qMessageIn);

				// Connect to the server
				m_connection->ConnectToServer(endpoints);


				// Start context thread
				thrContext = std::thread([this]() {m_context.run();});

			}
			catch( std::exception& e)
			{
				std::cerr << "Client Exception: " << e.what() << "\n";
				return false;
				
			}

			return true;
		}

		// Disconnect from server
		void Disconnect()
		{
			if(IsConnected())
			{
				// ...disconnect from server gracefully
				m_connection->Disconnect();
			}

			// We are done with the asio context and its thread
			m_context.stop();
			if(thrContext.joinable())
			{
				thrContext.join();
			}

			// Destroy the connection object
			m_connection.release();
		}
		
		// If connection is valid to a server
		bool IsConnected( )
		{
			if(m_connection)
			{
				return m_connection->IsConnected();
			}
			return false;		
		}

		// Retrive queue of messages from server
		tsqueue<owned_message<T>> &Incoming( )
		{
			return m_qMessageIn;
		}

		// Send message to server
		void Send(const message<T>& msg)
		{
			if (IsConnected())
				m_connection->Send(msg);
		}

	protected:
		// asio context handles the data transfer
		asio::io_context m_context;
		// but needs a thread of its own to execute its work command
		std::thread thrContext;

		// The Client has a single instance of a "connection" object, 
		// which handles data transfer
		std::unique_ptr<connection<T>> m_connection;

	private:
		// This is the thread save queue of incoming messages from server
		tsqueue<owned_message<T>> m_qMessageIn;
	};
}
