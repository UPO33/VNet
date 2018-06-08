#pragma once

#include "VNet/VSerializer.h"

enum EPacketId : unsigned short
{
	EPI_FlagAutoReplicate = 1 << 15,
	EPI_Flag1 = 1 << 14,
	EPI_Flag2 = 1 << 13,
	EPI_Flag3 = 1 << 12,

	EPI_Null = 0,
	EPI_CheckConnection,
	EPI_Initial = 10,
	//VVV your ids here VVV
	EPI_SendPublicChat,
	EPI_GetPublicChats,
	EPI_SpriteChangeProperties,
	EPI_CreateSprite,
	EPI_SceneReplicate,
	EPI_DeleteObject,
	EPI_CreatePlayer,
	EPI_CreateSceneObjects,
	EPI_FetchCurrentScene,
	//file system
	EPI_GetServerFiles,

	EPI_MaxId = 0xFFF,
};

typedef uint32_t NetFileId;

struct NetFileInfo
{
	enum EState
	{
		EFS_Invalid,
		EFS_Avail, //file is available on server
		EFS_Downloading, //file is being downloaded by a client from server
		EFS_Uploading, //file is being uploaded by a client to server
		EFS_Stopped,
	};

	NetFileId mId = 0;
	std::string mName;
	uint64_t mFileSize = 0;
	EState mState = EFS_Invalid;
};



inline VNetSerilizer& operator <<(VNetSerilizer& ser, const NetFileInfo& f)
{
	ser && f.mId && f.mName && f.mFileSize && f.mState;
	return ser;
}
inline VNetDeSerilizer& operator >> (VNetDeSerilizer& dser, NetFileInfo& f)
{
	dser && f.mId && f.mName && f.mFileSize && f.mState;
	return dser;
}

