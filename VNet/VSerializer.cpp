#include "VSerializer.h"



VNetSerilizer::VNetSerilizer(size_t initialCapacity) : mCapacity(initialCapacity), mSeek(0), mBytes(nullptr)
{
	if (mCapacity)
		mBytes = (uint8_t*)malloc(initialCapacity);
}

VNetSerilizer::~VNetSerilizer()
{
	if (mBytes)
	{
		free(mBytes);
		mBytes = nullptr;
	}
	mCapacity = mSeek = 0;
}

void VNetSerilizer::CheckInc(size_t numBytes)
{
	if (numBytes > WriteAvail())
	{
		mCapacity += 128 + numBytes;
		mBytes = (uint8_t*)realloc(mBytes, mCapacity);
	}
}

void VNetSerilizer::WriteBytes(const void* bytes, size_t size)
{
	CheckInc(size);
	memcpy(GetCur(), bytes, size);
	mSeek += size;
}

void VNetDeSerilizer::ReadBytes(void* outBytes, size_t size)
{
	if (!mIsValid) return;

	if (ReadAvail() < size)
	{
		mIsValid = false;
		return;
	}

	memcpy(outBytes, mBytes + mSeek, size);
	mSeek += size;
}
