#include "instantsearch/Schema.h"
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

void testStringSerialization(Alloc& allocator) {

  testSingleString(allocator);
  testMultipleStrings(allocator);
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
