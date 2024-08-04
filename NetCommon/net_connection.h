#pragma once

#include "net_tsqueue.h"
#include "net_message.h"
#include "net_server.h"

namespace net
{
	// Forward declare
	/*template<typename T>
	class server_interface;*/

	template<typename T>
	class connection : public std::enable_shared_from_this<connection<T>>  //creating shared pointer internally from this object
	{
	public:
		enum class owner
		{
			server,
			client		
		};

		connection( owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, tsqueue<owned_message<T>>& qIn)
			: m_asioContext(asioContext), m_socket(std::move(socket)), m_qMessagesIn(qIn)
		{
			m_nOwnerType = parent;

			// Construct validation check
			if(m_nOwnerType == owner::server)
			{
				// Connection Server->Client, construct random data for client
				// to transform and send back
				m_nHandshakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
				// Pre-calculate the result
				m_nHandshakeCheck = scramble(m_nHandshakeOut);
			}
			else
			{
				m_nHandshakeIn = 0;
				m_nHandshakeOut = 0;
			}

		}

		virtual ~connection()
		{}

		uint32_t GetID() const
		{
			return m_id;
		}	

	public:
		void ConnectToClient( net::server_interface<T>* server, uint32_t uid=0)
		{
			if(m_nOwnerType == owner::server)
			{
				if(m_socket.is_open())
				{
					m_id = uid;
					//start listening
					//ReadHeader();

					// A client has attempted to connect to the server
					// write out the handshake data to be validated
					WriteValidation();

					// Next, issue a task to sit and wait async for validation data
					// to be sent back from the client
					ReadValidation(server);
				}
			}
		}

		void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints) //clients only
		{
			// Onlyclients can connect
			if(m_nOwnerType == owner::client)
			{
				// Request asio to attempt to connect to an endpoint
				asio::async_connect(m_socket, endpoints,
					[this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
					{
						if(!ec)
						{
							ReadValidation();
							// Primed and ready to listen from messages 
							// From the server
							//ReadHeader();					
						}
					});
			}
		} 

		void Disconnect()
		{

		} //server client
		bool IsConnected() const
		{
			return m_socket.is_open();
		}


	public:
		void Send( const message<T>& msg)
		{
			// send a job to asio context, async
			asio::post(m_asioContext, 
				[this, msg]()
				{
					//in case asio is already writting or not
					//to avoid another workload and possible conflicts
					bool bWritingMessage = !m_qMessagesOut.empty();
					m_qMessagesOut.push_back(msg);
					if(!bWritingMessage)
					{
						WriteHeader();
					}
				});
		}

	private:
		// ASYNC - Prime context ready to read a message header
		void ReadHeader()
		{
			// header has fixed size
			asio::async_read(m_socket, 
				asio::buffer(&m_msgTemporaryIn.header, sizeof(message_header<T>)),
				[this](std::error_code ec, std::size_t lenght)
				{
					if(!ec)
					{
						//check if there is message body
						if(m_msgTemporaryIn.header.size > 0)
						{
							m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);
							ReadBody();
						}
						else
						{
							AddToIncomingMessageQueue();
						}
					}
					else
					{
						//force close socket
						std::cout<< "[" <<m_id<< "] Read Header Fail.\n";
						m_socket.close();
					}
				});
		}
		
		// ASYNC - Prime context ready to read a message body
		void ReadBody()
		{
			//we know the size of the data we need to read
			asio::async_read(m_socket, 
				asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()),
				[this](std::error_code ec, std::size_t length)
			{						
				if (!ec)
				{
					AddToIncomingMessageQueue();
				}
				else
				{				
					std::cout << "[" << m_id << "] Read Body Fail.\n";
					m_socket.close();
				}
			});

		}

		void AddToIncomingMessageQueue()
		{
			if( m_nOwnerType == owner::server)
			{
				m_qMessagesIn.push_back({this->shared_from_this(), m_msgTemporaryIn});
			}
			else
			{
				//clients have only one connection
				m_qMessagesIn.push_back({nullptr, m_msgTemporaryIn});
			}

			// Reg another task for asio context to perfrom here
			// wait to read another header
			ReadHeader();
		}

		// ASYNC - Prime context to write a message header
		void WriteHeader()
		{
			asio::async_write(m_socket, asio::buffer(&m_qMessagesOut.front().header, sizeof(message_header<T>)),
				[this](std::error_code ec, std::size_t length)
				
				{
					if(!ec)
					{
						if(m_qMessagesOut.front().body.size()>0)
						{
							WriteBody();
						}
						else
						{
							//pop out of queue and check for more messages 
							m_qMessagesOut.pop_front();
							if(!m_qMessagesOut.empty())
							{
								WriteHeader();
							}
						}
					}
					else
					{
						//force close socket
						std::cout<< "[" <<m_id<< "] Write Header Fail.\n";
						m_socket.close();
					}			
				});
		}

		// ASYNC - Prime context to write a message body
		void WriteBody()
		{
			asio::async_write(m_socket,
				asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),
				[this](std::error_code ec, std::size_t length)
			{
				if(!ec)
				{
					//pop out of queue and check for more messages 
					m_qMessagesOut.pop_front();

					if(!m_qMessagesOut.empty())
					{
						WriteHeader();
					}
				}
				else
				{
					//force close socket
					std::cout<< "[" <<m_id<< "] Write Body Fail.\n";
					m_socket.close();
				}
			});
		}
		
		// "Encrypt" data, temp
		uint64_t scramble(uint64_t nInput)
		{
			uint64_t out = nInput ^ 0xAAAADDDDAAAADDDD;
			out = (out & 0xF0F0F0F0F0F0F0 ) >>  4 | (out & 0x0F0F0F0F0F0F0F ) << 4;
			return out ^ 0xA16AD2D5AAA3DD47;						 
		}

		// Async 
		void WriteValidation()
		{
			asio::async_write(m_socket, 
				asio::buffer(&m_nHandshakeOut, sizeof(uint64_t)),
				[this](std::error_code ec, std::size_t lenght)
			{
				if(!ec)
				{
					// Validation data sent, client should sit and wait
					// for a response(or a closure)
					if( m_nOwnerType == owner::client)
					{
						ReadHeader();
					}
				}
				else
				{					
					m_socket.close();
				}
			});	
		}


		void ReadValidation( net::server_interface<T>* server = nullptr)
		{
			// read the bites of data into handshakeIn
			asio::async_read(m_socket, 
				asio::buffer(&m_nHandshakeIn, sizeof(uint64_t)),
				[this, server](std::error_code ec, std::size_t lenght)
			{
				if(!ec)
				{
					if( m_nOwnerType == owner::server)
					{
						if(m_nHandshakeIn == m_nHandshakeCheck)
						{
							// Client has provited valid solution
							std::cout << "Client Validated" << std::endl;
							server->OnClientValidated(this->shared_from_this());		

							// Sit and wait to receive data now
							ReadHeader();
						}
						else
						{
							// Client gave incorrect data, so disconnect
							// Can add client to ban list or counter in the future
							std::cout << "Client Disconnected (Fail Validation)\n";
							m_socket.close();						
						}
					}
					else
					{
						// Connection is client, solve puzzle
						m_nHandshakeOut = scramble(m_nHandshakeIn);
						// Write the result
						WriteValidation();
					}
				}
				else
				{
					std::cout<<"Client Disconnected (ReadValidation)\n";
					m_socket.close();
				}
			});	
		}

	protected:
		// Responsible for ASIO
		// Each connection has a unique socket to a remote
		asio::ip::tcp::socket m_socket;

		// Context shared with thew whole asio instance(server will have multiple connections)
		asio::io_context& m_asioContext;

		// Queue hold all messages to be send to remote side of this connection
		tsqueue<message<T>> m_qMessagesOut;

		// This queue hold all messages that have been received from the remote side
		// of this connection, It isa reference as the 'owner' of this connection is
		// expected to provide a queueu
		tsqueue<owned_message<T>>& m_qMessagesIn;

		message<T> m_msgTemporaryIn;
		// "Owner" dices how some of the connection behaves
		owner m_nOwnerType = owner::server;
		uint32_t m_id = 0;

		// Handshake Validation
		uint64_t m_nHandshakeOut = 0;
		uint64_t m_nHandshakeIn = 0;
		uint64_t m_nHandshakeCheck = 0;
	};

}
