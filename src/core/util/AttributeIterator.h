
#ifndef __ATTRIBUTE_ITERATOR_H__
#define  __ATTRIBUTE_ITERATOR_H__

#include <iterator>

namespace srch2 {
namespace instantsearch {

/* Iterators over all the set Attributes in a given attribute mask, ie. if the
   attribute mask 0000..01001001 was given the iterator would return
   0 then 4 then 7, and then subsequent calls to hasNext will return 0 */
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
