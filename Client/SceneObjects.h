#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "../VNet/VNet.h"

enum ESceneObject : uint8_t
{
	ESO_SceneObject,
	ESO_Sprite,
	ESO_SceneTank,
	ESO_PlayerTank,
};

typedef UInt31Comp ObjectNetId;

struct SceneObject
{
	ObjectNetId mId = 0;
	//the session id of the user who can replicate the changes, if 0 anyone can replicate
	uint16_t mOwnerId = 0;
	uint16_t mChangeFlags = 0;

	std::string mName;

	virtual ESceneObject GetType() const
	{
		return ESO_SceneObject;
	}
	virtual void OnRepSer(VNetSerilizer& ser) 
	{
		ser && mChangeFlags;
		if (mChangeFlags & 1)
			ser && mName;
	}
	virtual void OnRepDSer(VNetDeSerilizer& dser, uint16_t changeFlags)
	{
		if (changeFlags & 1)
			dser && mName;
	}
	virtual void DrawProperties()
	{
		char name[256];
		strcpy_s(name, mName.c_str());
		if (ImGui::InputText("Name", name, sizeof(name)))
		{
			mName = name;
			mChangeFlags |= 1;
		}
	}
	virtual void OnSer(VNetSerilizer& ser) const
	{
		ser && mId && mOwnerId && mName;
	}
	virtual void OnDSer(VNetDeSerilizer& dser)
	{
		dser && mId && mOwnerId && mName;
	}
	virtual void Draw(){}
};
//////////////////////////////////////////////////////////////////////////
struct SpriteObject : SceneObject
{
	ImColor mColor = ImColor(255, 255, 255, 255);
	ImVec2 mPos = ImVec2(0, 0);
	ImVec2 mSize = ImVec2(32, 32);
	float mRounding = 0;

	ESceneObject GetType() const override
	{
		return ESO_Sprite;
	}

	void OnRepSer(VNetSerilizer& ser) override
	{
		SceneObject::OnRepSer(ser);
		if (mChangeFlags & 2)
			ser && mColor;
		if (mChangeFlags & 4)
			ser && mPos;
		if (mChangeFlags & 8)
			ser && mSize;
		if (mChangeFlags & 16)
			ser && mRounding;
	}
	void OnRepDSer(VNetDeSerilizer& dser, uint16_t changeFlags) override
	{
		SceneObject::OnRepDSer(dser, changeFlags);
		if (changeFlags & 2)
			dser && mColor;
		if (changeFlags & 4)
			dser && mPos;
		if (changeFlags & 8)
			dser && mSize;
		if (changeFlags & 16)
			dser && mRounding;
	}

	void DrawProperties() override
	{
		SceneObject::DrawProperties();

		if (ImGui::ColorEdit4("Color", (float*)&mColor))
		{
			mChangeFlags |= 2;
		}
		if (ImGui::DragFloat2("Pos", (float*)&mPos))
		{
			mChangeFlags |= 4;
		}
		if (ImGui::DragFloat2("Size", (float*)&mSize))
		{
			mChangeFlags |= 8;
		}
		if (ImGui::DragFloat("Rounding", (float*)&mRounding))
		{
			mChangeFlags |= 16;
		}
	}
	void Draw() override
	{
		auto pDrawer = ImGui::GetOverlayDrawList();
		pDrawer->AddRectFilled(mPos, ImVec2(mPos.x + mSize.x, mPos.y + mSize.y), mColor, mRounding);
	}
	virtual void OnSer(VNetSerilizer& ser) const
	{
		SceneObject::OnSer(ser);
		ser && mColor && mPos && mSize && mRounding;
	}
	virtual void OnDSer(VNetDeSerilizer& dser)
	{
		SceneObject::OnDSer(dser);
		dser && mColor && mPos && mSize && mRounding;
	}
	SpriteObject()
	{
		mName = std::string("S");
	}
};
//////////////////////////////////////////////////////////////////////////
struct SceneTank : SceneObject
{
	ImVec2 mPos = ImVec2(0,0);
	ImColor mColor = ImColor(0,255, 0, 200);
	float mPistolRotation = 0;

	SceneTank()
	{
		mName = "Tank";
	}
	ESceneObject GetType() const override
	{
		return ESO_SceneTank;
	}

	void SetPos(ImVec2 pos)
	{
		if(mPos.x != pos.x || mPos.y != pos.y)
			mChangeFlags |= 2;
		mPos = pos;
		
	}
	void SetColor(ImColor c)
	{
		if(mColor != c)
			mChangeFlags |= 4;
		mColor = c;
		
	}
	void SetPistolRotation(float r)
	{
		if(mPistolRotation != r)
			mChangeFlags |= 8;

		mPistolRotation = r;
	}
	void OnRepSer(VNetSerilizer& ser) override
	{
		SceneObject::OnRepSer(ser);

		if (mChangeFlags & 2)
			ser && mPos;
		if (mChangeFlags & 4)
			ser && mColor;
		if (mChangeFlags & 8)
			ser && mPistolRotation;
	}
	void OnRepDSer(VNetDeSerilizer& dser, uint16_t changeFlags)
	{
		SceneObject::OnRepDSer(dser, changeFlags);

		if (mChangeFlags & 2)
			dser && mPos;
		if (mChangeFlags & 4)
			dser && mColor;
		if (mChangeFlags & 8)
			dser && mPistolRotation;
	}
	void Draw() override
	{
		ImVec2 dir = ImVec2(cos(mPistolRotation), sin(mPistolRotation));
		dir = dir * ImVec2(44, 44);
		auto pDrawer = ImGui::GetOverlayDrawList();
		pDrawer->AddCircleFilled(mPos, 28, mColor, 32);
		pDrawer->AddLine(mPos, mPos + dir, mColor, 8);
	}

	virtual void OnSer(VNetSerilizer& ser) const
	{
		SceneObject::OnSer(ser);
		ser && mPos && mColor && mPistolRotation;
	}
	virtual void OnDSer(VNetDeSerilizer& dser)
	{
		SceneObject::OnDSer(dser);
		dser && mPos && mColor && mPistolRotation;
	}
};

//////////////////////////////////////////////////////////////////////////
struct PlayerTank : SceneTank
{
	ESceneObject GetType() const override
	{
		return ESO_PlayerTank;
	}

	PlayerTank()
	{
		mName = "Player";
	}
	void Draw() override;
};


//////////////////////////////////////////////////////////////////////////
struct Scene
{
	std::vector<SceneObject*> mObjects;
	SceneObject* mSelectedObject = nullptr;
	
	Scene();
	~Scene();

	SceneObject* FindObject(uint32_t id) const;
	bool DeleteObject(uint32_t id);

	void Update();
};
