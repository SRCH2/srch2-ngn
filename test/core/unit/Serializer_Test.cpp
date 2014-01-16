#include "instantsearch/Schema.h"
#include "instantsearch/Constants.h"
#include "util/Serializer.h"
#include <cstdio>
#include <cstring>
#include <cassert>

typedef DefaultBufferAllocator Alloc;

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

void testStringSerialization(Alloc& allocator) {
  testSingleString(allocator);
  testMultipleStrings(allocator);
  testMultipleStringsOutOfOrder(allocator);
  testIntAndString(allocator);
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
