#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

#include "Allocator.h"
#include "instantsearch/Schema.h"
#include <vector>

#define DEFAULT_VARIBLE_ATTRIBUTE_LENGTH 64

typedef unsigned offset_type;

enum variable_length_types {
  VARIABLE_LENGTH_TYPE_STRING 
};

class Serializer {
  DefaultBufferAllocator& allocator;
  srch2::instantsearch::Schema& schema;
  offset_type maxOffsetOfBuffer;
  offset_type lastOffsetOfWrittenBuffer;
  //first: searchable Offset and second: Refining Offsets
  const std::pair<std::vector<offset_type>, std::vector<offset_type> > 
    offsets;
  //marks the end of the fixed size buffer, ie. all the offsets & fixed values
  const offset_type fixedSizedOffset;
  //marks the begin of offset to variable length code
  const offset_type variableLengthOffsetStart;
  char *buffer;

  template<typename T>
  void add(const offset_type offset, const T& attribute);

  void expandBuffer(size_t);
  offset_type 
    calculateVariableLengthOffset(variable_length_types, offset_type);
  bool repositionBuffer(offset_type, offset_type, offset_type); 

 public:
  Serializer(srch2::instantsearch::Schema&, DefaultBufferAllocator&);

  template<typename T>
  void addRefiningAttribute(const int, const T&);

  template<typename T>
  void addSearchableAttribute(const int, const T&);

  offset_type getSearchableOffset(const unsigned);
  offset_type getRefiningOffset(const unsigned);

  void* serialize();
};

template <> void
Serializer::add<std::string>(const offset_type, const std::string&);

inline
offset_type Serializer::getSearchableOffset(const unsigned searchableId) {
  return offsets.first.at(searchableId);
}

inline
offset_type Serializer::getRefiningOffset(const unsigned refiningId) {
  return offsets.second.at(refiningId);
}

template<typename T> inline
void Serializer::addRefiningAttribute(const int refiningId, 
    const T& attribute) {
 // ASSERT(0 < refiningID && refiningID < offsets.second.size());

  add(offsets.second.at(refiningId), attribute);
}

template<typename T> inline
void Serializer::addSearchableAttribute(const int searchableId,
    const T& attribute) {
 // ASSERT(0 < refiningID && refiningID < offsets.first.size());
  
  add(offsets.first.at(searchableId), attribute);
}

template<typename T> inline
void Serializer::add(const unsigned offset, const T& attribute) {
  *((T*) (buffer + offset)) = attribute;
}

#endif
