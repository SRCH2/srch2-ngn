#ifndef __RECORDIDITERATOR_H__
#define __RECORDIDITERATOR_H__

#include <limits.h>
#include <stdint.h>
#include <assert.h>


class RecordIdSetIterator {
public:
    static const int NO_MORE_RECORDS = INT_MAX;
    // Return NO_MORE_RECORDS if iterator has exhausted.
    // Otherwise it should return the record ID
    virtual int recordID() = 0;

    // Advance to the next record and return it currently recod ID
    virtual int nextRecord() = 0;

    // Advance to the first beyond the current record number and greater than equal to the target
    virtual int advance(int target) = 0;

    virtual uint64_t cost() = 0;

};

#endif
