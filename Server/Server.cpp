#include "Server.h"
#include "../Client/SceneObjects.h"
#include <fstream>

//#include "imgui.h"


#pragma comment(lib, "VNet.lib")

Server* gServer;



void ServerInit()
{
	gServer = new Server;
}
void ServerUpdate()
{
	gServer->Update();
	gServer->Draw();
}

void ServerRelease()
{
	delete gServer;
	gServer = nullptr;
}

Server::Server()
{
	VSetLogListener([](const char* log) {
		gServer->mLogs += log;
		gServer->mLogScrollToBottom = true;
	});

	RegisterRPCs();

	this->Listen(5656);
	
	mScene = new Scene();
	
}


Server::~Server()
{
}

void Server::Update()
{
	VNetServer::Update(ImGui::GetIO().DeltaTime);
	ReplicateSceneChanges();
}

void Server::Draw()
{
	DrawLog();
}

void Server::RegisterRPCs()
{
	VREG_RPC(EPI_SendPublicChat, &Server::HSendPublicChat);
	VREG_RPC(EPI_GetPublicChats, &Server::HGetPublicChats);

	VREG_RPC(EPI_CreateSprite, &Server::HCreateSprite);
	VREG_RPC(EPI_SceneReplicate, &Server::HSceneReplicate);
	VREG_RPC(EPI_DeleteObject, &Server::HDeleteObject);
	VREG_RPC(EPI_CreatePlayer, &Server::HCreatePlayer);
	VREG_RPC(EPI_FetchCurrentScene, &Server::HFetchCurrentScene);

	VREG_RPC(EPI_GetServerFiles, &Server::RPCGetServerFiles);
}

void Server::DrawLog()
{
	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
	ImGui::Begin("ServerLog");
	ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::TextUnformatted(mLogs.c_str());
	if (mLogScrollToBottom)
		ImGui::SetScrollHere(1.0f);
	mLogScrollToBottom = false;
	ImGui::EndChild();
	ImGui::End();
}

bool Server::HSendPublicChat(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser)
{
	std::string message;
	params && message;

	if (params.IsValid())
	{
		mPublicChat = message + '\n' + mPublicChat;
		VNetSerilizer replicate;
		replicate && message;
		SendToClients(EPI_SendPublicChat, replicate);
		return true;
	}
	return false;
}

bool Server::HGetPublicChats(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser)
{
	respond && mPublicChat;
	return true;
}



bool Server::HCreateSprite(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser)
{
	auto newSprite = new SpriteObject();
	newSprite->OnDSer(params);
	newSprite->mId = mObjectIdCounter++;
	static unsigned SCounter = 0;
	char name[128];
	sprintf_s(name, "Sprite_%d", SCounter++);
	newSprite->mName = name;
	assert(mScene->FindObject(newSprite->mId.mValue) == nullptr);
	VLOG_SUCCESS("Server NewSprite id %d", newSprite->mId);
	mScene->mObjects.push_back(newSprite);

	VNetSerilizer replicate;
	replicate && ((uint32_t)1) && ESO_Sprite;
	newSprite->OnSer(replicate);
	SendToClients(EPI_CreateSceneObjects, replicate);

	return true;
}
void Server::SendCurrentScene()
{
	
}
bool Server::HSceneReplicate(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser)
{
	uint32_t numObj = 0;
	params && numObj;
	for (uint32_t i = 0; i < numObj; i++)
	{
		ObjectNetId objId = 0;
		params && objId;
		if (SceneObject* pObj = mScene->FindObject(objId.mValue))
		{
			uint16_t changeFlags = 0;
			params && changeFlags;
			pObj->OnRepDSer(params, changeFlags);
			pObj->mChangeFlags |= changeFlags;

			if (!params.IsValid())
				return false;
		}
		else
		{
			return false;
		}
	}
	return true;
}

bool Server::HDeleteObject(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser)
{
	ObjectNetId objId = 0;
	params && objId;
	if (mScene->DeleteObject(objId.mValue))
	{
		VNetSerilizer rep;
		rep && objId;
		SendToClients(EPI_DeleteObject, rep);
	}
	return true;
}

bool Server::HCreatePlayer(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser)
{
	VNetSerilizer rep;

	PlayerTank* tank = new PlayerTank;
	tank->mPos = ImVec2(256, 256);
	tank->mId = mObjectIdCounter++;
	tank->mOwnerId = pUser->mSessionId;
	tank->mColor = ImColor(rand() % 2 == 0 ? 0 : 255, rand() % 2 == 0 ? 0 : 255, rand() % 2 == 0 ? 0 : 255, 200);
	
	tank->OnSer(rep);

	mScene->mObjects.push_back(tank);

	SendToClients(EPI_CreatePlayer, rep);
	return true;
}

bool Server::HFetchCurrentScene(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser)
{
	//collect objects
	std::vector<SceneObject*> objs;
	for (auto pObj : mScene->mObjects)
	{
		if (pObj) objs.push_back(pObj);
	}
	//serialize objects
	VNetSerilizer rep;
	rep && (uint32_t)objs.size();
	for (auto pObj : objs)
	{
		rep && pObj->GetType();
		pObj->OnSer(rep);
	}

	pUser->mSockPkt.SendPacket(EPI_CreateSceneObjects, rep.GetBytes(), rep.GetSeek());
	return true;
}

void Server::ReplicateSceneChanges()
{
	for (size_t iClient = 0; iClient < mClients.size(); iClient++)
	{
		auto pClient = mClients[iClient];

		VNetSerilizer serReplicate;
		std::vector<SceneObject*> dirtyObjects;

		for (SceneObject* pObj : mScene->mObjects)
		{
			if (pObj)
			{
				if (pObj->mChangeFlags && (pObj->mOwnerId == 0 || pObj->mOwnerId != pClient->mSessionId))
					dirtyObjects.push_back(pObj);
			}
		}

		if (dirtyObjects.size() == 0)continue;

		serReplicate && (uint32_t)dirtyObjects.size();

		for (SceneObject* pObj : dirtyObjects)
		{
			serReplicate && pObj->mId;
			pObj->OnRepSer(serReplicate);
		}
		
		pClient->mSockPkt.SendPacket(EPI_SceneReplicate, serReplicate.GetBytes(), serReplicate.GetSeek());
	}
	
	for (SceneObject* pObj : mScene->mObjects)
	{
		if (pObj) pObj->mChangeFlags = 0;
	}
}

bool Server::RPCGetServerFiles(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser)
{
	uint32_t num = mNetFiles.size();
	respond && num;
	for (uint32_t i = 0; i < num; i++)
	{
		respond && mNetFiles[i].mInfo;
	}
	return true;
}

bool Server::RPCDownloadFile(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser)
{
	NetFileId id = 0;
	params && id;
	if(ServerFile* pServerFile = FindFile(id))
	{
		uint64_t fpos = 0, fSize = 0;
		params && fpos && fSize;
	}
	return false;
}

ServerFile* Server::FindFile(NetFileId id)
{
	for (ServerFile& iter : mNetFiles)
	{
		if (iter.mInfo.mId == id)
			return &iter;
	}
	return nullptr;
}

void* VReadFileContents(const char *filename, size_t& outSize)
{
	std::ios_base::openmode mode = (std::ios_base::openmode) std::ios::binary;
	std::ifstream file(filename, mode);
	if (!file) return nullptr;

	file.seekg(0, std::ios::end);
	std::streampos length = file.tellg();
	outSize = (size_t)length;
	file.seekg(0, std::ios::beg);
	auto content = malloc(length);
	file.read((char*)content, length);
	return content;
}

void Server::LoadFileInfoTable()
{
	size_t fsize = 0;
	void* fData = VReadFileContents("files/files.bin", fsize);
	if(fsize && fData)
	{
		VNetDeSerilizer dser(fData, fsize);
		dser && mNetFiles;
	}
}

void Server::SaveFileInfoTable()
{
	VNetSerilizer ser;
	ser && mNetFiles;
	std::ofstream fs("files/files.bin", std::ios::binary);
	fs.write((char*)ser.GetBytes(), ser.GetSeek());
}
