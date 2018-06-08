#include "VNet.h"

#include <boost/asio.hpp>

using namespace boost::asio;
using namespace boost::asio::ip;




// bool VNetClient::SendPacket(const void* pData, size_t size, std::function<void(VPacket&)> respond)
// {
// 	VPacketHeader hdr;
// 	hdr.SetId(size);
// 	hdr.SetId(EPF_Post);
// 
// 	hdr.Write(mWriteBuffer);
// 	size_t nWrite = mWriteBuffer.write(pData, size);
// 	assert(nWrite == size);
// 	//currently we just assert and don't handle overflow of buffer
// 
// 	mPosts.push_back(PostData{ respond });
// 	
// 	return true;
// }

// bool VNetClient::SendPacket(const void* pData, size_t size)
// {
// 	VPacketHeader hdr;
// 	hdr.SetId(size);
// 	hdr.SetId(EPF_PostOnly);
// 
// 	hdr.Write(mWriteBuffer);
// 	size_t nWrite = mWriteBuffer.write(pData, size);
// 	assert(nWrite == size);
// 	//currently we just assert and don't handle overflow of buffer
// 
// 	return true;
// }

// bool VNetClient::OnRecvPacket(void* pData, size_t size)
// {
// 	return false;
// }

VNetClient::VNetClient() : 
	mIoContext(new boost::asio::io_context()),
	mSockPkt(this)
{
// 
// 	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), 3434);
// 	mAcceptor = new tcp::acceptor(mIOCtx, endpoint);
// 
// 	mAcceptThread = std::thread([this]() {
// 		this->AcceptHandle();
// 	});

}

VNetClient::~VNetClient()
{
	delete mIoContext;
	mIoContext = nullptr;
}

bool VNetClient::Connect(const char* ipAddress, unsigned short port)
{
	if (mSockPkt.mSocket)return false;

	mSockPkt.mPostCallbacks.clear();
	mSockPkt.mPostOnlyPackets.clear();
	mSockPkt.mPostPackets.clear();
	mSockPkt.mRespondPackets.clear();
	mSockPkt.mHasCurReadPacketHdr = false;
	mSockPkt.mReadBuffer.Reset();
	//mSockPkt.mWriteBuffer.Reset();

	auto pIoCtx = (boost::asio::io_context*)mIoContext;
	auto pSocket = new tcp::socket(*pIoCtx);
	tcp::endpoint endpoint(ip::address::from_string(ipAddress), port);
	boost::system::error_code ec;
	pSocket->connect(endpoint, ec);
	if (ec)
	{
		VLOG_ERROR("faile to connect. ip:%s port:%d", ec.message().c_str(), (unsigned)port);
		return false;
	}
	else
	{
		VLOG_SUCCESS("socket connected.");

		struct InitialData
		{
			uint32_t mSessId = 0;
		};

		InitialData indt;
		size_t nRead = 0;

		do
		{
			nRead += pSocket->read_some(boost::asio::buffer(&indt, sizeof(indt)));
		} while (nRead != sizeof(indt));

		mSessionId = indt.mSessId;
	}

	mSockPkt.mSocket = pSocket;

	return true;
}

bool VNetClient::IsConnected() const
{
	return mSockPkt.mSocket != nullptr;
}

void VNetClient::Update(float delta)
{
	mSockPkt.Update();

	auto pIoCtx = (boost::asio::io_context*)mIoContext;

	pIoCtx->poll();
}

bool VNetClient::SendPacket(uint16_t id, const VNetSerilizer& post)
{
	return mSockPkt.SendPacket(id, post.GetBytes(), post.GetSeek());
}

bool VNetClient::SendPacket(uint16_t id, const VNetSerilizer& post, RPCRespondProc respond)
{
	return mSockPkt.SendPacket(id, post.GetBytes(), post.GetSeek(), respond);
}

void VNetClient::WaitForRespond()
{
	mSockPkt.WaitForRespond();
}

void VNetClient::OnDisconnect()
{
	
}

VPacket* VNetBase::GetEmptyPacket()
{
	return AllocatePacket(0);
}

VPacket* VNetBase::AllocatePacket(size_t size)
{
	 auto pkt = (VPacket*)malloc(size + sizeof(VPacket));
	 pkt->mHeader.SetSize(size);
	 return pkt;
}

void VNetBase::FreePacket(VPacket* pkt)
{
	free(pkt);
}

//void VNetBase::HandlePostOnly(VPacket* pkt, VSocketPkt* owner)
//{
//	VNetDeSerilizer params(pkt->GetData(), pkt->GetSize());
//	uint16_t id = 0;
//	params && id;
//	if (!params.IsValid())
//	{
//		VLOG_ERROR("cant' read packet id");
//		return;
//	}
//
//	if (id == 0) //is packet id unknown?
//	{
//		VLOG_ERROR("invalid packet id.");
//		return;
//	}
//
//	auto iter = mRPCHandles.find(id);
//	if (iter != mRPCHandles.end())
//	{
//		VLOG_MESSAGE("handling RPC %s", iter->second.mIdString);
//		auto proc = iter->second.GetProc();
//		assert(proc);
//		VNetSerilizer respond; //this is post only respond is not used
//		bool bRet = (this->*proc)(params, respond);
//		if (bRet)
//		{
//			VLOG_MESSAGE("RPC %s handled succesfully", iter->second.mIdString);
//		}
//	}
//	else
//	{
//		VLOG_MESSAGE("RPC not found. id:%d", (unsigned)id);
//	}
//}
//////////////////////////////////////////////////////////////////////////
void VNetBase::HandlePost(VPacket* pPacket, VSocketPkt* pOwner, bool bPostOnly)
{
	VNetDeSerilizer params(pPacket->GetData(), pPacket->GetSize());
	uint16_t id = 0;
	params && id;
	if (!params.IsValid())
	{
		VLOG_ERROR("cant' read packet id");
		return;
	}

	if (id == 0)
	{
		VLOG_ERROR("invalid packet id.");
		return;
	}

	auto iter = mRPCHandles.find(id);
	if (iter != mRPCHandles.end())
	{
		VLOG_MESSAGE("handling RPC %s", iter->second.mIdString);
		auto proc = iter->second.GetProc();
		assert(proc);
		VNetSerilizer respond;
		bool bRet = (this->*proc)(params, respond, pOwner->mNetUser);
		if (!bPostOnly) //write respond?
		{
			pOwner->SendRespondPacket(respond.GetBytes(), respond.GetSeek());
		}
	}
	else
	{
		VLOG_MESSAGE("RPC not found. id:%d", (unsigned)id);
	}
}

VNetServer::VNetServer()
{
	mIoCtx = new boost::asio::io_context();
}

VNetServer::~VNetServer()
{
	ShutDown();

	if (auto pIoCtx = (boost::asio::io_context*)mIoCtx)
	{
		pIoCtx->run();
		delete pIoCtx;
		mIoCtx = nullptr;
	}
}
void accept_handler(const boost::system::error_code& error)
{
	if (!error)
	{
		// Accept succeeded.
	}
}
bool VNetServer::Listen(unsigned short port)
{
	auto pIoCtx = (boost::asio::io_context*)mIoCtx;
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
	auto pAcceptor = new tcp::acceptor(*pIoCtx, endpoint);
	mAcceptor = pAcceptor;

	//std::thread([=]() {
	//	boost::system::error_code err;
	//	tcp::socket connection = pAcceptor->accept(err);
	//	if (!err)
	//	{
	//		new tcp::socket(std::move(connection));
	//	}
	//});

	HandleAccept();

	return true;
}

void VNetServer::Update(float delta)
{
	auto pIoCtx = (boost::asio::io_context*)mIoCtx;
	
	pIoCtx->poll();

	for (size_t i = 0; i < mClients.size(); i++)
	{
		auto client = mClients[i];
		client->mSockPkt.Update();
	}
}

void VNetServer::ShutDown()
{
	
	if (auto pIoCtx = (boost::asio::io_context*)mIoCtx)
	{
		pIoCtx->poll();
		pIoCtx->stop();
	}

	if (auto pAcceptor = (tcp::acceptor*)mAcceptor)
	{
		pAcceptor->close();
		delete pAcceptor;
		mAcceptor = nullptr;
	}


	for (size_t i = 0; i < mClients.size(); i++)
	{
		if (mClients[i]) delete mClients[i];
	}
	mClients.clear();

	if (auto pIoCtx = (boost::asio::io_context*)mIoCtx)
		delete pIoCtx;
	mIoCtx = nullptr;
}

void VNetServer::SendToClients(uint16_t packetId, const void* pData, size_t dataSize)
{
	for (size_t i = 0; i < mClients.size(); i++)
	{
		if (mClients[i]->mSockPkt.IsOnline())
		{
			mClients[i]->mSockPkt.SendPacket(packetId, pData, dataSize);
		}
	}
}

void VNetServer::SendToClients(uint16_t packetId, const VNetSerilizer& data)
{
	SendToClients(packetId, data.GetBytes(), data.GetSeek());
}

void VNetServer::HandleAccept()
{
	auto pIoCtx = (boost::asio::io_context*)mIoCtx;
	auto pAcceptor = (tcp::acceptor*)mAcceptor;
	auto pPeerSocket = new tcp::socket(*pIoCtx);
	static tcp::endpoint ep;

 	pAcceptor->async_accept(*pPeerSocket, ep, [this, pPeerSocket](const boost::system::error_code& error) {
 		if(!error)
 		{
 			VLOG_MESSAGE("new connection acccepted.");
			auto nc = new VNetUser(this, pPeerSocket);
			mClients.push_back(nc);
			nc->mSessionId = mSessionIdCounter++;

			
			struct InitialData
			{
				uint32_t mSessId = 0;
			};
			InitialData indt;
			indt.mSessId = nc->mSessionId;

			pPeerSocket->write_some(boost::asio::buffer(&indt, sizeof(indt)));
 		}
 		else
 		{
 			VLOG_ERROR("acept failed %s", error.message().c_str());
 		}
 		HandleAccept();
 	});
}
//////////////////////////////////////////////////////////////////////////
bool VSocketPkt::SendPacket(uint32_t headerFlag, uint16_t packetId, const void* pData, size_t size)
{
#if 0
	if (mSocket == nullptr)
	{
		VLOG_ERROR("no connection!");
		return false;
	}
	size_t writeSizeFull = VPacketHeader::RWSIZE + size + sizeof(packetId);
	if (mWriteBuffer.WriteAvail() < writeSizeFull)
	{
		VLOG_ERROR("Buffer is full! can't write the packet.");
		return false;
	}

	VPacketHeader(headerFlag, size + sizeof(packetId)).Write(mWriteBuffer); //write header
	mWriteBuffer.Write(&packetId, sizeof(packetId)); // write pkt id
	size_t nWrite = mWriteBuffer.Write(pData, size); //write data
	assert(nWrite == size);

	return true;
#else
	if (mSocket == nullptr)
	{
		VLOG_ERROR("no connection!");
		return false;
	}
	
	size_t pktSize = size + sizeof(packetId);
	size_t writeSizeFull = VPacketHeader::RWSIZE + pktSize;
	auto pPkt = mNetBase->AllocatePacket(writeSizeFull);
	uint8_t* pDst = (uint8_t*)pPkt->GetData();
	VPacketHeader(headerFlag, pktSize).Write(pDst); //header
	pDst += VPacketHeader::RWSIZE;
	memcpy(pDst, &packetId, sizeof(packetId)); //pkt id
	pDst += sizeof(packetId);
	memcpy(pDst, pData, size); //data

	mWritePackets.push_back(pPkt);
	return true;

#endif 
}
//////////////////////////////////////////////////////////////////////////
bool VSocketPkt::SendPacket(uint16_t packetId, const void* pData, size_t size, RPCRespondProc respond)
{
	bool bOk = SendPacket(EPF_Post, packetId, pData, size);
	if(bOk) mPostCallbacks.push_back(respond);
	return bOk;
}

bool VSocketPkt::SendPacket(uint16_t PacketId, const void* pData, size_t size)
{
	return SendPacket(EPF_PostOnly, PacketId, pData, size);
}





bool VSocketPkt::SendRespondPacket(const void* pData, size_t size)
{
#if 0
	if (mSocket == nullptr)
	{
		VLOG_ERROR("no connection!");
		return false;
	}
	size_t writeSizeFull = VPacketHeader::RWSIZE + size;
	if (mWriteBuffer.WriteAvail() < writeSizeFull)
	{
		VLOG_ERROR("Buffer is full! can't write the packet.");
		return false;
	}

	VPacketHeader(EPF_Respond, size).Write(mWriteBuffer); //write header
	mWriteBuffer.Write(pData, size); //write data

	return true;
#endif

	if (mSocket == nullptr)
	{
		VLOG_ERROR("no connection!");
		return false;
	}

	size_t pktSize = size;
	size_t writeSizeFull = VPacketHeader::RWSIZE + pktSize;
	auto pPkt = mNetBase->AllocatePacket(writeSizeFull);
	uint8_t* pDst = (uint8_t*)pPkt->GetData();
	VPacketHeader(EPF_Respond, pktSize).Write(pDst); //header
	pDst += VPacketHeader::RWSIZE;
	memcpy(pDst, pData, size); //data

	mWritePackets.push_back(pPkt);
	return true;

}

bool VSocketPkt::IsOnline()
{
	return mSocket != nullptr;
}

void VSocketPkt::Disconnect()
{
	if(auto pSocket = (tcp::socket*)mSocket)
	{
		delete pSocket;
		mSocket = nullptr;
	}
}

void VSocketPkt::WaitForRespond()
{
	while (true)
	{
		if (mPostCallbacks.size() == 0)
			return;

		Update();
	}
}

VSocketPkt::VSocketPkt(VNetBase* pBase) : mNetBase(pBase), mReadBuffer(VRWBUFFERS_CAPACITY)
{
	mLastClock = clock();
}

VSocketPkt::~VSocketPkt()
{
	if (auto pSocket = (tcp::socket*)mSocket)
	{
		delete pSocket;
		mSocket = nullptr;
	}

}

void VWritePkt(uint8_t* pDst, uint32_t flag, uint16_t pktId, const void* pData, uint32_t dataSize)
{
	assert(flag < 3);
	assert(dataSize < VNET_MAX_PACKET_SIZE);

	uint32_t n = dataSize | (flag << 22);
	memcpy(pDst, &n, 3);
	pDst += 3;
	memcpy(pDst, &pktId, sizeof(pktId));
	pDst += sizeof(pktId);
	memcpy(pDst, pData, dataSize);
}

void VSocketPkt::Update()
{
	auto pBase = mNetBase;
	auto pSocket = (tcp::socket*)mSocket;
	
	if (pSocket)
	{
		if (size_t avail = pSocket->available())
		{
			auto pPkt = mNetBase->AllocatePacket(avail);
			pSocket->async_receive(boost::asio::buffer(pPkt->GetData(), pPkt->GetSize()), [=](const boost::system::error_code& error, std::size_t bytes_transferred) {
				if ((boost::asio::error::eof == error) || (boost::asio::error::connection_reset == error))
				{
					Disconnect();
				}
				else
				{
					size_t nWrite = this->mReadBuffer.Write(pPkt->GetData(), bytes_transferred);
					assert(nWrite == bytes_transferred);
				}
				mNetBase->FreePacket(pPkt);
			});

			//size_t recvAvail = std::min(mTmpReadBuffSize, mReadBuffer.WriteAvail());
			//pSocket->async_receive(boost::asio::buffer(mTmpReadBuff, recvAvail), [this](const boost::system::error_code& error, std::size_t bytes_transferred) {
			//	//VLOG_MESSAGE("ASync Reacv Handled. nByte %d", bytes_transferred);
			//	if ((boost::asio::error::eof == error) || (boost::asio::error::connection_reset == error))
			//	{
			//		Disconnect();
			//	}
			//	else
			//	{
			//		size_t nWrite = this->mReadBuffer.Write(mTmpReadBuff, bytes_transferred);
			//		assert(nWrite == bytes_transferred);
			//	}
			//
			//});
		}
		/*
		if (this->mWriteBuffer.ReadAvail())
		{

			void* pMems[2];
			size_t sizes[2];
			size_t nRead = mWriteBuffer.ReadPt(mWriteBuffer.ReadAvail(), pMems, sizes);
			mWriteBuffer.ReadIgnore(nRead);

			for (size_t i = 0; i < 2; i++)
			{
				if(sizes[i] == 0)
					continue;

				pSocket->async_send(boost::asio::buffer(pMems[i], sizes[i]), [=](const boost::system::error_code& error, std::size_t bytes_transferred) {
					//VLOG_MESSAGE("ASync Send Handled. nByte %d", bytes_transferred);
					//if (boost::asio::error::eof == error || boost::asio::error::connection_reset == error 
					//	|| error == boost::asio::error::connection_aborted)
					if (error)
					{
						VLOG_ERROR("%s", error.message().c_str());
						Disconnect();
					}
					else
					{
						assert(sizes[i] == bytes_transferred);
						//mWriteBuffer.ReadIgnore(sizes[i]);
					}
				});
			}			
		}
		*/
		for (size_t i = 0; i < mWritePackets.size(); i++)
		{
			auto pPkt = mWritePackets[i];
			pSocket->async_send(boost::asio::buffer(pPkt->GetData(), pPkt->GetSize()), [=](const boost::system::error_code& error, std::size_t bytes_transferred) {
				//if (boost::asio::error::eof == error || boost::asio::error::connection_reset == error 
				//	|| error == boost::asio::error::connection_aborted)
				if (error)
				{
					VLOG_ERROR("%s", error.message().c_str());
					Disconnect();
				}
				else
				{
					assert(pPkt->GetSize() == bytes_transferred);
				}
				mNetBase->FreePacket(pPkt);
			});
			mWritePackets[i] = nullptr;
		}
		mWritePackets.clear();

		//if (clock() - mLastClock >= 5000)
		//{
		//	VNetSerilizerStack<8> ser;
		//	ser && uint16_t(1);
		//	mLastClock = clock();
		//	SendPacket(0, ser.GetBytes(), ser.GetSeek());
		//}
	}

	//receive packets
	while (true)
	{
		if (mHasCurReadPacketHdr)
		{
			auto pktSize = mCurReadPacketHdr.GetSize();
			auto pktId = mCurReadPacketHdr.GetId();

			if (pktSize == 0)
			{
				VPacket* newPkt = pBase->GetEmptyPacket();
				if (pktId == EPF_Respond)
					mRespondPackets.push_back(newPkt);
				else if (pktId == EPF_Post)
					mPostPackets.push_back(newPkt);
				else if (pktId == EPF_PostOnly)
					mPostOnlyPackets.push_back(newPkt);


				mHasCurReadPacketHdr = false;
			}
			else
			{
				if (mReadBuffer.ReadAvail() >= pktSize) //do we have enough data for reading packet ?
				{
					VPacket* newPkt = pBase->AllocatePacket(pktSize);
					size_t nRead = mReadBuffer.Read(newPkt->GetData(), pktSize);
					assert(nRead == pktSize);

					if (pktId == EPF_Respond)
						mRespondPackets.push_back(newPkt);
					else if (pktId == EPF_Post)
						mPostPackets.push_back(newPkt);
					else if (pktId == EPF_PostOnly)
						mPostOnlyPackets.push_back(newPkt);

					mHasCurReadPacketHdr = false;
				}
				else
				{
					break;
				}
			}


		}
		else
		{
			//if we at least have enough data for header read it
			if (mReadBuffer.ReadAvail() >= VPacketHeader::RWSIZE)
			{
				VPacketHeader pktHeader;
				pktHeader.Read(mReadBuffer);
				mCurReadPacketHdr = pktHeader;
				mHasCurReadPacketHdr = true;
				VLOG_MESSAGE("packet header was read. size:%d id:%d", pktHeader.GetSize(), pktHeader.GetId());
				//assert(mCurReadPacketHdr.GetSize() >= 2);
			}
			else
			{
				break;
			}
		}
	}

	//handle responds (the result of post)
	{
		for (size_t i = 0; i < mRespondPackets.size(); i++)
		{
			VPacket* packet = mRespondPackets[i];
			VNetDeSerilizer respond(packet->GetData(), packet->GetSize());
			auto& post = mPostCallbacks[i];
			if (post)
			{
				post(respond);
			}
			pBase->FreePacket(packet);
			mRespondPackets[i] = nullptr;
		}

		//remove the processed responds
		mPostCallbacks.erase(mPostCallbacks.begin(), mPostCallbacks.begin() + mRespondPackets.size());
		mRespondPackets.clear();
	}
	//handle post pockets that we received 
	{
		for (size_t i = 0; i < mPostPackets.size(); i++)
		{
			VPacket* pkt = mPostPackets[i];
			pBase->HandlePost(pkt, this, false);
			pBase->FreePacket(pkt);
			mPostPackets[i] = nullptr;
		}
		mPostPackets.clear();
	}
	//handle post only packets that we received
	{
		for (size_t i = 0; i < mPostOnlyPackets.size(); i++)
		{
			VPacket* pkt = mPostOnlyPackets[i];
			pBase->HandlePost(pkt, this, true);
			pBase->FreePacket(pkt);
			mPostOnlyPackets[i] = nullptr;
		}
		mPostOnlyPackets.clear();
	}
}

FPHandleRPC VRPCHandle::GetProc() const
{
	union
	{
		void* raw;
		FPHandleRPC proc;
	};

	raw = mProcRaw;
	return proc;
}
