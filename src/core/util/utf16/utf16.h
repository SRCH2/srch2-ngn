#ifndef __UFT16_H__
#define __UFT16_H__

#define U16_LENGTH(c) (((c)<0xD800 || (c)>0xDBFF) ? 1 : 2)

namespace utf16 {

  // The typedefs for 8-bit, 16-bit and 32-bit unsigned integers
  // You may need to change them to match your system.
  // These typedefs have the same names as ones from cstdint, or boost/cstdint
  typedef unsigned char   uint8_t;
  typedef unsigned short  uint16_t;
  typedef unsigned int    uint32_t;

  inline int byteLength(int numChars, uint16_t const* wchar) {
    int byteSize=0;
    short len;
    for(int i=0; i<numChars; ++i) {
      len= U16_LENGTH(*wchar);
      byteSize+= len*2;
      wchar+= len;
    }
    return byteSize;
  }
}

#endif /* __UFT16_H */

