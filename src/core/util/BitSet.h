#ifndef __BITSET_H__
#define __BITSET_H__

#include <stdint.h>
#include <string.h>
#include "BitSetIterator.h"
//#include "BitUtil.h"

class BitSet {

private:
    //TODO Take care of release memory
    uint64_t* bits;
    int numBits;
    int wordLength;

public:
    BitSet(){
        bits = NULL;
        numBits = wordLength = 0;
    }

    BitSet(int numBits) {
        this->numBits = numBits;
        wordLength = bits2words(numBits);
        bits = new uint64_t[wordLength];
    }

    BitSet(uint64_t* storedBits, int numBits) {
        this->wordLength = bits2words(numBits);
        this->numBits = numBits;
        this->bits = storedBits;
    }

    // do deep copy
    BitSet(const BitSet &other) {
        bits = new uint64_t[other.wordLength];
        memcpy(bits, other.bits, other.wordLength);
        numBits = other.numBits;
        wordLength = other.wordLength;
    }


    ~BitSet() {
        if (bits)
            delete[] bits;
    }

    // return the number of 64 bit words int to hold numBits
    int bits2words(int numBits) {
        int num = numBits >> 6;
        if ((numBits & 63) != 0)
            num++;
        return num;
    }

    void resize(int numBits){
        this->numBits = numBits;
        wordLength = bits2words(numBits);
        uint64_t* newbits = new uint64_t[wordLength];
        memset(newbits, 0, wordLength*sizeof(uint64_t));
        if(bits){
            memcpy(newbits, bits, wordLength);
            delete[] bits;
        }
        bits = newbits;
    }

    //TODO take case of the release of the memory
    RecordIdSetIterator* iterator() {
        return new BitSetIterator(bits, wordLength);
    }

    int length() {
        return numBits;
    }
    bool get(int index) {
        assert(index >= 0 && index < numBits);
        int wordNum = index >> 6;
        int bit = index & 0x3f;
        uint64_t bitmask = 1L << bit;
        return (bits[wordNum] & bitmask) != 0;
    }

    void set(int index){
	   assert(index >= 0 && index < numBits);
	   int wordNum = index >> 6;
	   int bit = index & 0x3f;
	   uint64_t bitmask = 1L << bit;
	   bits[wordNum] |= bitmask;
   }


    bool getAndSet(int index) {
        assert(index >= 0 && index < numBits);
        int wordNum = index >> 6;
        int bit = index & 0x3f;
        uint64_t bitmask = 1L << bit;
        bool val = (bits[wordNum] & bitmask) != 0;
        bits[wordNum] |= bitmask;
        return val;
    }

    void clear(int index) {
        assert(index >= 0 && index < numBits);
        int wordNum = index >> 6;
        int bit = index & 0x3f;
        uint64_t bitmask = 1L << bit;
        bits[wordNum] &= ~bitmask;
    }

    bool getAndClear(int index) {
        assert(index >= 0 && index < numBits);
        int wordNum = index >> 6;
        int bit = index & 0x3f;
        uint64_t bitmask = 1L << bit;
        bool val = (bits[wordNum] & bitmask) != 0;
        bits[wordNum] &= ~bitmask;
        return val;
    }
};

#endif
