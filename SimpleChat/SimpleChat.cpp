

#include <iostream>
#include <time.h>

#include "../VNet/VNet.h"

enum EPktId : uint16_t
{
	EPI_Null,
	EPI_PublicChat,
	EPI_GetMessages,
	EPI_GimmeHelloWorld,
	EPI_TestPost,
};

struct Client : public VNetClient
{
	std::vector<std::string> mMesssages;

	void RPCTestPost()
	{
		static unsigned SCounter = 0;

		VNetSerilizer ser;
		unsigned n = SCounter++;
		ser && n;
		SendPacket(EPI_TestPost, ser, [=](VNetDeSerilizer& respond) {
			unsigned readN = 0;
			respond && readN;
			assert(n == readN);
		});
	}
	void RPCGimmeHelloWorld()
	{
		this->SendPacket(EPI_GimmeHelloWorld, VNetSerilizer(), [this](VNetDeSerilizer& respond) {
			
			std::string str;
			respond && str;
			VLOG_MESSAGE("%s", str.c_str());
		});
	}
	void RPCPublicChat(const std::string& cstr)
	{
		VNetSerilizer ser;

		ser && cstr;
		this->SendPacket(EPI_PublicChat, ser);
	}
	bool GotPublicChat(VNetDeSerilizer& post, VNetSerilizer& respond)
	{
		std::string strMessage;
		post && strMessage;
		
		mMesssages.push_back(strMessage);
		std::cout << "User:" << strMessage.c_str() << '\n';

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
		VREG_RPC(EPI_TestPost, &Server::RPCTestPost);
	}
	bool GimmeHelloWorld(VNetDeSerilizer& post, VNetSerilizer& respond)
	{
		respond && std::string("hello world.");
		return true;
	}
	bool RPCPublicChat(VNetDeSerilizer& post, VNetSerilizer& respond)
	{
		//replicate to the users
		this->SendToClients(EPI_PublicChat, post.GetBytes(), post.GetSize());
		return true;
	}
	bool RPCTestPost(VNetDeSerilizer& post, VNetSerilizer& respond)
	{
		unsigned n = 0;
		post && n;
		respond && n;
		return true;
	}
};

Client* gClient = nullptr;
Server* gServer = nullptr;

int main(int argc, char** argv)
{
	bool bServer = false;
	std::cout << "Server ?";
	std::cin >> bServer;
	
	if (bServer)
	{
		gServer = new Server();
		gServer->Listen(5656);
		gServer->RegisterRPCs();
	}
	else
	{
		gClient = new Client();
		gClient->RegisterRPCs();
		gClient->Connect("127.0.0.1", 5656);
	}
	unsigned lastClock = clock();
	bool bOnce = false;

	while (true)
	{
		if (gServer) gServer->Update(1);
		if (gClient)
		{
			if (!bOnce)
			{
				gClient->RPCGimmeHelloWorld();
				bOnce = true;
				
			}
			gClient->Update(1);
			gClient->RPCTestPost();

			if (clock() - lastClock >= 4000)
			{
				lastClock = clock();
				gClient->RPCPublicChat("Hello world");
			}
		}



	}
	
    return 0;
}

