#pragma once

#include "../VNet/VNet.h"
#include "../Shared.h"
#include "imgui.h"

class Client : public VNetClient
{
public:
	Client();
	~Client();

	char mHostAddres[128];
	int mHostPort;

	char mPublicChatInput[512];
	std::string mChats;

	std::string mLogs;
	bool mLogScrollToBottom = false;

	class Scene* mScene = nullptr;
	struct SceneObject* mSelectedObject = nullptr;
	//
	std::vector<NetFileInfo> mNetFiles;

	void Update();
	void Draw();
	void DrawScene();
	void DrawLog();
	void DrawChatRoom();

	void RegisterRPCs();
	void TestSerilizer();
	void SendPublicChat(const char* message);
	bool GotPublicChat(VNetDeSerilizer& request, VNetSerilizer& respond, VNetUser* pUser);
	void GetPublicChatsAsync();

	void DownloadItem(const char* itemName);
	void SpriteChanged(struct SpriteObject* sprite);
	void CreateSprite(const SpriteObject& data);
	bool HCreateSprite(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser);
	void DeleteObject(SceneObject*);
	bool HDeleteObject(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser);

	void FetchCurrentScene();
	bool HCreateSceneObject(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser);

	void CreatePlayer();
	bool HCreatePlayer(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser);

	void ReplicateSceneChanges();
	bool HSceneReplicate(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser);

	void PostConnect();
	void GetServerFilesASync();
	void TestSpritesCreate();

};

extern Client* gClient;

