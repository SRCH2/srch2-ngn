#ifndef __UTIL_BITSET_H__
#define __UTIL_BITSET_H__

#include <stdint.h>
#include <string.h>
#include "util/Assert.h"
#include "BitSetIterator.h"

namespace srch2
{
namespace instantsearch
{
/*
 *  Bitset is very similar to vector<bool>, it contains a collection of bits, and provides constant-time access to each bit.
 *  It has two difference from the stl bitset<T>.
 *  1. stl bitset<T> must have the template parameter T, which specifies the number of bits in the bitset,
 *  must be an integer constant. Our Bitset doesn't have such a restriction, it can be resized.
 *  2. stl bitset<T> does not have iterators. Bitset can have a BitSetIterator, which can be returned by calling an iterator function.
 * */
class BitSet {

private:
    // This is an array of bits. We use uint64_t as the container
    uint64_t* bits;
    // numBits specifies how many bits are kept in this array
    int numBits;
    // workLength is the number of uint64_t's we allocate. We keep at most 64 * wordLength bits.
    int wordLength;

public:
    // constructors
    BitSet(){
        bits = NULL;
        numBits = wordLength = 0;
    }

    BitSet(int numBits) {
        ASSERT(numBits >= 0);
        this->numBits = numBits;
        wordLength = getWordNumberForBits(numBits);
        bits = new uint64_t[wordLength];
    }

    BitSet(uint64_t* storedBits, int numBits) {
        ASSERT(numBits >= 0);
        this->wordLength = getWordNumberForBits(numBits);
        this->numBits = numBits;
        this->bits = storedBits;
    }

    // Copy constructor. We do a deep copy
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

    // return the number of 64-bit words to store a given number of bits
    int getWordNumberForBits(int numBits) {
        int wordNumber = numBits >> 6;
        if ((numBits & 0x3f) != 0) // If the remainder is more than 0, we need to allocate one more word for these bits.
            wordNumber++;
        return wordNumber;
    }

    // resize the array
    void resize(int numBits){
        ASSERT(numBits >= 0);
        this->numBits = numBits;
        wordLength = getWordNumberForBits(numBits);
        uint64_t* newBits = new uint64_t[wordLength];
        memset(newBits, 0, wordLength*sizeof(uint64_t));
        if(bits){
            memcpy(newBits, bits, wordLength);
            delete[] bits;
        }
        bits = newBits;
    }

    // return an iterator of this Bitset
    RecordIdSetIterator* iterator() {
        return new BitSetIterator(bits, wordLength);
    }

    int getBitNumber() {
        return numBits;
    }

    // get bit at index position(start from 0)
    bool get(int index) {
        ASSERT(index >= 0 && index < numBits);
        int wordNum = index >> 6;
        int remainderBits = index & 0x3f;
        // for example. we get index = 66, we will first get wordNum = 66 >> 6 = 1, remainderBits = 66 & 0x3f = 2,
        // so we will return the idx=1 word's idx=2 bit value
        uint64_t bitmask = 1L << remainderBits;
        return (bits[wordNum] & bitmask) != 0;
    }

    // set bit at index position(start from 0)
    void set(int index){
       ASSERT(index >= 0 && index < numBits);
	   int wordNum = index >> 6;
	   int bit = index & 0x3f;
	   // for example. we get index = 66, we will first get wordNum = 66 >> 6 = 1, remainderBits = 66 & 0x3f = 2,
	   // so we will set the idx=1 word's idx=2 bit value to be 1
	   uint64_t bitmask = 1L << bit;
	   bits[wordNum] |= bitmask;
   }

    // get bit at index position(start from 0), then set it
    bool getAndSet(int index) {
        ASSERT(index >= 0 && index < numBits);
        int wordNum = index >> 6;
        int bit = index & 0x3f;
        uint64_t bitmask = 1L << bit;
        // for example. we get index = 66, we will first get wordNum = 66 >> 6 = 1, remainderBits = 66 & 0x3f = 2,
        // so we will set the idx=1 word's idx=2 bit value to be 1, and return the old value
        bool val = (bits[wordNum] & bitmask) != 0;
        bits[wordNum] |= bitmask;
        return val;
    }

    // clear bit at index position(start from 0)
    void clear(int index) {
        ASSERT(index >= 0 && index < numBits);
        int wordNum = index >> 6;
        int bit = index & 0x3f;
        // for example. we get index = 66, we will first get wordNum = 66 >> 6 = 1, remainderBits = 66 & 0x3f = 2,
        // so we will set the idx=1 word's idx=2 bit value to be 0
        uint64_t bitmask = 1L << bit;
        bits[wordNum] &= ~bitmask;
    }

    // get the bit at a given index position (starting from 0), then clear it
    bool getAndClear(int index) {
        ASSERT(index >= 0 && index < numBits);
        int wordNum = index >> 6;
        int bit = index & 0x3f;
        uint64_t bitmask = 1L << bit;
        // for example. we get index = 66, we will first get wordNum = 66 >> 6 = 1, remainderBits = 66 & 0x3f = 2,
        // so we will set the idx=1 word's idx=2 bit value to be 0, and return the old value
        bool val = (bits[wordNum] & bitmask) != 0;
        bits[wordNum] &= ~bitmask;
        return val;
    }
};

}
}
#endif
