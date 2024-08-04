#pragma once
#include "net_common.h"

namespace net
{
	// Message Header is sent at start of all messages
	// T allowes us to use 'enum class' to ensure message is valid at compile time
	template<typename T>
	struct message_header
	{
		T id{};
		uint32_t size = 0;	//concurent in 32/64 bit system
	};

	template <typename T>
	struct message
	{
		message_header<T> header{};
		std::vector<uint8_t> body;

		size_t size() const
		{
			return body.size();
		}

		// override of cout compatibility to produce friendly description of message
		friend std::ostream& operator << (std::ostream& os, const message<T>& msg)
		{
			os << "ID: " <<int(msg.header.id) << " Size: " << msg.header.size;
			return os;		
		}
	
		// Pushes any POD-like data into message buffer,
		template <typename DataType>
		friend message<T>& operator << (message<T>& msg, const DataType& data)
		{
			// Checks that the type provided is trivially copyable
			static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

			// Cache current size of vector, this will be point we insert the data
			size_t i = msg.body.size();

			// Resize the vector by the size of the data being pushed
			msg.body.resize(msg.body.size() + sizeof(DataType));

			// Physically copy the data into the newly allocated vector space
			std::memcpy(msg.body.data() + i, &data, sizeof(DataType));	

			// Recalculate message size of the header
			msg.header.size = msg.size();

			// Returns the target message so it can be chained
			return msg;
		}

		//Reads the message buffer
		template <typename DataType>
		friend message<T>& operator >> (message<T>& msg, DataType& data)
		{
			// Checks that the type provided is trivially copyable
			static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");
			
			// Cache the location towards the end of the vector where the pulled data starts
			size_t i = msg.body.size() - sizeof(DataType);
		
			// Physically copy the data from the vector into the user variable
			std::memcpy(&data, msg.body.data() + i , sizeof(DataType));

			// Shrink the vector to remove read bytes, and reset and position
			msg.body.resize(i);

			// Recalculate message size of the header
			msg.header.size = msg.size();

			// Returns the target message so it can be chained
			return msg;
		}
	};

	// forward declare the connection
	template<typename T>
	class connection;


	template <typename T>
	struct owned_message
	{
		std::shared_ptr<connection<T>> remote = nullptr;
		message<T> msg;
	
		//friendly string maker
		friend std::ostream& operator<<(std::ostream& os, const owned_message<T>& msg)
		{
			os<<msg.msg;
			return os;
		}
	};
}