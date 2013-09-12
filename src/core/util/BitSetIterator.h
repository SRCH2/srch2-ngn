#ifndef __UTIL_BITSETITERATOR_H__
#define __UTIL_BITSETITERATOR_H__
#include "RecordIdSetIterator.h"

namespace srch2
{
namespace instantsearch
{
// An iterator to iterate over Bitset
class BitSetIterator: public RecordIdSetIterator {
public:
    static const int bitList[256];
private:
    // bits is a pointer to the Bitset's bits
    uint64_t* bits;
    // words is the wordLength of Bitset
    int words;
    // i is current word position
    int i;
    // word contains current 64 bits
    uint64_t word;
    // shift bit in a word
    int wordShift;
    // indexArray keep next bits in current shifted word
    unsigned indexArray;
    // current record id
    int curRecordId;

public:

    BitSetIterator(uint64_t* bits, int numWords) {
        this->bits = bits;
        words = numWords;
        i = -1;
        curRecordId = -1;
        word = 0;
        wordShift = 0;
        indexArray = 0;
    }

    // return the next Record in the Bitset
    int getNextRecordId() {
        // if the indexArray is empty, we will move to the next byte in current word
        if (indexArray == 0) {
            if (word != 0) {
                word >>= 8;
                wordShift += 8;
            }
            // if the word is empty, we move to the next word
            while (word == 0) {
                if (++i >= words) {// if it's larger than words, return no more records
                    return curRecordId = NO_MORE_RECORDS;
                }
                // set the word and wordShift
                word = bits[i];
                wordShift = -1;
            }
            //shift to the next wordshift
            shift();
        }
        int bitIndex = (indexArray & 0x0f) + wordShift; //get the bit index
        indexArray >>= 4;
        // return current record id
        return curRecordId = (i<<6) + bitIndex;
    }

    // advance the next record which id >= target
    int advance(int target) {
    	indexArray = 0;
    	i = target >> 6;
    	if (i >= words) {
    		word = 0;
    		return curRecordId = NO_MORE_RECORDS;
    	}
    	wordShift = target & 0x3f;
    	word = bits[i] >> wordShift;
    	if (word != 0) {
    		wordShift--;
    	}
    	else {
    		while (word == 0) {
    			if (++i >= words) {
    				return curRecordId = NO_MORE_RECORDS;
    			}
    			word = bits[i];
    		}
    		wordShift = -1;
    	}

    	shift();

    	int bitIndex = (indexArray & 0x0f) + wordShift;
    	indexArray >>= 4;
    	return curRecordId = (i<<6) + bitIndex;
    }

    int getCurrentRecordID() {
        return curRecordId;
    }


    uint64_t getNumberOfWords() {
        return words / 64;
    }

private:
    // 64 bit shifts
    void shift() {
        if ((int)word == 0) {
            wordShift +=32;
            word = word >>32;
        }
        if ((word & 0x0000FFFF) == 0) {
            wordShift +=16;
            word >>=16;
        }
        if ((word & 0x000000FF) == 0) {
            wordShift +=8;
            word >>=8;
        }
        indexArray = bitList[(int)word & 0xff];
    }

};

}
}
#endif
