#ifndef __BITSETITERATOR_H__
#define __BITSETITERATOR_H__
#include "RecordIdSetIterator.h"
// An iterator to iterate over set bits
class BitSetIterator: public RecordIdSetIterator {
public:
    static const int bitlist[256];
private:
    uint64_t* arr;
    int words;
    int i;
    uint64_t word;
    int wordShift;
    unsigned indexArray;
    int curDocId;

public:

    BitSetIterator(uint64_t* bits, int numWords) {
        arr = bits;
        words = numWords;
        i = -1;
        curDocId = -1;
        word = 0;
        wordShift = 0;
        indexArray = 0;
    }

    int nextRecord() {
        if (indexArray == 0) {
            if (word != 0) {
                word >>= 8;
                wordShift += 8;
            }

            while (word == 0) {
                if (++i >= words) {
                    return curDocId = NO_MORE_RECORDS;
                }
                word = arr[i];
                wordShift = -1;
            }
            shift();
        }
        int bitIndex = (indexArray & 0x0f) + wordShift;
        indexArray >>= 4;

        return curDocId = (i<<6) + bitIndex;
    }

    int advance(int target) {
    	indexArray = 0;
    	i = target >> 6;
    	if (i >= words) {
    		word = 0;
    		return curDocId = NO_MORE_RECORDS;
    	}
    	wordShift = target & 0x3f;
    	word = arr[i] >> wordShift;
    	if (word != 0) {
    		wordShift--;
    	}
    	else {
    		while (word == 0) {
    			if (++i >= words) {
    				return curDocId = NO_MORE_RECORDS;
    			}
    			word = arr[i];
    		}
    		wordShift = -1;
    	}

    	shift();

    	int bitIndex = (indexArray & 0x0f) + wordShift;
    	indexArray >>= 4;
    	return curDocId = (i<<6) + bitIndex;
    }

    int recordID() {
        return curDocId;
    }


    uint64_t cost() {
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
        indexArray = bitlist[(int)word & 0xff];
    }

};

#endif
