#include "VBuffers.h"

size_t VCircularBuffer::ReadIgnore(size_t bytes)
{
	if (bytes == 0) return 0;

	size_t capacity = capacity_;
	size_t bytes_to_read = std::min(bytes, size_);

	// Read in a single step
	if (bytes_to_read <= capacity - beg_index_)
	{
		beg_index_ += bytes_to_read;
		if (beg_index_ == capacity) beg_index_ = 0;
	}
	// Read in two steps
	else
	{
		size_t size_1 = capacity - beg_index_;
		size_t size_2 = bytes_to_read - size_1;
		beg_index_ = size_2;
	}

	size_ -= bytes_to_read;
	return bytes_to_read;
}

size_t VCircularBuffer::ReadPt(size_t bytes, void* pBlocks[2], size_t sizes[2]) const
{
	pBlocks[0] = pBlocks[1] = nullptr;
	sizes[0] = sizes[1] = 0;

	//char* data = (char *)pData;
	if (bytes == 0) return 0;

	size_t capacity = capacity_;
	size_t bytes_to_read = std::min(bytes, size_);

	// Read in a single step
	if (bytes_to_read <= capacity - beg_index_)
	{
		pBlocks[0] = data_ + beg_index_;
		sizes[0] = bytes_to_read;

		//memcpy(data, data_ + beg_index_, bytes_to_read);
		//beg_index_ += bytes_to_read;
		//if (beg_index_ == capacity) beg_index_ = 0;
	}
	// Read in two steps
	else
	{

		size_t size_1 = capacity - beg_index_;
		//memcpy(data, data_ + beg_index_, size_1);
		size_t size_2 = bytes_to_read - size_1;
		//memcpy(data + size_1, data_, size_2);
		//beg_index_ = size_2;

		pBlocks[0] = data_ + beg_index_;
		sizes[0] = size_1;
		pBlocks[1] = data_;
		sizes[1] = size_2;
	}

	//size_ -= bytes_to_read;
	return bytes_to_read;
}

size_t VCircularBuffer::Read(void* pData, size_t bytes)
{
	char* data = (char *)pData;
	if (bytes == 0) return 0;

	size_t capacity = capacity_;
	size_t bytes_to_read = std::min(bytes, size_);

	// Read in a single step
	if (bytes_to_read <= capacity - beg_index_)
	{
		memcpy(data, data_ + beg_index_, bytes_to_read);
		beg_index_ += bytes_to_read;
		if (beg_index_ == capacity) beg_index_ = 0;
	}
	// Read in two steps
	else
	{
		size_t size_1 = capacity - beg_index_;
		memcpy(data, data_ + beg_index_, size_1);
		size_t size_2 = bytes_to_read - size_1;
		memcpy(data + size_1, data_, size_2);
		beg_index_ = size_2;
	}

	size_ -= bytes_to_read;
	return bytes_to_read;
}

size_t VCircularBuffer::Write(const void* pData, size_t bytes)
{
	char* data = (char *)pData;
	if (bytes == 0) return 0;

	size_t capacity = capacity_;
	size_t bytes_to_write = std::min(bytes, capacity - size_);

	// Write in a single step
	if (bytes_to_write <= capacity - end_index_)
	{
		memcpy(data_ + end_index_, data, bytes_to_write);
		end_index_ += bytes_to_write;
		if (end_index_ == capacity) end_index_ = 0;
	}
	// Write in two steps
	else
	{
		size_t size_1 = capacity - end_index_;
		memcpy(data_ + end_index_, data, size_1);
		size_t size_2 = bytes_to_write - size_1;
		memcpy(data_, data + size_1, size_2);
		end_index_ = size_2;
	}

	size_ += bytes_to_write;
	return bytes_to_write;
}
size_t VCircularBuffer::WriteIgnore(size_t bytes)
{
	if (bytes == 0) return 0;

	size_t capacity = capacity_;
	size_t bytes_to_write = std::min(bytes, capacity - size_);

	// Write in a single step
	if (bytes_to_write <= capacity - end_index_)
	{
		end_index_ += bytes_to_write;
		if (end_index_ == capacity) end_index_ = 0;
	}
	// Write in two steps
	else
	{
		size_t size_1 = capacity - end_index_;
		size_t size_2 = bytes_to_write - size_1;
		end_index_ = size_2;
	}

	size_ += bytes_to_write;
	return bytes_to_write;
}

VCircularBuffer::VCircularBuffer(size_t capacity) : beg_index_(0)
, end_index_(0)
, size_(0)
, capacity_(capacity)
{
	data_ = new char[capacity];
}

VCircularBuffer::VCircularBuffer()
{
	data_ = nullptr;
	beg_index_ = end_index_ = size_ = capacity_ = 0;
}

VCircularBuffer::~VCircularBuffer()
{
	if (data_)
	{
		delete[] data_;
		data_ = nullptr;
	}
}
