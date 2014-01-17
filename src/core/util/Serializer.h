#ifndef __SERIALIZER_H__
#define __SERIALIZER_H__

//
//  This class is reponsible for create a compact representation of a given set
//  of inputs, which all adhere to a given schema.
//
//    So let the input Schema be
//               0 -> refiningInt: "reviews"
//               1 -> refiningString-Multivalued: "category"
//               2 -> refiningFloat: "averageNumberOfStars"
//               3 -> refiningDate: "lastReviewed"
//               4 -> refiningString: "city"
//               0 -> searchableString: "placeName"
//               1 -> searchableString: "address"
//               2 -> searchableString: "description
//               5 -> refiningInt: "events"
//
//     Class would construct an "imaginary buffer" in the following format
//
//         refiningInts refiningDate refingFloats refiningString 
//         multivalued  searchableString
//
//
//      so the following example would be
//
//         reviews [4 bytes]  events [4 bytes]  lastReviewed [8 bytes] 
//         averageNumberOfStars [4 bytes]    city [4 bytes]
//         category [4 bytes]   placeName [4] bytes   address [4 bytes]
//         descripton [4 bytes]
//         
//       or in by ids :   
//                        0 5 3 2 4 1 0 1 2 
//
//        and the variable 'offsets' = the following pair
//
//                   offsets.first  = [ 28 32 36]  the searchable attributes
//                   offsets.second = [ 0 24 16 8 20 4]
//
//
//         so if the user input the following records
//
//             { 2343, 20, 12/20/13, 3.44, "Los Angles", "Food", 
//               "Diplomate Cafe", , "yum" }
//
//
//          through the following commands: 
//                  Serializer serializerAttribute(yelpSchema);
//
//                                                       (epoch value)
//                  serializer.addRefiningAttribute('3', 1387507479); 
//                  serializer.addRefiningAttribute('0', 2343); 
//                  serializer.addRefiningAttribute('1', "Food");
//                  serializer.addRefiningAttribute('5', 20);
//                  serializer.addRefiningAttribute('4, "Los Angles");
//                  serializer.addRefiningAttribute('2',3.44);
//                  serializer.addSearchableAttribute('0', "Diplomate Cafe");
//                  serializer.addSearchableAttribute('2', "yum");
//
//
//        Then the .serialize() method will return a [temporary] buffer
//
//  offset1 records  events   last    rating  city  cat'g name addr desc end
//     [     2343     20   1387507479  3.44    40    50    54   68   68   71 
//
//                  offset40    offset50   offset54      offset68  offset71
//                Los Angles      Food   Diplomate Cafe   yum         ]  
//
//
//     * Remember to call nextRecord to serialize the next record this will
//       destroy the current buffer though

#include "Allocator.h"
#include "instantsearch/Schema.h"
#include <vector>

#define DEFAULT_VARIBLE_ATTRIBUTE_LENGTH 64

using namespace srch2::instantsearch;

#include "Assert.h"

typedef unsigned offset_type;

enum variable_length_types {
  VARIABLE_LENGTH_TYPE_STRING 
};

class Serializer {
 public:
  typedef SingleBufferAllocator Alloc;
 private:
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

  Serializer(srch2::instantsearch::Schema&, Alloc&);

  template<typename T>
  void addRefiningAttribute(const int, const T&);

  template<typename T>
  void addSearchableAttribute(const int, const T&);

  offset_type getSearchableOffset(const unsigned);
  offset_type getRefiningOffset(const unsigned);

  void* serialize();

  //Cleans up after previous serialize call
  Serializer& nextRecord();
};

template <> void
Serializer::add<std::string>(const offset_type, const std::string&);

inline
offset_type Serializer::getSearchableOffset(const unsigned searchableId) {
  ASSERT(0 <= searchableId && searchableId < offsets.first.size());
  return offsets.first.at(searchableId);
}

inline
offset_type Serializer::getRefiningOffset(const unsigned refiningId) {
  ASSERT(0 <= refiningId && refiningId < offsets.second.size());
  return offsets.second.at(refiningId);
}

template<typename T> inline
void Serializer::addRefiningAttribute(const int refiningId, 
    const T& attribute) {
  ASSERT(0 <= refiningId && refiningId < offsets.second.size());

  add(offsets.second.at(refiningId), attribute);
}

template<typename T> inline
void Serializer::addSearchableAttribute(const int searchableId,
    const T& attribute) {
  ASSERT(0 <= searchableId && searchableId < offsets.first.size());
  
  add(offsets.first.at(searchableId), attribute);
}

template<typename T> inline
void Serializer::add(const unsigned offset, const T& attribute) {
  *((T*) (buffer + offset)) = attribute;
}

#endif
