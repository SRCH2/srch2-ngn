#ifndef __ALLOCATOR_H__
#define __ALLOCATOR_H__

#include <memory>
#include <cstddef>
#include <climits>

// ceilings input to the closest power of two greater, unless it is smaller an
// 64 is which case it returns 256
static inline size_t round(size_t input) {
#ifdef __GNUC__
    if(input == 0) return 1;
    int allocSize = 
      1 << (((sizeof(size_t) == 8) ? 64 : 32) - __builtin_clz(input));
    return (allocSize == 0) ? UINT_MAX : (allocSize < 64) ? 256 : allocSize;
#else
     input--;
     input |= input >> 1;
     input |= input >> 2;
     input |= input >> 4;
     input |= input >> 8;
     input |= input >> 16;
     if(64 bit) input >> 32;
     input++;
     return (input == 0) ? UINT_MAX : (input < 64) ? 256 : input;
#endif
}

//Used to Allocate memory from the heap in the same way as the standard
//template library's allocator; however, it rounds allocation to the nearest
//power of two greater than it
struct DefaultBufferAllocator : public std::allocator<char> {
  size_t round(size_t input) {
    return ::round(input);
  }
};

// Reuses the same piece of memory over and over again. The size of this 
// memory piece is at least the size of the largest allocated unit
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

inline void SingleBufferAllocator::deallocate(pointer p, size_type) {
  //Can not deallocate part of current buffer or an empty pointer
  if(!p || p == buffer || 
      (p > buffer && p < buffer + (1 << powerOfTwoSizeOfBuffer)))  {
      return;
  }

  delete [] p;

  //null pointer for safety: Dangling Pointers :(
  buffer = NULL;
}

inline SingleBufferAllocator::pointer 
SingleBufferAllocator::allocate(size_type n, const_pointer) {
  size_t allocateRound = round(n);
  char allocatePowerOfTwo = (char) __builtin_ffs(allocateRound);

  //must also check buffer in case it was accidently explicitly deleted
  //by user
  if(allocatePowerOfTwo == powerOfTwoSizeOfBuffer && buffer) {
    return buffer;
  }
  
  char *oldBuffer = buffer;
  buffer = new char[allocateRound];

  //deallocate the oldBuffer to prevent leaks
  if(oldBuffer) delete [] oldBuffer;

  powerOfTwoSizeOfBuffer = allocatePowerOfTwo;

  return buffer;
}

inline void SingleBufferAllocator::construct(pointer p, const_reference val) {}
inline void SingleBufferAllocator::destroy(pointer p) {}

inline size_t maxSize() { return  1 << ((sizeof(size_t) == 8) ?  64 : 32); }

inline SingleBufferAllocator::SingleBufferAllocator() 
  : buffer(NULL), powerOfTwoSizeOfBuffer(0) {}

inline SingleBufferAllocator::~SingleBufferAllocator() {
  if(buffer) {
    delete [] buffer;
  }
}

#endif /* __ALLOCATOR_H__ */
