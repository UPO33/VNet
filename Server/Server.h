#pragma once

#include "../VNet/VNet.h"
#include "../Shared.h"

struct ServerFile 
{
	NetFileInfo mInfo;
	std::string mRealFileName;
};

inline VNetSerilizer& operator <<(VNetSerilizer& ser, const ServerFile& f)
{
	ser && f.mInfo && f.mRealFileName;
	return ser;
}
inline VNetDeSerilizer& operator >> (VNetDeSerilizer& dser, ServerFile& f)
{
	dser && f.mInfo && f.mRealFileName;
	return dser;
}

class Server : public VNetServer
{
public:
	Server();
	~Server();

	void Update();
	void Draw();

	std::string mLogs;
	bool mLogScrollToBottom = false;
	std::string mPublicChat;
	class Scene* mScene = nullptr;
	
	std::vector<ServerFile> mNetFiles;
	unsigned mObjectIdCounter = 1;

	void RegisterRPCs();
	void DrawLog();
	
	bool HSendPublicChat(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser);
	bool HGetPublicChats(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser);
	bool HCreateSprite(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser);
	bool HSceneReplicate(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser);
	bool HDeleteObject(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser);
	bool HCreatePlayer(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser);
	bool HFetchCurrentScene(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser);
	void ReplicateSceneChanges();
	void SendCurrentScene();

	bool RPCGetServerFiles(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser);
	bool RPCDownloadFile(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser);
	ServerFile* FindFile(NetFileId id);

	void LoadFileInfoTable();
	void SaveFileInfoTable();
};

extern Server* gServer;
