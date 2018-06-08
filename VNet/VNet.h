#pragma once

#include "VSerializer.h"
#include "VBuffers.h"
#include <vector>
#include <map>
#include <functional>
#include <assert.h>





static const size_t VRWBUFFERS_CAPACITY = 1024 * 1024;

using VPacketSizeT = uint32_t;
using VPacketRequestT = uint32_t;

enum EPacketFlag : uint8_t
{
	EPF_PostOnly = 0,
	EPF_Post = 1,
	EPF_Respond = 2,
};

//packet size must be less than this, 22 bits
static const size_t VNET_MAX_PACKET_SIZE = 0x3FffFF;


#pragma pack(push, 1)
struct VPacketHeader
{
	//size of the packet header when its written/read
	static const size_t RWSIZE = 3;
	

	uint32_t mSize = 0;
	EPacketFlag mId = EPF_PostOnly;

	VPacketHeader(){}
	VPacketHeader(uint32_t id, uint32_t size)
	{
		SetSize(size);
		SetId(id);
	}
	uint32_t GetSize() const
	{
		return mSize;
		//return mDWord & VNET_MAX_PACKET_SIZE;
	}
	uint32_t GetId()
	{
		return mId;
		//return (mDWord >> 22) & 3;
	}
	void SetSize(uint32_t size)
	{
		assert(size < VNET_MAX_PACKET_SIZE);
		//mDWord &= (~VNET_MAX_PACKET_SIZE);
		//mDWord |= size;
		mSize = size;
	}
	void SetId(uint32_t id)
	{
		assert(id < 3);
		//mDWord |= (id << 22);
		mId = (EPacketFlag)id;
	}
	//write this header to specified buffer
	void Write(VCircularBuffer& out) const
	{
		uint32_t dword = mSize | ((uint32_t)mId << 22);
		size_t nWrite = out.Write(&dword, RWSIZE);
		assert(nWrite == 3);
	}
	void Write(void* out) const
	{
		uint32_t dword = mSize | ((uint32_t)mId << 22);
		memcpy(out, &dword, 3);
	}
	//read from the specified buffer
	void Read(VCircularBuffer& in)
	{
		uint32_t dword = 0;
		size_t nRead = in.Read(&dword, RWSIZE);
		SetSize(dword & VNET_MAX_PACKET_SIZE);
		SetId((dword >> 22) & 3);
		assert(nRead == 3);
	}
	void Read(const void* in)
	{
		uint32_t dword = 0;
		memcpy(&dword, in, RWSIZE);
		SetSize(dword & VNET_MAX_PACKET_SIZE);
		SetId((dword >> 22) & 3);
	}
};
#pragma pack(pop)

static const size_t S = sizeof(VPacketHeader);


struct VPacket
{
	VPacketHeader mHeader;
	//the rest is data ..

	size_t GetSize() const { return mHeader.GetSize(); }
	void* GetData() { return &mHeader + 1; }
};
struct VNetUser;
struct VSocketPkt;
class VNetBase;
using FPHandleRPC = bool (VNetBase::*)(VNetDeSerilizer& request, VNetSerilizer& respond, VNetUser* pSender);

struct VRPCHandle
{
	uint16_t mId;
	const char* mIdString;
	void* mProcRaw;
	
	VRPCHandle() : mId(0), mIdString(nullptr), mProcRaw(0) {}
	FPHandleRPC GetProc() const;

	template<typename T> VRPCHandle(uint16_t id, const char* idStr, bool(T::*proc)(VNetDeSerilizer&, VNetSerilizer&, VNetUser*))
	{
		mId = id;
		mIdString = idStr;
		union
		{
			bool(T::* lproc)(VNetDeSerilizer&, VNetSerilizer&, VNetUser*);
			void* raw;
		};
		lproc = proc;
		mProcRaw = raw;
	}
};

#define VREG_RPC(id, func) mRPCHandles[id] = VRPCHandle( id, #id, func);

//////////////////////////////////////////////////////////////////////////
class VNetBase
{
public:
	VPacket* GetEmptyPacket();
	VPacket* AllocatePacket(size_t size);
	void FreePacket(VPacket* pkt);

	void HandlePost(VPacket* pPacket, VSocketPkt* pOwner, bool bPostOnly);

	std::map<uint16_t, VRPCHandle> mRPCHandles;
	
	VNetBase(){}
	virtual ~VNetBase() {}
	
};

using RPCRespondProc = std::function<void(VNetDeSerilizer&)>;

//////////////////////////////////////////////////////////////////////////
struct VSocketPkt
{
	VNetBase* mNetBase = nullptr;
	VNetUser* mNetUser = nullptr;
	void* mSocket = nullptr;
	VCircularBuffer mReadBuffer;
	//VCircularBuffer mWriteBuffer;
	VPacketHeader mCurReadPacketHdr;
	bool mHasCurReadPacketHdr = false;
	std::vector<VPacket*> mRespondPackets;
	std::vector<VPacket*> mPostPackets;
	std::vector<VPacket*> mPostOnlyPackets;
	std::vector<RPCRespondProc> mPostCallbacks;
	unsigned mLastClock = 0;
	
	std::vector<VPacket*> mWritePackets;

	VSocketPkt(VNetBase*);
	~VSocketPkt();
	
	void Update();
	bool SendPacket(uint16_t PacketId, const void* pData, size_t size);
	bool SendPacket(uint16_t packetId, const void* pData, size_t size, RPCRespondProc respond);
	bool SendPacket(uint32_t headerFlag, uint16_t PacketId, const void* pData, size_t size);
	bool SendRespondPacket(const void* pData, size_t size);
	bool IsOnline();
	void Disconnect();
	void WaitForRespond();
};

//////////////////////////////////////////////////////////////////////////
struct VNetUser
{
	VSocketPkt mSockPkt;
	uint32_t mSessionId = 0;

	VNetUser(VNetBase* pBase, void* pSocket) : mSockPkt(pBase)
	{
		mSockPkt.mSocket = pSocket;
		mSockPkt.mNetUser = this;
	}
	virtual ~VNetUser() {}
};

//////////////////////////////////////////////////////////////////////////
class VNetServer : public VNetBase
{
public:
	VNetServer();
	~VNetServer();

	bool Listen(unsigned short port);
	void Update(float delta);
	void ShutDown();



	void SendToClients(uint16_t packetId, const void* pData, size_t dataSize);
	void SendToClients(uint16_t packetId, const VNetSerilizer& data);

	std::vector<VNetUser*> mClients;
private:
	void HandleAccept();

	void* mIoCtx = nullptr;
	void* mAcceptor = nullptr;
	uint32_t mSessionIdCounter = 1;
	
};



//////////////////////////////////////////////////////////////////////////
class VNetClient : public VNetBase
{
public:
	VNetClient();
	~VNetClient();

	bool Connect(const char* ipAddress, unsigned short port);
	bool IsConnected() const;
	void Update(float delta);
	//send a packet without respond
	bool SendPacket(uint16_t id, const VNetSerilizer& post);
	//send a packet with respond
	bool SendPacket(uint16_t id, const VNetSerilizer& post, RPCRespondProc respond);
	void WaitForRespond();

	virtual void OnDisconnect();

	VSocketPkt mSockPkt;
	void* mIoContext = nullptr;
	uint32_t mSessionId = 0;
};
