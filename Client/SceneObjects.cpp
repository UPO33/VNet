#include "SceneObjects.h"
#ifndef SERVER
#include "Client.h"
#endif


Scene::Scene()
{
	//for (size_t i = 0; i < 128; i++)
	//{
	//	mSprites.push_back(new SpriteObject());
	//}
#ifdef SERVER
#endif
}

Scene::~Scene()
{

}



SceneObject* Scene::FindObject(uint32_t id) const
{
	for (SceneObject* pObj : mObjects)
	{
		if (pObj && pObj->mId.mValue == id)
			return pObj;
	}
	return nullptr;
}

bool Scene::DeleteObject(uint32_t id)
{
	for (size_t i = 0; i < mObjects.size(); i++)
	{
		if (mObjects[i] && mObjects[i]->mId.mValue == id)
		{
			delete mObjects[i];
			mObjects.erase(mObjects.begin() + i);
			return true;
		}
	}
	return false;
}



void Scene::Update()
{

}

void PlayerTank::Draw()
{
#ifndef SERVER
	if(mOwnerId == gClient->mSessionId)
	{
		
		if (ImGui::IsKeyDown(' '))
		{

		}

		ImVec2 dir = ImGui::GetMousePos() - mPos;
		SetPistolRotation(atan2(dir.y, dir.x));

		float yVal = ImGui::IsKeyDown('W') ? -1 : ImGui::IsKeyDown('S') ? 1 : 0;
		float xVal = ImGui::IsKeyDown('D') ? 1 : ImGui::IsKeyDown('A') ? -1 : 0;
		ImVec2 offset = ImVec2(xVal, yVal) * 100 * ImGui::GetIO().DeltaTime;
		SetPos(mPos + offset);
	}
#endif

	SceneTank::Draw();
}
