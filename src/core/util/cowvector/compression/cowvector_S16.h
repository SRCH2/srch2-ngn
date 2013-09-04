
// $Id: cowvector_S16.h 3456 2013-06-14 02:11:13Z jiaying $
#ifndef __COWVECTOR_S16__
#define __COWVECTOR_S16__

#include "S16.h"
#include "util/cowvector/cowvector.h"
#include "index/InvertedIndex.h"
#include <vector>

using namespace std;

namespace srch2
{
namespace instantsearch
{

// VectorViewS16 has the similar interface as vector<unsigned>, but store data in S16 compressed format
// It segments the data to blocks, every block store S16_COMPRESSION_BUFFER_SIZE elements
// VectorViewS16 doesn't compress the last block until it's full
// Example. If there are 35 elements, S16_COMPRESSION_BUFFER_SIZE = 10,
// It will store the first 30 as 3 compressed blocks and the last 5 element uncompressed
class VectorViewS16
{
public:
	const static unsigned S16_COMPRESSION_BUFFER_SIZE = 100;
	VectorViewS16()
	{
		compressedDataBlocks.reset(new vector<vector<unsigned> >(0));
		m_size = 0;
		lastBufferSize = 0;
		decompressedBlockId = -1;
	}

	void push_back(const unsigned inElement)
	{
		lastBuffer.push_back(inElement);
		lastBufferSize ++;
		m_size ++;
		if(lastBufferSize == S16_COMPRESSION_BUFFER_SIZE){
			compressLastBuffer();
			lastBufferSize = 0;
		}
	}

	unsigned& at(unsigned idx)
	{
		assert(idx < m_size);
		// the data is in compressed blocks
		if(idx < m_size - lastBufferSize )
		{
			// if the data was not decompressed before, it should be decompressed first
			if((int)(idx / S16_COMPRESSION_BUFFER_SIZE) != decompressedBlockId)
				decompressBlock(idx/S16_COMPRESSION_BUFFER_SIZE);
			return decompressedBlock[idx%S16_COMPRESSION_BUFFER_SIZE];
		}
		else // the data is in the last Buffer
		{
			return lastBuffer[idx- (m_size - lastBufferSize)];
		}
	}

	unsigned& operator[](unsigned idx)
	{
		return this->at(idx);
	}

	void clear()
	{
		compressedDataBlocks->clear();
		lastBuffer.clear();
		decompressedBlock.clear();
		m_size = 0;
		lastBufferSize = 0;
		decompressedBlockId = -1;
	}

	size_t size() const
	{
		return m_size;
	}
protected:
	shared_ptr<vector<vector<unsigned> > > compressedDataBlocks;		// the compressed blocks
	vector<unsigned> lastBuffer;										// the last one block
	unsigned lastBufferSize;											// the size of the last one
	vector<unsigned> decompressedBlock;									// the cached block
	int decompressedBlockId;											// the id of the cached block
	size_t m_size;														// total size

	friend class boost::serialization::access;
	template<class Archive>
	//TODO further reduce the size
	void serialize(Archive & ar, const unsigned int version)
	{
		typename Archive::is_loading load;
		typename Archive::is_saving save;
		if(save)
		{
			if(lastBufferSize > 0)
				compressLastBuffer();
		}
		ar & compressedDataBlocks;
		ar & m_size;
		ar & lastBufferSize;
		if(load)
		{
			decompressedBlockId = -1;
			if(lastBufferSize > 0)
				decompressLastBuffer();
		}
	}
	virtual void compressLastBuffer() = 0;
	virtual void decompressLastBuffer() = 0;
	virtual void compressBlock() = 0;
	virtual void decompressBlock(unsigned idx) = 0;
};

class VectorViewS16Big: public VectorViewS16
{
	// compress the last buffer to compressed blocks
	void compressLastBuffer()
	{
		unsigned compressedSize = S16::getEncodeSize(&lastBuffer[0], lastBufferSize, S16::big_directory);
		compressedDataBlocks->push_back(vector<unsigned>(compressedSize));
		S16::encode(&lastBuffer[0], lastBufferSize, &compressedDataBlocks->back()[0], S16::big_directory);
		lastBuffer.clear();
	}
	// decompress the last buffer from compressed blocks
	void decompressLastBuffer()
	{
		lastBuffer.resize(lastBufferSize);
		int lastBlockId = m_size/S16_COMPRESSION_BUFFER_SIZE;
		S16::decode(&compressedDataBlocks->at(lastBlockId)[0], compressedDataBlocks->at(lastBlockId).size(), &lastBuffer[0], S16::big_directory);
		compressedDataBlocks->pop_back();
	}
	// compress the cache block to compressed blocks
	void compressBlock()
	{
		unsigned compressedSize = S16::getEncodeSize(&decompressedBlock[0], S16_COMPRESSION_BUFFER_SIZE, S16::big_directory);
		compressedDataBlocks->at(decompressedBlockId) = vector<unsigned>(compressedSize);
		S16::encode(&decompressedBlock[0], S16_COMPRESSION_BUFFER_SIZE, &compressedDataBlocks->at(decompressedBlockId)[0], S16::big_directory);
	}

	// decompress the idx compressed block to the cache block
	void decompressBlock(unsigned idx)
	{
		if(decompressedBlockId != -1) compressBlock();
		decompressedBlockId = idx;
		unsigned* compressedArray = &(compressedDataBlocks->at(decompressedBlockId)[0]);
		unsigned size = compressedDataBlocks->at(decompressedBlockId).size();
		decompressedBlock = vector<unsigned>(S16_COMPRESSION_BUFFER_SIZE);
		S16::decode(compressedArray, size, &decompressedBlock[0],S16::big_directory);
	}
};

class VectorViewS16Small: public VectorViewS16
{
	// compress the last buffer to compressed blocks
	void compressLastBuffer()
	{
		unsigned compressedSize = S16::getEncodeSize(&lastBuffer[0], lastBufferSize, S16::small_directory);
		compressedDataBlocks->push_back(vector<unsigned>(compressedSize));
		S16::encode(&lastBuffer[0], lastBufferSize, &compressedDataBlocks->back()[0], S16::small_directory);
		lastBuffer.clear();
	}
	// decompress the last buffer from compressed blocks
	void decompressLastBuffer()
	{
		lastBuffer.resize(lastBufferSize);
		int lastBlockId = m_size/S16_COMPRESSION_BUFFER_SIZE;
		S16::decode(&compressedDataBlocks->at(lastBlockId)[0], compressedDataBlocks->at(lastBlockId).size(), &lastBuffer[0], S16::small_directory);
		compressedDataBlocks->pop_back();
	}
	// compress the cache block to compressed blocks
	void compressBlock()
	{
		unsigned compressedSize = S16::getEncodeSize(&decompressedBlock[0], S16_COMPRESSION_BUFFER_SIZE, S16::small_directory);
		compressedDataBlocks->at(decompressedBlockId) = vector<unsigned>(compressedSize);
		S16::encode(&decompressedBlock[0], S16_COMPRESSION_BUFFER_SIZE, &compressedDataBlocks->at(decompressedBlockId)[0], S16::small_directory);
	}

	// decompress the idx compressed block to the cache block
	void decompressBlock(unsigned idx)
	{
		if(decompressedBlockId != -1) compressBlock();
		decompressedBlockId = idx;
		unsigned* compressedArray = &(compressedDataBlocks->at(decompressedBlockId)[0]);
		unsigned size = compressedDataBlocks->at(decompressedBlockId).size();
		decompressedBlock = vector<unsigned>(S16_COMPRESSION_BUFFER_SIZE);
		S16::decode(compressedArray, size, &decompressedBlock[0], S16::small_directory);
	}
};


// InvertedListVectorView have a interface as vector<InvertedListElement>, but store in two array inside
class InvertedListVectorView
{
public:
	InvertedListVectorView() {};

	InvertedListVectorView(InvertedListVectorView *invertedlistVV)
	{
		recordIds = invertedlistVV->recordIds;
		positionIndexOffsets = invertedlistVV->positionIndexOffsets;
	}

	void getElement(unsigned idx, InvertedListElement &invertedListElement)
	{
		assert(idx < recordIds.size());
		invertedListElement.recordId = recordIds[idx];
		invertedListElement.positionIndexOffset = positionIndexOffsets[idx];
	}

	void push_back(const InvertedListElement &inElement)
	{
		recordIds.push_back(inElement.recordId);
		positionIndexOffsets.push_back(inElement.positionIndexOffset);
	}

	void setInvertedListElement(int idx, const unsigned _recordId, const unsigned _positionIndexOffset)
	{
		recordIds[idx] = _recordId;
		positionIndexOffsets[idx] = _positionIndexOffset;
	}

	void setInvertedListElement(int idx, const InvertedListElement &inElement)
	{
		recordIds[idx] = inElement.recordId;
		positionIndexOffsets[idx] = inElement.positionIndexOffset;
	}

	void getInvertedList(vector<InvertedListElement> &invertedlist)
	{
		invertedlist.resize(recordIds.size());
		for(unsigned i = 0; i < recordIds.size(); i++)
		{
			invertedlist[i].recordId = recordIds[i];
			invertedlist[i].positionIndexOffset = positionIndexOffsets[i];
		}
	}

	void setInvertedList(const vector<InvertedListElement> &invertedlist)
	{
		recordIds.clear();
		positionIndexOffsets.clear();
		for(unsigned i = 0; i< invertedlist.size(); i++)
		{
			recordIds.push_back(invertedlist[i].recordId);
			positionIndexOffsets.push_back(invertedlist[i].positionIndexOffset);
		}
	}

	size_t size() const {return recordIds.size();}

private:
	VectorViewS16Big recordIds;
	VectorViewS16Small positionIndexOffsets;

	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & recordIds;
		ar & positionIndexOffsets;
	}

};

// CowInvertedList keep the readView and writeViw, and support copy on write(cow)
class CowInvertedList {

private:
    shared_ptr<InvertedListVectorView > m_readView;
    shared_ptr<InvertedListVectorView > m_writeView;

    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        ar << this->m_readView;
    }

    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        ar >> this->m_readView;
        m_writeView = m_readView;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int file_version)
	{
        boost::serialization::split_member(ar, *this, file_version);
    }

public:
    CowInvertedList()
	{
        m_readView.reset(new InvertedListVectorView());
        m_writeView = m_readView;
    };

    CowInvertedList(unsigned capacity)
	{
		m_readView.reset(new InvertedListVectorView());
		m_writeView = m_readView;
	};

    virtual ~CowInvertedList() {}

    void getReadView(shared_ptr<InvertedListVectorView>& view) const
    {
        view = m_readView;
    }

    void getWriteView(shared_ptr<InvertedListVectorView> & view)
    {
        view = m_writeView;
    }

    void commit() //Set readView to writeView and create new a writeView from readViewCopy.
    {
		m_readView = m_writeView;
		m_writeView.reset(new InvertedListVectorView(*m_readView));
	}
};

}
}
#endif
