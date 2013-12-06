// $Id: AttributeIterator.h 2013-12-01 RJ $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2013 SRCH2 Inc. All rights reserved
 */

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
