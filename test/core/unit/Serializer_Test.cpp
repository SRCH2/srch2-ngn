#include "instantsearch/Schema.h"
#include "instantsearch/Constants.h"
#include "util/Serializer.h"
#include <cstdio>
#include <cstring>
#include <cassert>

typedef SingleBufferAllocator Alloc;

void compare(char const *expected, char const *actual, size_t num) {
  assert(!memcmp(expected, actual, num));
}

static char testSingleStringExpected[] = {
  0x8, 0x0, 0x0, 0x0,
  0xd, 0x0, 0x0, 0x0,
  'a', 'p', 'p', 'l', 'e' };

void testSingleString(Alloc &alloc) {
  srch2::instantsearch::Schema *schema = 
    srch2::instantsearch::Schema::create(srch2::instantsearch::DefaultIndex);

  int NameID = schema->setSearchableAttribute("name");

  Serializer s(*schema, alloc);

  s.addSearchableAttribute(NameID, std::string("apple"));

  char *buffer = (char*) s.serialize();

  compare(testSingleStringExpected, buffer, 13);
}

static char testMultipleStringsExpected[] = {
  0xc, 0x0, 0x0, 0x0,
  0x11, 0x0, 0x0, 0x0,
  0x14, 0x0, 0x0, 0x0,
  'a', 'p', 'p', 'l', 'e',
  'c', ' ', '+' };

void testMultipleStrings(Alloc& alloc) {
  srch2::instantsearch::Schema *schema = 
    srch2::instantsearch::Schema::create(srch2::instantsearch::DefaultIndex);

  int NameID = schema->setSearchableAttribute("name");
  int addressID = schema->setSearchableAttribute("address");

  Serializer s(*schema, alloc);

  s.addSearchableAttribute(NameID, std::string("apple"));
  s.addSearchableAttribute(addressID, std::string("c +"));

  char *buffer = (char*) s.serialize();

  compare(testMultipleStringsExpected, buffer, 20);
}

static char testMultipleStringsOutofOrderExpected[] = {
  0xc, 0x0, 0x0, 0x0,
  0xf, 0x0, 0x0, 0x0,
  0x14, 0x0, 0x0, 0x0,
  'c', ' ', '+',
  'a', 'p', 'p', 'l', 'e'};

void testMultipleStringsOutOfOrder(Alloc& alloc) {
  srch2::instantsearch::Schema *schema = 
    srch2::instantsearch::Schema::create(srch2::instantsearch::DefaultIndex);

  int addressID = schema->setSearchableAttribute("address");
  int NameID = schema->setSearchableAttribute("name");

  Serializer s(*schema, alloc);

  s.addSearchableAttribute(NameID, std::string("apple"));
  s.addSearchableAttribute(addressID, std::string("c +"));

  char *buffer = (char*) s.serialize();

  compare(testMultipleStringsOutofOrderExpected, buffer, 20);
}

static char testIntAndStringExpected[] = {
  0x14, 0x0, 0x0, 0x0,
  0xc, 0x0, 0x0, 0x0,
  0x11, 0x0, 0x0, 0x0,
  'h', 'a', 'p', 'p', 'y'};

void testIntAndString(Alloc& alloc) {
  srch2::instantsearch::Schema *schema = 
    srch2::instantsearch::Schema::create(srch2::instantsearch::DefaultIndex);

  int nameID = schema->setSearchableAttribute("name");
  int addressID = 
    schema->setRefiningAttribute("address", 
        srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED, std::string("0"));

  Serializer s(*schema, alloc);

  s.addRefiningAttribute(addressID, 20);
  s.addSearchableAttribute(nameID, std::string("happy"));

  char *buffer = (char*) s.serialize();

  compare(testIntAndStringExpected, buffer, 20);
}

void testReuseBuffer(Alloc& alloc) {
  srch2::instantsearch::Schema *schema = 
    srch2::instantsearch::Schema::create(srch2::instantsearch::DefaultIndex);

  int nameID = schema->setSearchableAttribute("name");
  int addressID = 
    schema->setRefiningAttribute("address", 
        srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED, std::string("0"));

  Serializer s(*schema, alloc);
  
  for(int i=0; i < 5; ++i) {
    s.addRefiningAttribute(addressID, 20);
    s.addSearchableAttribute(nameID, std::string("happy"));
    
    char *buffer = (char*) s.serialize();
    compare(testIntAndStringExpected, buffer, 20);
  }
}

static char testIntAndStringOutOfOrderExpected[] = {
  0x3F, (char) 0xA7, (char) 0xdf, 0x5,
  0x0, 0x2, 0x0, 0x0,
  0x1c, 0x0, 0x0, 0x0,
  0x24, 0x0, 0x0, 0x0,
  0x29, 0x0, 0x0, 0x0,
  0x2d, 0x0, 0x0, 0x0,
  0x33, 0x0, 0x0, 0x0,
  'r', 'e', 'f', 'i', 'n', 'i', 'n', 'g',
  'a', 't', 't', 'i', 'c',
  'o', 'p', 'e', 'n', 
  'p', 'y', 't', 'h', 'o', 'n'};

void testIntAndStringOutOfOrder(Alloc& alloc) {
 srch2::instantsearch::Schema *schema = 
    srch2::instantsearch::Schema::create(srch2::instantsearch::DefaultIndex);

  int str1ID = schema->setSearchableAttribute("str1");
  int int1ID = 
    schema->setRefiningAttribute("int1", 
        srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED, std::string("0"));
  int str2ID = schema->setSearchableAttribute("str2");
  int int2ID = 
    schema->setRefiningAttribute("int2", 
        srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED, std::string("0"));
  int str3ID = schema->setSearchableAttribute("str3");
  int str4ID = 
    schema->setRefiningAttribute("int4", 
        srch2::instantsearch::ATTRIBUTE_TYPE_TEXT, std::string("0"));

  Serializer s(*schema, alloc);
  
  for(int i=0; i < 5; ++i) {
    s.addSearchableAttribute(str3ID, std::string("python"));
    s.addRefiningAttribute(int2ID, 512);
    s.addSearchableAttribute(str1ID, std::string("attic"));
    s.addRefiningAttribute(int1ID, 98543423);
    s.addSearchableAttribute(str2ID, std::string("open"));
    s.addRefiningAttribute(str4ID, std::string("refining"));

    char *buffer = (char*) s.serialize();
    compare(testIntAndStringOutOfOrderExpected, buffer, 39);
  }
}

static char testLongSerializationExpected[] = {
  0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
};

void testLongSerialization(Alloc& alloc) {
 srch2::instantsearch::Schema *schema = 
    srch2::instantsearch::Schema::create(srch2::instantsearch::DefaultIndex);

  int long1ID = 
    schema->setRefiningAttribute("long1", 
        srch2::instantsearch::ATTRIBUTE_TYPE_TIME, std::string("0"));

  Serializer s(*schema, alloc);

  s.addRefiningAttribute(long1ID, (unsigned long) 2);

  char *buffer = (char*) s.serialize();
  compare(testLongSerializationExpected, buffer, 8);
}

void testStringSerialization(Alloc& allocator) {
  testSingleString(allocator);
  testMultipleStrings(allocator);
  testMultipleStringsOutOfOrder(allocator);
  testIntAndString(allocator);
  testReuseBuffer(allocator);
  testIntAndStringOutOfOrder(allocator);
}


int main(int argc, char *argv[]) {
 /*   bool verbose = false;
    if (argc > 1 && strcmp(argv[1], "--verbose") == 0) {
        verbose = true;
    }*/
    Alloc alloc= Alloc();

    testStringSerialization(alloc);

    printf("Serializer unit tests: Passed\n");

    return 0;
}
