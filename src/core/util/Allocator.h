#ifndef __ALLOCATOR_H__
#define __ALLOCATOR_H__

#include <memory>
#include <cstddef>
#include <climits>

#define IS_64_BIT ((sizeof(int) == 8) ? 1 : 0)

struct DefaultBufferAllocator : public std::allocator<char> {
  size_t round(size_t input) {
    if(input == 0) return 1;
    int allocSize = 1 << (((IS_64_BIT) ? 64 : 32) - __builtin_clz(input));
    return (allocSize == 0) ? UINT_MAX : (allocSize < 64) ? 256 : allocSize;
  /* else if not GCC
     input--;
     input |= v >> 1;
     input |= v >> 2;
     input |= v >> 4;
     input |= v >> 8;
     input |= v >> 16;
     if(64 bit) input >> 32;
     input++;
   */
  }
};


#endif /* __ALLOCATOR_H__ */
