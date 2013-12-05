#include <iterator>

#ifndef __ATTRIBUTE_ITERATOR_H__
#define  __ATTRIBUTE_ITERATOR_H__

namespace srch2 {
namespace instantsearch {

class AttributeIterator : std::iterator<unsigned, std::forward_iterator_tag> {
  unsigned char bit;
  unsigned place;
  unsigned mask;

  public:
  AttributeIterator(unsigned mask) : bit(__builtin_ffs(mask)), place(bit-1),
    mask(mask) {}

  AttributeIterator& operator++() {
    mask= mask >> bit;
    bit= __builtin_ffs(mask);
    place+= bit;
    
    return *this;
  }

  unsigned operator*() { return place;}

  bool hasNext() { return mask; }
};

}}

#endif /* __ATTRIBUTE_ITERATOR_H__ */
