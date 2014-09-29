//Author: RJ Atwal

#include "RecordSerializer.h"
#include "Assert.h"
#include <cstring>
#include "instantsearch/Constants.h"

using namespace srch2::util;

inline offset_type nextOffset(const offset_type& offset, int amount) {
  return offset + sizeof(offset_type) * amount;
}

inline offset_type nextOffset(const offset_type& offset) {
  return nextOffset(offset, 1);
}
inline offset_type incrementOffset(offset_type& offset, int amount) {
  return offset += sizeof(offset_type) * amount;
}

inline offset_type  incrementOffset(offset_type& offset) {
  return incrementOffset(offset, 1);
}

void RecordSerializer::expandBuffer(size_t needAddition) {
  //rounds default size of the nearest allocation page size
  size_t newBufferSize = allocator.round(maxOffsetOfBuffer +  needAddition);
  char* oldBuffer = buffer;
  buffer = allocator.allocate(newBufferSize);

  std::memcpy(buffer, oldBuffer, maxOffsetOfBuffer);
  
  allocator.deallocate(oldBuffer, maxOffsetOfBuffer);
  
  maxOffsetOfBuffer = newBufferSize;
}

inline offset_type dereferenceOffset(char *buffer, offset_type offset) {
  return *((offset_type*) (buffer + offset));
}

bool RecordSerializer::repositionBuffer(offset_type bufferOffset,
    offset_type offsetToWriteAt, offset_type length) {
  if(lastOffsetOfWrittenBuffer + length > maxOffsetOfBuffer) {
    return true;
  }
  
  offset_type currentOffset;

  //use memmove instead of memcpy since region likely overlaps
  std::memmove(buffer + offsetToWriteAt + length, 
      buffer + offsetToWriteAt, lastOffsetOfWrittenBuffer - offsetToWriteAt);

  incrementOffset(bufferOffset, 2);

  while(bufferOffset != fixedSizedOffset) {
    if((currentOffset = dereferenceOffset(buffer, bufferOffset))) {
      add(bufferOffset, currentOffset + length);
    }
    incrementOffset(bufferOffset);
  }

  return false;
}

inline offset_type
RecordSerializer::calculateVariableLengthOffset(variable_length_types, 
    offset_type offset) {
  //find out if we have written buffer out of place, by seeing if
  //a later element is already written
  do { 
    if(dereferenceOffset(buffer, offset)) {
      return dereferenceOffset(buffer, offset);
    }
  } while(incrementOffset(offset) != fixedSizedOffset);

  return lastOffsetOfWrittenBuffer;
}

template<>
void RecordSerializer::add<std::string>(
    const offset_type offset, const std::string& attribute) {
  offset_type offsetToWriteStringAt;
  //writes out length
  
  //loop till we have enough space to add string
  while(true) {
    //calculates the offset which given string should be written at
    offsetToWriteStringAt =
      calculateVariableLengthOffset(VARIABLE_LENGTH_TYPE_STRING, offset);

    //if buffer not big enough try again
    if(lastOffsetOfWrittenBuffer + attribute.length() > maxOffsetOfBuffer) {
      expandBuffer(attribute.length());
      continue;
    }

    if(offsetToWriteStringAt != lastOffsetOfWrittenBuffer) {
     //repositionBuffer() returns true if Buffer will Overflow
     if(repositionBuffer(offset, offsetToWriteStringAt, attribute.length())) {
        return;
     }
    }

    //write out offset
    add(offset, offsetToWriteStringAt);
    //explicit cast needed since attribute.length() is size_t which can
    //be unsigned long
    add(nextOffset(offset), 
        (offset_type) (offsetToWriteStringAt + attribute.length()));

    lastOffsetOfWrittenBuffer += attribute.length();

    std::memcpy(buffer+offsetToWriteStringAt, attribute.data(), 
         attribute.length());

    return;
  }
}

RecordSerializerBuffer RecordSerializer::serialize() {
  char* rtn = buffer;
  buffer = allocator.allocate(maxOffsetOfBuffer);
  return RecordSerializerBuffer(rtn, lastOffsetOfWrittenBuffer);
}

RecordSerializer& RecordSerializer::nextRecord() {
  std::memset(buffer, 0x0, fixedSizedOffset); 
  lastOffsetOfWrittenBuffer = fixedSizedOffset;
  return *this;
}

inline
void categorizeSearchableAttributes(const int numberOfSearchableAttributes,
    srch2::instantsearch::Schema& schema, 
    //categories
    std::vector<int>& searchableStrings) {
  
  // add IDs of all searchable attributes
  for(int i = 0; i < numberOfSearchableAttributes; ++i) {
    searchableStrings.push_back(i);
  }

  // TODO: look at searchableMultivalued case
}

inline void categorizeRefiningAttributes(const int numberOfRefiningAttributes,
    srch2::instantsearch::Schema& schema,
    //categories
    std::vector<int>& refiningInt, std::vector<int>& refiningLong,
    std::vector<int>& refiningFloat, std::vector<int>& refiningDouble, std::vector<int>& refiningTimes,
    std::vector<int>& refiningMultiValued, std::vector<int>& refiningTexts) {

  //adds IDS of refining attributes
  for(int i = 0; i < numberOfRefiningAttributes; ++i) {
    if(schema.isRefiningAttributeMultiValued(i)) {
      refiningMultiValued.push_back(i);
      break;
    }
        switch (schema.getTypeOfRefiningAttribute(i)) {
        case srch2::instantsearch::ATTRIBUTE_TYPE_INT:
            refiningInt.push_back(i);
            break;
        case srch2::instantsearch::ATTRIBUTE_TYPE_LONG:
            refiningLong.push_back(i);
            break;
        case srch2::instantsearch::ATTRIBUTE_TYPE_FLOAT:
            refiningFloat.push_back(i);
            break;
        case srch2::instantsearch::ATTRIBUTE_TYPE_DOUBLE:
            refiningDouble.push_back(i);
            break;
        case srch2::instantsearch::ATTRIBUTE_TYPE_TEXT:
            refiningTexts.push_back(i);
            break;
        case srch2::instantsearch::ATTRIBUTE_TYPE_TIME:
            refiningTimes.push_back(i);
            break;
        default:
        //should not enter this case
        //ASSERT(false);
        break;
    }
  }
}

void initializeOffsetArray(const std::vector<int>& positionsInOffsetArray, 
    std::vector<offset_type>& types, offset_type& offset,
    offset_type increment) {

  for(std::vector<int>::const_iterator 
      attributeId(positionsInOffsetArray.begin()); 
      attributeId != positionsInOffsetArray.end(); ++attributeId) {
    types[*attributeId] = offset;
    offset += increment;
  }
}

std::pair< std::vector<offset_type>, std::vector<offset_type> > 
initAttributeOffsetArray(srch2::instantsearch::Schema& schema, 
    offset_type& offset, offset_type& variableLengthOffsetStart) {
  const int numberOfSearchable = schema.getNumberOfSearchableAttributes();
  const int numberOfRefining = schema.getNumberOfRefiningAttributes();

  // harvest IDs of all attributes
  std::vector<int> refiningTexts, refiningInt, refiningLong,
    refiningFloat, refiningDouble, refiningTimes, refiningMultiValued,
    searchableStrings;

  categorizeSearchableAttributes(numberOfSearchable, schema, 
      searchableStrings);


  categorizeRefiningAttributes(numberOfRefining, schema,
    refiningInt, refiningLong, refiningFloat, refiningDouble, refiningTimes,
    refiningMultiValued, refiningTexts);

  // creates offset array 
  std::vector<offset_type> searchableOffsets = 
    std::vector<offset_type>(numberOfSearchable);
  std::vector<offset_type> refiningOffsets = 
    std::vector<offset_type>(numberOfRefining);

  // sets offsets to find attributes based off their id 
  offset = 0;
  initializeOffsetArray(refiningInt, refiningOffsets, offset,
      sizeof(int));
  initializeOffsetArray(refiningLong, refiningOffsets, offset,
        sizeof(long));
  initializeOffsetArray(refiningFloat, refiningOffsets, offset,
          sizeof(float));
  initializeOffsetArray(refiningDouble, refiningOffsets, offset,
          sizeof(double));
  initializeOffsetArray(refiningTimes, refiningOffsets, offset,
      sizeof(long));

  variableLengthOffsetStart = offset;

    initializeOffsetArray(refiningTexts, refiningOffsets, offset,
        sizeof(offset_type));
    initializeOffsetArray(refiningMultiValued, refiningOffsets, offset,
        sizeof(offset_type));
    initializeOffsetArray(searchableStrings, searchableOffsets, offset,
        sizeof(offset_type));

  //have variable sized Attributes
  if(refiningTexts.size() || searchableStrings.size() ||
      refiningMultiValued.size()) {
    incrementOffset(offset);
  }

  return std::pair< std::vector<offset_type>, 
         std::vector<offset_type> >(searchableOffsets,
      refiningOffsets);
}

//              variableLengthOffsetStart
//                           ^       _____________
//                           |      |             |
//   |______________________||____________||______V___________________|
//     int,date(long),float     offsets       strings/variable length
//   |____________________________________|
//                Fixed Length            |
//                                        V
//                                 fixedSizedOffset
//
// offsets() creates a pair of arrays with offset at each index to its 
// associated Attribute's offset in the returned serialized buffer
//   It uses maxOffsetBuffer and lastOffsetOfWrittenBuffer as temporary
//   holders to fill in the const members: fixedSizedOffset,
//   variableLengthOffsetStart
//
RecordSerializer::RecordSerializer(srch2::instantsearch::Schema& schema, 
    Alloc& allocator) : allocator(allocator), schema(schema),
  maxOffsetOfBuffer(0), lastOffsetOfWrittenBuffer(0),
  offsets(initAttributeOffsetArray(schema, maxOffsetOfBuffer,
        lastOffsetOfWrittenBuffer)),
  fixedSizedOffset(maxOffsetOfBuffer),
  variableLengthOffsetStart(lastOffsetOfWrittenBuffer) {

  lastOffsetOfWrittenBuffer = fixedSizedOffset;
  //rounds default size of the nearest allocation page size
  maxOffsetOfBuffer = allocator.round(fixedSizedOffset + 
      DEFAULT_VARIBLE_ATTRIBUTE_LENGTH * (offsets.first.size()));
  buffer = allocator.allocate(maxOffsetOfBuffer);
  std::memset(buffer, 0x0, fixedSizedOffset); 
}

RecordSerializer::RecordSerializer(srch2::instantsearch::Schema& schema) : 
  allocator(defaultAllocator), schema(schema),
  maxOffsetOfBuffer(0), lastOffsetOfWrittenBuffer(0),
  offsets(initAttributeOffsetArray(schema, maxOffsetOfBuffer,
        lastOffsetOfWrittenBuffer)),
  fixedSizedOffset(maxOffsetOfBuffer),
  variableLengthOffsetStart(lastOffsetOfWrittenBuffer) {

  lastOffsetOfWrittenBuffer = fixedSizedOffset;
  //rounds default size of the nearest allocation page size
  maxOffsetOfBuffer = allocator.round(fixedSizedOffset + 
      DEFAULT_VARIBLE_ATTRIBUTE_LENGTH * (offsets.first.size()));
  buffer = allocator.allocate(maxOffsetOfBuffer);
  std::memset(buffer, 0x0, fixedSizedOffset); 
}
