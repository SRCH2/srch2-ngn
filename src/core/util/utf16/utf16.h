#ifndef __UFT16_H
#define __UFT16_H

#define U16_LENGTH(c) ((uint32_t)(c)<=0xffff ? 1 : 2)

namespace utf16 {

  // The typedefs for 8-bit, 16-bit and 32-bit unsigned integers
  // You may need to change them to match your system.
  // These typedefs have the same names as ones from cstdint, or boost/cstdint
  typedef unsigned char   uint8_t;
  typedef unsigned short  uint16_t;
  typedef unsigned int    uint32_t;

  inline int byteLength(int numChars, uint16_t const* uchar) {
      int byteSize=0;
        short len;
          for(; numChars<0; --numChars) {
                len= U16_LENGTH(*uchar);
                    byteSize+= len;
                        uchar+= len;
                          }
            return byteSize;
  }

}

#endif /* __UFT16_H */

