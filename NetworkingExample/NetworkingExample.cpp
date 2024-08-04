#include <iostream>

#ifdef  _WIN32
#define _WINT32_WINNT 0x0A00
#endif //  _WIN32

//to not use BOOST lib
#define ASIO_STANDALONE

//Don't forget to include asio SDK include to project includes
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

//resonably large buffer
std::vector<char> vBuffer(20*1024);

//async
void GrabSomeData(asio::ip::tcp::socket& socket)
{
	socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size()),
		[&](std::error_code ec, std::size_t lenght)
		{	
			if(!ec)
			{
				std::cout << "\n\nRead " << lenght << " bytes\n\n";
				for(int i = 0; i < lenght; i++)
				{
					std::cout << vBuffer[i];
				}

				//to grab bit more data, and bit more again..
				//until no data left, then we wait
				GrabSomeData(socket);
			}				
		}		
	);
}


int main()
{
	asio::error_code ec;

	//creates platform specific context
	asio::io_context context;

	//throws in 'fake' work for the context to do so the thread execution doesn't finish
	asio::io_context::work idleWork(context);

	//letting context run in its own thread
	std::thread thrContext = std::thread([&](){ context.run(); });

	//the adress of somewhere
	//tcp style end-point - ip adress and port
	asio::ip::tcp::endpoint endpoint(asio::ip::make_address("51.38.81.49", ec), 80);

	//networking socket(doorway)
	asio::ip::tcp::socket socket(context);

	//socket to try connect
	socket.connect(endpoint, ec);

	if(!ec)
	{
		std::cout<<"Connected!" <<std::endl;
	}
	else
	{
		std::cout<<"Failed to connect to address!\n" <<ec.message() <<std::endl;
	}

	if(socket.is_open())
	{
		//Async
		GrabSomeData(socket);	

		//some bogus http request
		std::string sRequest = 
			"GET /index.html HTTP/1.1\r\n"
			"HOST: example.com\r\n"
			"Connection: close\r\n\r\n";
	
		//sents the request through socket, sends as much of 'this' data as possible
		socket.write_some(asio::buffer(sRequest.data(),sRequest.size()),ec);			
	}

	//program handles something else, while asio handles data transfer in background
	using namespace std::chrono_literals;
	std::this_thread::sleep_for(20000ms);


	system("pause");
	return 0;
}

//Sync variant, no
/*
int main()
{
asio::error_code ec;

//creates platform specific context
asio::io_context context;

//the adress of somewhere
//tcp style end-point - ip adress and port
asio::ip::tcp::endpoint endpoint(asio::ip::make_address("93.184.216.34", ec), 80);

//networking socket(doorway)
asio::ip::tcp::socket socket(context);

//socket to try connect
socket.connect(endpoint, ec);

if(!ec)
{
std::cout<<"Connected!" <<std::endl;
}
else
{
std::cout<<"Failed to connect to address!\n" <<ec.message() <<std::endl;
}

if(socket.is_open())
{
//some bogus http request
std::string sRequest = 
"GET /index.html HTTP/1.1\r\n"
"HOST: example.com\r\n"
"Connection: close\r\n\r\n";

//sents the request through socket, sends as much of 'this' data as possible
socket.write_some(asio::buffer(sRequest.data(),sRequest.size()),ec);

//no
//using namespace std::chrono_literals;
//std::this_thread::sleep_for(200ms);

//yes, blocking
socket.wait(socket.wait_read);

//if any bytes are send for us to read
//problem: not all bytes will fully be delivered if their size is too much
//problem: we dont know if serevr will respond and with how much data
size_t bytes = socket.available();
if (bytes > 0)
{
std::vector<char> vBuffer(bytes);
socket.read_some(asio::buffer(vBuffer.data(), vBuffer.size()),ec);		

for(auto c :vBuffer)
{
std::cout << c;
}
}
}

return 0;
}
*/