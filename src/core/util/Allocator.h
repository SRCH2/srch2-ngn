#ifndef __ALLOCATOR_H__
#define __ALLOCATOR_H__

#include <memory>
#include <cstddef>
#include <climits>

#define IS_64_BIT ((sizeof(int) == 8) ? 1 : 0)

static inline size_t round(size_t input) {
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

struct DefaultBufferAllocator : public std::allocator<char> {
  size_t round(size_t input) {
    return ::round(input);
  }
};

struct SingleBufferAllocator {
  typedef char value_type;
  typedef char* pointer;
  typedef char& reference;
  typedef const char* const_pointer;
  typedef const char& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

 size_t round(size_t input) {
    return ::round(input);
  }

  pointer buffer;
  char powerOfTwoSizeOfBuffer;

  pointer address(reference);
  const_pointer address(const_reference);

  pointer allocate(size_type, const_pointer hint=0);
  void deallocate(pointer, size_type);
  size_t max_size();

  void construct(pointer p, const_reference);
  void destroy(pointer p);

  ~SingleBufferAllocator();
  SingleBufferAllocator();
};

inline SingleBufferAllocator::pointer 
SingleBufferAllocator::address(reference x) {
  return &x;
}

inline SingleBufferAllocator::const_pointer 
SingleBufferAllocator::address(const_reference x) {
  return &x;
}

inline SingleBufferAllocator::pointer 
SingleBufferAllocator::allocate(size_type n, const_pointer) {
  size_t allocateRound = round(n);
  char allocatePowerOfTwo = (char) __builtin_ffs(allocateRound);

  if(allocatePowerOfTwo == powerOfTwoSizeOfBuffer) {
    return buffer;
  }

  buffer = new char[allocateRound];
  allocatePowerOfTwo = powerOfTwoSizeOfBuffer;

  return buffer;
}
inline void SingleBufferAllocator::deallocate(pointer p, size_type) {
  if(p == buffer) return;

  delete [] buffer;

}
inline void SingleBufferAllocator::construct(pointer p, const_reference val) {}
inline void SingleBufferAllocator::destroy(pointer p) {}

inline size_t maxSize() { return ((size_t) 1) << 63; }

inline SingleBufferAllocator::SingleBufferAllocator() 
  : buffer(NULL), powerOfTwoSizeOfBuffer(0) {}
inline SingleBufferAllocator::~SingleBufferAllocator() {
  if(buffer) {
    delete [] buffer;
  }
}

#endif /* __ALLOCATOR_H__ */
