#pragma once

#include "VBase.h"
#include <vector>
#include <string.h>
#include <assert.h>

//////////////////////////////////////////////////////////////////////////
class VNetSerilizer
{
public:
	VNetSerilizer() : mBytes(nullptr), mSeek(0), mCapacity(0) {}
	VNetSerilizer(size_t initialCapacity);
	virtual ~VNetSerilizer();
	

	uint8_t* GetBytes() const { return mBytes; }
	//return the first byte at current seek
	uint8_t* GetCur() const { return mBytes + mSeek; }
	size_t GetSeek() const { return mSeek; }
	size_t GetCapacity() const { return mCapacity; }
	void Reset() { mSeek = 0; }
	//returns number of bytes available to write without reallocation
	size_t WriteAvail() const { return mCapacity - mSeek; }
	void CheckInc(size_t numBytes);

	void WriteBytes(const void* bytes, size_t size);

protected:
	uint8_t * mBytes;
	size_t mSeek;
	size_t mCapacity;
};

template<size_t Capacity> class VNetSerilizerStack : public VNetSerilizer
{
public:
	VNetSerilizerStack()
	{
		mBytes = mStackBytes;
		mCapacity = Capacity;
		mSeek = 0;
	}
	~VNetSerilizerStack()
	{
		mBytes = nullptr;
	}
	uint8_t mStackBytes[Capacity];
};

//////////////////////////////////////////////////////////////////////////
class VNetDeSerilizer
{
public:
	VNetDeSerilizer(void* pData, size_t size) : mBytes((uint8_t*)pData), mSize(size), mSeek(0), mIsValid(true)
	{
	}
	virtual ~VNetDeSerilizer()
	{
		mBytes = nullptr;
		mSeek = mSize = 0;
		mIsValid = false;
	}
	
	//returns number of bytes available to read
	size_t ReadAvail() const { return mSize - mSeek; }
	uint8_t* GetCur() const { return mBytes + mSeek; }
	size_t GetSize() const { return mSize; }
	uint8_t* GetBytes() const { return mBytes; }

	void ReadBytes(void* outBytes, size_t size);
	bool IsValid() const { return mIsValid; }

protected:
	uint8_t* mBytes;
	size_t mSeek;
	size_t mSize;
	bool mIsValid;

};

template<typename T> VNetSerilizer& operator && (VNetSerilizer& ser, const T& data)
{
	//static_assert(std::is_arithmetic<T>::value, "operator && not implemented");
	ser.WriteBytes(&data, sizeof(T));
	return ser;
}
inline VNetSerilizer& operator && (VNetSerilizer& ser, bool b)
{
	ser.WriteBytes(&b, sizeof(bool));
	return ser;
}
inline VNetSerilizer& operator && (VNetSerilizer& ser, int n)
{
	ser.WriteBytes(&n, sizeof(int));
	return ser;
}
inline VNetSerilizer& operator && (VNetSerilizer& ser, float f)
{
	ser.WriteBytes(&f, sizeof(float));
	return ser;
}
inline VNetSerilizer& operator && (VNetSerilizer& ser, const char* cstr)
{
	size_t len = strlen(cstr);
	if (len == 0)
		ser.WriteBytes("\0", 1);
	else
		ser.WriteBytes(cstr, len + 1);
	return ser;
}
inline VNetSerilizer& operator && (VNetSerilizer& ser, const std::string& stdStr)
{
	ser && (stdStr.c_str());
	return ser;
}

template<typename T> VNetSerilizer& operator && (VNetSerilizer& ser, const std::vector<T>& vec)
{
	uint32_t len = (uint32_t)vec.size();
	ser && len;
	for (uint32_t i = 0; i < len; i++)
		ser && (vec[i]);

	return ser;
}











template<typename T> VNetDeSerilizer& operator && (VNetDeSerilizer& ser, T& data)
{
	//static_assert(std::is_arithmetic<T>::value, "operator && not implemented");
	ser.ReadBytes(&data, sizeof(T));
	return ser;
}
inline VNetDeSerilizer& operator && (VNetDeSerilizer& ser, bool& b)
{
	ser.ReadBytes(&b, sizeof(bool));
	return ser;
}
inline VNetDeSerilizer& operator && (VNetDeSerilizer& ser, int& n)
{
	ser.ReadBytes(&n, sizeof(int));
	return ser;
}
inline VNetDeSerilizer& operator && (VNetDeSerilizer& ser, float& f)
{
	ser.ReadBytes(&f, sizeof(float));
	return ser;
}
inline VNetDeSerilizer& operator && (VNetDeSerilizer& ser, std::string& stdStr)
{
	if(ser.IsValid() && ser.ReadAvail() > 0)
	{
		size_t len = strlen((const char*)ser.GetCur());
		stdStr.clear();
		stdStr.resize(len);
		ser.ReadBytes((char*)stdStr.c_str(), len + 1);
		
		if (!ser.IsValid())
		{
			stdStr.clear();
		}
		else
		{
			assert(stdStr.length() == len);
		}
	}
	else
	{
		stdStr.clear();
	}
	return ser;
}


template<typename T> VNetDeSerilizer& operator && (VNetDeSerilizer& ser, std::vector<T>& vec)
{
	vec.clear();
	uint32_t num = 0;
	ser && num;
	for (uint32_t i = 0; i < num; i++)
	{
		if (ser.IsValid())
		{
			T item;
			ser && item;
			vec.push_back(item);
		}
		else
		{
			vec.clear();
			break;
		}
	}
	return ser;
}


inline void VSerUInt31(VNetSerilizer& ser, uint32_t n)
{
	assert(n <= 0x7FffFFff);
	uint32_t f = n > 0x7Fff ? 1 : 0;
	n = (n << 1) | f;
	if(f)
	{
		ser && n;
	}
	else
	{
		ser && (uint16_t)n;
	}
}
inline void VSerUInt15(VNetSerilizer& ser, uint16_t n)
{
	assert(n <= 0x7fFF);
	uint16_t f = n > 127 ? 1 : 0;
	n = (n << 1) | f;
	if (f)
	{
		ser && n;
	}
	else
	{
		ser && (uint8_t)n;
	}
}
inline uint32_t VDSerUInt31(VNetDeSerilizer& dser)
{
	uint16_t lp = 0;
	dser && lp;
	if (lp & 1)
	{
		uint16_t hp = 0;
		dser && hp;
		return (((uint32_t)lp) >> 1) | (((uint32_t)hp) << 15);
	}
	else
	{
		return ((uint32_t)lp) >> 1;
	}
}
inline uint16_t VDSerUInt15(VNetDeSerilizer& dser)
{
	uint8_t lp = 0;
	dser && lp;
	if (lp & 1)
	{
		uint8_t hp = 0;
		dser && hp;
		return (((uint16_t)lp) >> 1) | (((uint16_t)hp) << 7);
	}
	else
	{
		return ((uint16_t)lp) >> 1;
	}
}
//0 to 32,767 takes 2 bytes
//32,768 to ‭2,147,483,647‬ takes 4 bytes
struct UInt31Comp
{
	uint32_t mValue;
	UInt31Comp(){}
	UInt31Comp(uint32_t n) : mValue(n){}
};

inline VNetSerilizer& operator && (VNetSerilizer& ser, const UInt31Comp n)
{
	VSerUInt31(ser, n.mValue);
	return ser;
}
inline VNetDeSerilizer& operator && (VNetDeSerilizer& dser, UInt31Comp& n)
{
	n.mValue = VDSerUInt31(dser);
	return dser;
}
//0 to 127 takes 1 bytes
//128 to 32,767 takes 2 bytes
struct UInt15Comp
{
	uint16_t mValue;
	UInt15Comp() {}
	UInt15Comp(uint16_t n) : mValue(n) {}
};

inline VNetSerilizer& operator && (VNetSerilizer& ser, const UInt15Comp n)
{
	VSerUInt15(ser, n.mValue);
	return ser;
}
inline VNetDeSerilizer& operator && (VNetDeSerilizer& dser, UInt15Comp& n)
{
	n.mValue = VDSerUInt15(dser);
	return dser;
}