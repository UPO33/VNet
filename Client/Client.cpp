#include "Client.h"
#include "imgui.h"
#include "SceneObjects.h"

#pragma comment(lib, "VNet.lib")

Client* gClient = nullptr;

unsigned gPktIter = 0;

void ClientInit()
{
	gClient = new Client();
}
void ClientUpdate()
{
	gClient->Update();
	gClient->Draw();
}
void ClientRelease()
{
	delete gClient;
	gClient = nullptr;
}

Client::Client()
{
	TestSerilizer();

	mLogs.reserve(100 * 1024);
	strcpy_s(mHostAddres, "127.0.0.1");
	mHostPort = 5656;

	strcpy_s(mPublicChatInput, "");
	
	VSetLogListener([](const char* log) {
		gClient->mLogs += log;
		gClient->mLogScrollToBottom = true;
	});

	RegisterRPCs();

	mScene = new class Scene();
}


Client::~Client()
{
}

void Client::Update()
{
	VNetClient::Update(ImGui::GetIO().DeltaTime);

	if (ImGui::IsKeyPressed('N'))
	{
		SpriteObject spr;
		spr.mOwnerId = mSessionId;
		spr.mPos = ImGui::GetMousePos();
		spr.mColor = ImColor(255, 255, 0, 100);
		CreateSprite(spr);
	}

	ReplicateSceneChanges();
}

void Client::Draw()
{
	if(!this->IsConnected())
	{
		if (ImGui::Begin("Client"))
		{
			if (ImGui::Button("Connect"))
			{
				if (this->Connect(mHostAddres, (unsigned short)mHostPort))
				{
					PostConnect();
				}
			}
			ImGui::SameLine();
			ImGui::InputText("Host", mHostAddres, sizeof(mHostAddres));
			ImGui::SameLine();
			ImGui::InputInt("", &mHostPort, 1);

			ImGui::End();
		}
	}
	else
	{
		DrawScene();
		DrawChatRoom();
	}
	DrawLog();
}

void Client::DrawScene()
{
	if (ImGui::Begin("Scene"))
	{
		ImGui::Text("Press N to Spawn Sprite");
		if (ImGui::Button("TestSpritesCreate"))
		{
			TestSpritesCreate();
		}
		ImGui::SameLine();
		if (ImGui::Button("DeleteAll"))
		{
			for (size_t i = 0; i < mScene->mObjects.size(); i++)
				DeleteObject(mScene->mObjects[i]);

			mSelectedObject = nullptr;
		}

		ImGui::Columns(2);
		{
			for (size_t i = 0; i < mScene->mObjects.size(); i++)
			{
				ImGui::PushID(i);
				if (ImGui::Button("D"))
				{
					DeleteObject(mScene->mObjects[i]);
				}
				ImGui::SameLine();
				if (ImGui::Selectable(mScene->mObjects[i]->mName.c_str(), mScene->mObjects[i] == mSelectedObject))
				{
					mSelectedObject = mScene->mObjects[i];
				}
				ImGui::PopID();
			}
		}
		ImGui::NextColumn();
		if (auto pSelectedSprite = mSelectedObject)
		{
			pSelectedSprite->DrawProperties();
		}
		ImGui::NextColumn();
		ImGui::Columns(1);

	}
	ImGui::End();

	auto pDrawer = ImGui::GetOverlayDrawList();
	if (pDrawer)
	{
		for (const auto& pObj : mScene->mObjects)
		{
			pObj->Draw();
		}
	}
}

void Client::DrawLog()
{
	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
	ImGui::Begin("Log");
	ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::TextUnformatted(mLogs.c_str());
	if (mLogScrollToBottom)
		ImGui::SetScrollHere(1.0f);
	mLogScrollToBottom = false;
	ImGui::EndChild();
	ImGui::End();
}

void Client::DrawChatRoom()
{
	if (ImGui::Begin("PublicChat"))
	{
		if (ImGui::InputText("Chat", mPublicChatInput, sizeof(mPublicChatInput), ImGuiInputTextFlags_EnterReturnsTrue))
		{
			this->SendPublicChat(mPublicChatInput);

			mPublicChatInput[0] = 0;
			ImGui::SetKeyboardFocusHere(-1);
		}

		ImGui::TextUnformatted(mChats.c_str());
	}
	ImGui::End();
}

void Client::RegisterRPCs()
{
	VREG_RPC(EPI_SendPublicChat, &Client::GotPublicChat);
	VREG_RPC(EPI_CreateSprite, &Client::HCreateSprite);
	VREG_RPC(EPI_SceneReplicate, &Client::HSceneReplicate);
	VREG_RPC(EPI_DeleteObject, &Client::HDeleteObject);
	VREG_RPC(EPI_CreatePlayer, &Client::HCreatePlayer);
	VREG_RPC(EPI_CreateSceneObjects, &Client::HCreateSceneObject);

	//VRPCHandle(EPI_RPC_SendPublicChat, "", &Client::GotPublicChat);
}

void Client::TestSerilizer()
{
	{
		VNetSerilizer ser;
		ser && UInt31Comp(4) && UInt31Comp(8) && UInt31Comp(0x8000000);
		UInt31Comp a, b, c;
		VNetDeSerilizer dser(ser.GetBytes(), ser.GetSeek());
		dser && a && b && c;
		assert(a.mValue == 4);
		assert(b.mValue == 8);
		assert(c.mValue == 0x8000000);

	}
	VNetSerilizer ser;

	int a = 0;
	int b = 123;
	int c = 333333333;
	bool d = true;
	std::string hello = "hello";
	std::string world = "world";
	std::vector<int> vec0 = { 0,1,2,3,4,5 };
	std::vector<std::string> vec1 = { "asd", "qwe", "zxc"};

	
	ser  && a && b && c && d && hello && world && vec0 && vec1;
	void* pSered = malloc(ser.GetSeek());
	memcpy(pSered, ser.GetBytes(), ser.GetSeek());
	VNetDeSerilizer dser(pSered, ser.GetSeek());

	dser && a && b && c && d && hello && world && vec0 && vec1;
	
	VNetSerilizer reser;
	reser && a && b && c && d && hello && world && vec0 && vec1;
	
	assert(reser.GetSeek() == dser.GetSize());
	assert(memcmp(dser.GetBytes(), reser.GetBytes(), reser.GetSeek()) == 0);

}

void Client::SendPublicChat(const char* message)
{
	VNetSerilizerStack<512> ser;
	ser && message;
	
	SendPacket(EPI_SendPublicChat, ser);
}

bool Client::GotPublicChat(VNetDeSerilizer& request, VNetSerilizer& respond, VNetUser* pUser)
{
	std::string message;
	request && message;
	if (request.IsValid())
	{
		mChats = message + "\n" + mChats;
	}
	return true;
}

void Client::GetPublicChatsAsync()
{
	SendPacket(EPI_GetPublicChats, VNetSerilizer(), [this](VNetDeSerilizer& respond) {
		respond && mChats;
	});
}

void Client::DownloadItem(const char* itemName)
{
#if 0
	VNetSerilizer post;
	post && itemName;
	RPCSend(post, [this](VNetDeSerilizer& params) {
		std::vector<char> data;
		params && data;
	});
#endif
}

void Client::SpriteChanged(SpriteObject* sprite)
{
	VNetSerilizer post;
	post && *sprite;

	SendPacket(EPI_SpriteChangeProperties, post);
}


void Client::CreateSprite(const SpriteObject& spr)
{
	VNetSerilizer post;
	spr.OnSer(post);
	SendPacket(EPI_CreateSprite, post);
}

bool Client::HCreateSprite(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser)
{
	SpriteObject spr;
	spr.OnDSer(params);
	if (!params.IsValid())
		return false;
	
	mScene->mObjects.push_back(new SpriteObject(spr));
	return true;
}

void Client::DeleteObject(SceneObject* pObj)
{
	if (pObj)
	{
		VNetSerilizer ser;
		ser && pObj->mId;
		SendPacket(EPI_DeleteObject, ser);
	}
}

bool Client::HDeleteObject(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser)
{
	UInt31Comp objId;
	params && objId;
	
	mSelectedObject = nullptr;
	mScene->DeleteObject(objId.mValue);
	return true;
}

void Client::FetchCurrentScene()
{
	SendPacket(EPI_FetchCurrentScene, VNetSerilizer());
}

//////////////////////////////////////////////////////////////////////////
bool Client::HCreateSceneObject(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser)
{
	uint32_t count = 0;
	params && count;

	for (size_t i = 0; i < count; i++)
	{
		ESceneObject type = ESO_SceneObject;
		params && type;
		SceneObject* pObj = nullptr;
		switch (type)
		{
		case ESO_SceneObject: pObj = new SceneObject();
			break;
		case ESO_Sprite: pObj = new SpriteObject();
			break;
		case ESO_SceneTank: pObj = new SceneTank();
			break;
		case ESO_PlayerTank: pObj = new PlayerTank();
			break;
		}
		
		pObj->OnDSer(params);
		if (!params.IsValid())
		{
			delete pObj;
			return true;
		}
		if (mScene->FindObject(pObj->mId.mValue))
		{
			delete pObj;
		}
		else
		{
			mScene->mObjects.push_back(pObj);
		}
	}
	return true;
}

void Client::CreatePlayer()
{
	SendPacket(EPI_CreatePlayer, VNetSerilizer());
}

bool Client::HCreatePlayer(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser)
{
	PlayerTank* pPlayer = new PlayerTank();

	pPlayer->OnDSer(params);
	mScene->mObjects.push_back(pPlayer);

	return true;
}

void Client::ReplicateSceneChanges()
{
	VNetSerilizer serReplicate;
	std::vector<SceneObject*> dirtyObjects;

	for (SceneObject* pObj : mScene->mObjects)
	{
		if (pObj)
		{
			if (pObj->mChangeFlags && (pObj->mOwnerId == 0 || pObj->mOwnerId == mSessionId))
				dirtyObjects.push_back(pObj);
		}
	}
	
	if (dirtyObjects.size() == 0) return;

	serReplicate && (uint32_t)dirtyObjects.size();

	for (SceneObject* pObj : dirtyObjects)
	{
		serReplicate && pObj->mId;
		pObj->OnRepSer(serReplicate);
		pObj->mChangeFlags = 0;
	}

	VLOG_SUCCESS("RepScenne");
	SendPacket(EPI_SceneReplicate, serReplicate);
}

bool Client::HSceneReplicate(VNetDeSerilizer& params, VNetSerilizer& respond, VNetUser* pUser)
{
	uint32_t numObj = 0;
	params && numObj;
	for (uint32_t i = 0; i < numObj; i++)
	{
		ObjectNetId objId = 0;
		params && objId;
		if (SceneObject* pObj = mScene->FindObject(objId.mValue))
		{
			//if(pObj->mOwnerId == 0 || pObj->mOwnerId != mSessionId)
			{
				uint16_t changeFlags = 0;
				params && changeFlags;
				pObj->OnRepDSer(params, changeFlags);
				if (!params.IsValid())
					return false;
			}
		}
		else
		{
			return false;
		}
	}
	return true;
}

void Client::PostConnect()
{
	this->GetPublicChatsAsync();
	this->GetServerFilesASync();
	this->CreatePlayer();
	this->FetchCurrentScene();
}

void Client::GetServerFilesASync()
{
	SendPacket(EPI_GetServerFiles, VNetSerilizer(), [this](VNetDeSerilizer& respond) {
		mNetFiles.clear();
		respond && mNetFiles;
	});
}

void Client::TestSpritesCreate()
{
	int ggg = rand() % 255;
	int bbb = 0;
	ImVec2 pos = ImVec2(rand() % 256, rand() % 256) + ImVec2(64, 64);
	for (size_t i = 0; i < 128; i++)
	{
		SpriteObject spr;
		spr.mPos = pos;
		spr.mPos.x += i * 2;
		spr.mPos.y += i * 2;
		spr.mColor = ImColor(i, ggg, bbb, 150);
		spr.mSize.x += 0.25f * i;
		CreateSprite(spr);
	}
}

