//$Id$
/*
 * ULEB128.cpp
 *
 *  Created on: Sep 2, 2013
 *      Author: sbisht
 */

#include "ULEB128.h"
#include "util/Logger.h"
#ifndef NULL
#define NULL 0
#endif

namespace srch2 {
namespace util {
/*
 *  Converts array of unsigned 32 bit integers to variable length byte array in two steps.
 *  1- perform delta encoding of input numbers
 *  2- convert delta encoded numbers to VLB array
 *
 *  For more detail visit:
 *    http://en.wikipedia.org/wiki/Variable-length_quantity
 *    https://srch2inc.atlassian.net/wiki/pages/viewpage.action?pageId=5144602
 *
 */
int ULEB128::uInt32VectorToVarLenArray (const std::vector<unsigned>& positionList, uint8_t** buffer) {

    if (positionList.size() == 0)
        return 0;

    std::vector<uint8_t> tempBuffer;
    tempBuffer.reserve(1.5 * positionList.size());
    uint8_t buf[5];
    short len;
    uInt32ToVarLengthBytes(positionList[0], buf, &len);
    for (int j = 0; j < len; ++j){
        tempBuffer.push_back(buf[j]);
    }

    for (unsigned i = 1; i < positionList.size(); ++i) {
        uInt32ToVarLengthBytes(positionList[i] - positionList[i - 1], buf, &len);
        for (int j = 0; j < len; ++j)
            tempBuffer.push_back(buf[j]);
    }

    uint8_t *vlb = new uint8_t[tempBuffer.size()];
    for (unsigned i = 0; i< tempBuffer.size(); ++i)
        vlb[i] = tempBuffer[i];

    *buffer = vlb;
    return tempBuffer.size();
}
/*
 *  Converts variable length byte array to array of unsigned 32 bit integers in two steps.
 *  1- Converts VLB array to delta encoded numbers
 *  2 -Performs reverse delta encoding to get final output
 *
 */
void ULEB128::varLenByteArrayToInt32Vector (uint8_t* buffer, unsigned size,
		vector<unsigned>& positionList) {

    if (*(buffer + size - 1) & 0x80) {
        Logger::error("buffer has bad encoding..last byte is not a terminating one");
        return;
    }

    unsigned firstValue;
    short byteRead = 0;
    unsigned offset = 0;

    varLengthBytesToUInt32(buffer, &firstValue, &byteRead);
    positionList.push_back(firstValue);
    offset += byteRead;

    unsigned delta;
    while(offset < size) {
        varLengthBytesToUInt32(buffer + offset, &delta, &byteRead);
        unsigned previousValue = positionList.back();
        positionList.push_back(previousValue + delta);
        offset += byteRead;
    }
}
/*
 *  Converts unsigned 32 bit integer to variable length byte
 */
bool ULEB128::uInt32ToVarLengthBytes (unsigned position, uint8_t* varLenByteBuffer, short * length) {

    if (varLenByteBuffer == NULL || length  == NULL)
        return false;
    /*
     *  Note: the code below can be written as for loop but I chose to unroll the loop
     *  for better performance. Just not relying on compiler to do it for me.. other
     *  implementation on web also choose similar approach
     */

    *length = 0;
    // 7 bits
    uint8_t pos = (uint8_t)(position & 0x7F);
    *varLenByteBuffer=pos;
    (*length)++;
    position >>= 7;
    if (position == 0)
        return true;

    //14 bits
    (*varLenByteBuffer++) |= 0x80;
    pos = (uint8_t)(position & 0x7F);
    *varLenByteBuffer=pos;
    (*length)++;
    position >>= 7;
    if (position == 0)
        return true;

    //21 bits
    (*varLenByteBuffer++) |= 0x80;
    pos = (uint8_t)(position & 0x7F);
    *varLenByteBuffer=pos;
    (*length)++;
    position >>= 7;
    if (position == 0)
        return true;

    //28 bits
    (*varLenByteBuffer++) |= 0x80;
    pos = (uint8_t)(position & 0x7F);
    *varLenByteBuffer=pos;
    (*length)++;
    position >>= 7;
    if (position == 0)
        return true;

    // remaining bits
    (*varLenByteBuffer++) |= 0x80;
    pos = (uint8_t)(position & 0x7F);
    *varLenByteBuffer=pos;
    (*length)++;

    if (position>>=7 == 0)
        return -1;
    else
        return true;
}

/*
 *  Converts variable length byte to unsigned 32 bit integer.
 */

bool ULEB128::varLengthBytesToUInt32 (const uint8_t* varLenByteBuffer, unsigned* value, short* byteRead) {

    if (varLenByteBuffer == NULL || value == NULL || byteRead == NULL)
        return false;

    /*
     *  Note: the code below can be written as for loop but I chose to unroll the loop
     *  for better performance. Just not relying on compiler to do it for me.. other
     *  implementation on web also choose similar approach
     */

    short i = 0;
    unsigned& val = *value;
    // set 1-7
    val = varLenByteBuffer[i] & 0x7F;
    if (!(varLenByteBuffer[i++] & 0x80)) { *byteRead = i; return true; }
    // set 8-14
    val =  val | (varLenByteBuffer[i] & 0x7F) << 7;
    if (!(varLenByteBuffer[i++] & 0x80)) { *byteRead = i; return true; }
    // set 15-21
    val =  val | (varLenByteBuffer[i] & 0x7F) << 14;
    if (!(varLenByteBuffer[i++] & 0x80)) { *byteRead = i; return true; }
    // set 22-28
    val =  val | (varLenByteBuffer[i] & 0x7F) << 21;
    if (!(varLenByteBuffer[i++] & 0x80)) { *byteRead = i; return true; }
    // remaining
    val =  val | (varLenByteBuffer[i] & 0x7F) << 28;
    if (varLenByteBuffer[i++] & 0x80) {
        return false;  // fatal error : expeciting 32 bit integer encoding.
    }
    else {
        *byteRead = i; return true;
    }
}
}
}
