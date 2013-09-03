//$Id: $
/*
 * ULEB128.h
 *
 *  Created on: Sep 2, 2013
 *      Author: sbisht
 */

#ifndef __CORE_UTIL_ULEB128_H__
#define __CORE_UTIL_ULEB128_H__
#include <vector>
#include <stdint.h>
using namespace std;

namespace srch2 {
namespace util {

class ULEB128 {
public:
    static int uInt32VectorToVarLenArray(const std::vector<unsigned>& positionList,
    		uint8_t** buffer);
    static void varLenByteArrayToInt32Vector(uint8_t* buffer, unsigned size,
    		vector<unsigned>& positionList);
    static int uInt32ToVarLengthBytes(unsigned position, uint8_t* varLenByteBuffer,
    		short * length);
    static int varLengthBytesToUInt32(const uint8_t* varLenByteBuffer, unsigned* position,
    		short* byteRead);
private:
    ULEB128();
};
}
}
#endif /* __CORE_UTIL_ULEB128_H__ */
