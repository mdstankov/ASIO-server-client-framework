//Add 'NetCommon' to Build Dependancies
//Add path to \NetCommon in Include Directories

#include <iostream>
#include <net_headers.h>

//message types, type specific to net message type
enum class CustomMsgTypes : uint32_t
{
	Fire,
	Move
};

int main()
{
	net::message<CustomMsgTypes> msg;
	msg.header.id = CustomMsgTypes::Fire;

	//test
	int a = 1;
	bool b = true;
	float c = 3.14f;

	struct
	{
		float x;
		float y;	
	} d[5];

	//push to mesage
	msg << a << b << c << d;

	//change values
	a = 0;
	b = false;
	c = 0.0f;

	//read values back from message
	msg >> d >> c >> b >>a;


	return 0;
}
