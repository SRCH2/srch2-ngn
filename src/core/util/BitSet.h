#ifndef __UTIL_BITSET_H__
#define __UTIL_BITSET_H__

#include <stdint.h>
#include <string.h>
#include "BitSetIterator.h"
//#include "BitUtil.h"

/*
 *  Bitset is very similar to vector<bool>, it contains a collection of bits, and provides constant-time access to each bit.
 *  It have two difference from the stl bitset<T>.
 *  1. stl bitset<T> must have the template parameter T, which specifies the number of bits in the bitset,
 *  must be an integer constant. Our Bitset doesn't have such restrict, it can be resize.
 *  2. stl bitset<T> does not have iterators, we have BitSetIterator, which can be return by iterator function.
 * */
class BitSet {

private:
    // This is bits array, we used uint64_t as the container
    uint64_t* bits;
    // numBits specify how many bits kept in this Bitset
    int numBits;
    // workLength is the number of uint64_t we allocate, in Bitset, we can keep at most 64 * wordLength bits.
    int wordLength;

public:
    // constructors
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

    // destructor
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

    // resize the array
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

    // return an iterator of this Bitset
    RecordIdSetIterator* iterator() {
        return new BitSetIterator(bits, wordLength);
    }

    int length() {
        return numBits;
    }

    // get bit at index position(start from 0)
    bool get(int index) {
        assert(index >= 0 && index < numBits);
        int wordNum = index >> 6;
        int bit = index & 0x3f;
        uint64_t bitmask = 1L << bit;
        return (bits[wordNum] & bitmask) != 0;
    }

    // set bit at index position(start from 0)
    void set(int index){
	   assert(index >= 0 && index < numBits);
	   int wordNum = index >> 6;
	   int bit = index & 0x3f;
	   uint64_t bitmask = 1L << bit;
	   bits[wordNum] |= bitmask;
   }

    // get bit at index position(start from 0), then set it
    bool getAndSet(int index) {
        assert(index >= 0 && index < numBits);
        int wordNum = index >> 6;
        int bit = index & 0x3f;
        uint64_t bitmask = 1L << bit;
        bool val = (bits[wordNum] & bitmask) != 0;
        bits[wordNum] |= bitmask;
        return val;
    }

    // clear bit at index position(start from 0)
    void clear(int index) {
        assert(index >= 0 && index < numBits);
        int wordNum = index >> 6;
        int bit = index & 0x3f;
        uint64_t bitmask = 1L << bit;
        bits[wordNum] &= ~bitmask;
    }

    // get bit at index position(start from 0), then clear it
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
