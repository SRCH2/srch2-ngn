#include "Serializer.h"
#include "Assert.h"
#include <cstring>
#include "instantsearch/Constants.h"

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

void Serializer::expandBuffer(size_t needAddition) {
  //rounds default size of the nearest allocation page size
  size_t newBufferSize = allocator.round(maxOffsetOfBuffer +  needAddition);
  char* newBuffer = allocator.allocate(newBufferSize);

  std::memcpy(buffer, newBuffer, maxOffsetOfBuffer);

  allocator.deallocate(buffer, maxOffsetOfBuffer);
  allocator.destroy(buffer); 
  
  buffer = newBuffer;
  maxOffsetOfBuffer = newBufferSize;
}

inline offset_type dereferenceOffset(char *buffer, offset_type offset) {
  return *((offset_type*) (buffer + offset));
}

bool Serializer::repositionBuffer(offset_type bufferOffset,
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
Serializer::calculateVariableLengthOffset(variable_length_types, 
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
void Serializer::add<std::string>(
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
     //returns true if Buffer will Overflow
     if(repositionBuffer(offset, offsetToWriteStringAt, attribute.length())) {
        return;
     }
    }

    //write out offset
    add(offset, offsetToWriteStringAt);
    //explicted cast needed since attribute.length return size_t which can
    //be unsigned long
    add(nextOffset(offset), 
        (offset_type) (offsetToWriteStringAt + attribute.length()));

    lastOffsetOfWrittenBuffer += attribute.length();

    std::memcpy(buffer+offsetToWriteStringAt, attribute.data(), 
         attribute.length());

    return;
  }
}

void* Serializer::serialize() {
  char* rtn = buffer;
  buffer = allocator.allocate(maxOffsetOfBuffer);
  std::memset(buffer, 0x0, fixedSizedOffset); 
  return rtn;
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
    std::vector<int>& refiningInts,
    std::vector<int>& refiningFloats, std::vector<int>& refiningDates,
    std::vector<int>& refiningMultiValued, std::vector<int>& refiningStrings) {

  //adds IDS of refining attributes
  for(int i = 0; i < numberOfRefiningAttributes; ++i) {
    if(schema.isRefiningAttributeMultiValued(i)) {
      refiningMultiValued.push_back(i);
      break;
    }
    switch(schema.getTypeOfRefiningAttribute(i)) {
      case srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED:
        refiningInts.push_back(i);
        break;
      case  srch2::instantsearch::ATTRIBUTE_TYPE_FLOAT:
        refiningFloats.push_back(i);
        break;
      case srch2::instantsearch::ATTRIBUTE_TYPE_TEXT:
        refiningStrings.push_back(i);
        break;
      case srch2::instantsearch::ATTRIBUTE_TYPE_TIME:
        refiningDates.push_back(i);
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
  std::vector<int> refiningStrings, refiningInts,
    refiningFloats, refiningDates, refiningMultiValued,
    searchableStrings;

  categorizeSearchableAttributes(numberOfSearchable, schema, 
      searchableStrings);


  categorizeRefiningAttributes(numberOfRefining, schema,
    refiningInts, refiningFloats, refiningDates,
    refiningMultiValued, refiningStrings);

  // creates offset array 
  std::vector<offset_type> searchableOffsets = 
    std::vector<offset_type>(numberOfSearchable);
  std::vector<offset_type> refiningOffsets = 
    std::vector<offset_type>(numberOfRefining);

  // sets offsets to find attributes based off their id 
  offset = 0;
  initializeOffsetArray(refiningInts, refiningOffsets, offset, 
      sizeof(offset_type));
  initializeOffsetArray(refiningDates, refiningOffsets, offset, 
      sizeof(long));
  initializeOffsetArray(refiningFloats, refiningOffsets, offset, 
      sizeof(float));

  variableLengthOffsetStart = offset;

  if(refiningStrings.size() != 0) {
    initializeOffsetArray(refiningStrings, refiningOffsets, offset, 
        sizeof(offset_type));
    incrementOffset(offset);
  }
  if(refiningMultiValued.size() != 0) {
    initializeOffsetArray(refiningMultiValued, refiningOffsets, offset, 
        sizeof(offset_type));
    incrementOffset(offset);
  }
  if(searchableStrings.size() != 0) {
    initializeOffsetArray(searchableStrings, searchableOffsets, offset, 
        sizeof(offset_type));
    incrementOffset(offset);
  }

  return std::pair< std::vector<offset_type>, 
         std::vector<offset_type> >(searchableOffsets,
      refiningOffsets);
}

Serializer::Serializer(srch2::instantsearch::Schema& schema, 
    DefaultBufferAllocator& allocator) : allocator(allocator), schema(schema),
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
