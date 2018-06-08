#pragma once

#include <algorithm>

class VCircularBuffer
{
public:
	VCircularBuffer();
	VCircularBuffer(size_t capacity);
	~VCircularBuffer();

	size_t Capacity() const { return capacity_; }
	// Return number of bytes written.
	size_t Write(const void* pData, size_t bytes);
	size_t WriteIgnore(size_t bytes);
	// Return number of bytes read.
	size_t Read(void* pData, size_t bytes);
	size_t ReadIgnore(size_t bytes);
	size_t ReadPt(size_t bytes, void* pBlocks[2], size_t sizes[2]) const;
	//returns maximum number of bytes available to write
	size_t WriteAvail() const
	{
		return capacity_ - size_;
	}
	//returns maximum number of bytes available to read
	size_t ReadAvail() const
	{
		return size_;
	}
	void Reset()
	{
		beg_index_ = end_index_ = size_ = 0;
	}
private:
	size_t beg_index_, end_index_, size_, capacity_;
	char *data_;
};
