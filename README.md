# VNet
A simple and comprehensive RPC library for C++


#Note this project is not finished yet!

#example server client code

```C++
#include "VNet.h"

enum EPacketId : uint16_t
{
	EPI_Null,
	EPI_PublicChat,
	EPI_GimmeHelloWorld,
};

struct Client : public VNetClient
{
	std::vector<std::string> mMesssages;

	void RPCGimmeHelloWorld()
	{
		this->SendPacket(EPI_GimmeHelloWorld, VNetSerilizer(), [](VNetDeSerilizer& respond) {
			std::string str;
			respond && str;
			std::cout << str.c_str();
		});
	}
	void RPCPublicChat(const std::string& message)
	{
		VNetSerilizer ser;

		ser && message;
		this->SendPacket(EPI_PublicChat, ser);
	}
	bool GotPublicChat(VNetDeSerilizer& post, VNetSerilizer& respond, VNetUser* pUser)
	{
		std::string strMessage;
		post && strMessage;
		
		mMesssages.push_back(strMessage);
		std::cout << strMessage.c_str() << '\n';

		return true;
	}
	void RegisterRPCs()
	{
		VREG_RPC(EPI_PublicChat, &Client::GotPublicChat);
	}
};

struct Server : public VNetServer
{
	void RegisterRPCs()
	{
		VREG_RPC(EPI_PublicChat, &Server::RPCPublicChat);
		VREG_RPC(EPI_GimmeHelloWorld, &Server::GimmeHelloWorld);
	}
	bool GimmeHelloWorld(VNetDeSerilizer& post, VNetSerilizer& respond, VNetUser* pUser)
	{
		respond && std::string("hello world.");
		return true;
	}
	bool RPCPublicChat(VNetDeSerilizer& post, VNetSerilizer& respond. VNetUser* pUser)
	{
		//replicate to the users
		this->SendToClients(EPI_PublicChat, post.GetBytes(), post.GetSize());
		return true;
	}
};

```
