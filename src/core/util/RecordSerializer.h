#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

//
//  This class creates a compact representation from a given set of inputs,
//  which all adhere to a given schema.
//
//    So let the input Schema be
//               0 -> refiningInt: "numberOfReviews"
//               1 -> refiningString-Multivalued: "genres"
//               2 -> refiningFloat: "averageNumberOfStars"
//               3 -> refiningDate: "lastReviewed"
//               4 -> refiningString: "city"
//               0 -> searchableString: "placeName"
//               1 -> searchableString: "address"
//               2 -> searchableString: "description
//               5 -> refiningInt: "numberOfEvents"
//
//     Class would construct an "imaginary buffer" in the following format
//
//         refiningInts refiningDate refingFloats refiningString 
//         multivalued  searchableString
//
//      with each string position storing an additional offset to its
//      actual position.
//
//      so the following example would be
//
//         numberOfReviews [4 bytes]   numberOfEvents [4 bytes] 
//         lastReviewed [8 bytes]   averageNumberOfStars [4 bytes] 
//         city offset [4 bytes]    genres offset [4 bytes]
//         placeName offset [4] bytes   address offset [4 bytes]
//         description offset [4 bytes]
//         
//       or by ids :   
//                        0 5 3 2 4 1 0 1 2 
//
//        and the variable 'offsets' = the following pair
//
//          offsets.first  = [ 28 32 36]  // offsets of searchable attributes
//          offsets.second = [ 0 24 16 8 20 4] //offsets of refining attributes
//
//
//         so if the user inputs the following record:
//
//             { 2343, 20, 12/20/13, 3.44, "Los Angeles", "Food", 
//               "Diplomate Cafe", , "yum" }
//
//
//          through the following commands: 
//                  RecordSerializer serializer(yelpSchema);
//
//                                                       (epoch value)
//                  serializer.addRefiningAttribute(3, 1387507479); 
//                  serializer.addRefiningAttribute(0, 2343); 
//                  serializer.addRefiningAttribute(1,"Food");
//                  serializer.addRefiningAttribute(5, 20);
//                  serializer.addRefiningAttribute(4 "Los Angeles");
//                  serializer.addRefiningAttribute(2, 3.44);
//                  serializer.addSearchableAttribute(0, "Diplomate Cafe");
//                  serializer.addSearchableAttribute(2, "yum");
//
//
//        Then the .serialize() method will return a temporary buffer:
//
//  offset1 reviews  events   last    rating  city genres name addr desc end
//     [     2343     20   1387507479  3.44    40    51    55   69   69   72 
//
//                  offset40    offset51   offset55      offset69  offset72
//                Los Angles      Food   Diplomate Cafe   yum         ]  
//
//
//     * Remember to call nextRecord to serialize the next record this will
//       destroy the current buffer 

#include "Allocator.h"
#include "instantsearch/Schema.h"
#include <vector>

#define DEFAULT_VARIBLE_ATTRIBUTE_LENGTH 64

using namespace srch2::instantsearch;

#include "Assert.h"

namespace srch2 {
namespace util {

typedef unsigned offset_type;

enum variable_length_types {
  VARIABLE_LENGTH_TYPE_STRING 
};

class RecordSerializer {
 public:
  typedef SingleBufferAllocator Alloc;
 private:
  Alloc defaultAllocator;
  Alloc& allocator;
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

  RecordSerializer(srch2::instantsearch::Schema&, Alloc&);
  RecordSerializer(srch2::instantsearch::Schema&);

  template<typename T>
  void addRefiningAttribute(const int, const T&);

  template<typename T>
  void addSearchableAttribute(const int, const T&);

  offset_type getSearchableOffset(const unsigned);
  offset_type getRefiningOffset(const unsigned);

  void* serialize();

  //Cleans up after previous serialize call
  RecordSerializer& nextRecord();
};

template <> void
RecordSerializer::add<std::string>(const offset_type, const std::string&);

inline offset_type 
RecordSerializer::getSearchableOffset(const unsigned searchableId) {
  ASSERT(0 <= searchableId && searchableId < offsets.first.size());
  return offsets.first.at(searchableId);
}

inline
offset_type RecordSerializer::getRefiningOffset(const unsigned refiningId) {
  ASSERT(0 <= refiningId && refiningId < offsets.second.size());
  return offsets.second.at(refiningId);
}

template<typename T> inline
void RecordSerializer::addRefiningAttribute(const int refiningId, 
    const T& attribute) {
  ASSERT(0 <= refiningId && refiningId < offsets.second.size());

  add(offsets.second.at(refiningId), attribute);
}

template<typename T> inline
void RecordSerializer::addSearchableAttribute(const int searchableId,
    const T& attribute) {
  ASSERT(0 <= searchableId && searchableId < offsets.first.size());
  
  add(offsets.first.at(searchableId), attribute);
}

template<typename T> inline
void RecordSerializer::add(const unsigned offset, const T& attribute) {
  *((T*) (buffer + offset)) = attribute;
}

}
}
#endif
