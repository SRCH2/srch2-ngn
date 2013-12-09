//$Id: DynamicScoringFilter.h 3456 2013-01-02 RJ $

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

 * Copyright © 2010 SRCH2 Inc. All rights reserved
 */

#ifndef __CORE_POSTPROCESSING_DYNAMICSCOREFILTER_H__
#define __CORE_POSTPROCESSING_DYNAMICSCOREFILTER_H__


#include "instantsearch/ResultsPostProcessor.h"

namespace srch2 {
namespace instantsearch {

struct AttributeBoost {
  // the bitmap mask associated with this attribute. For example, if an
  //   attribute has id 5 then its mask is 0000..010000 
  unsigned attributeMask;
  // the factor by which this attribute is boost. For example, if qf=title^100
  // then title's boostFactor = 100 
  unsigned boostFactor;
  // the number of keywords occuring in a given instance of this attribute.
  // This field is used by ranker to calculate the boost for a given record 
  unsigned hitCount;
};

struct KeywordBoost {
  // the runtime score associated with a given keyword of a given record 
  float score;
  // the attribute mask of where this keyword occurs in a given record
  unsigned attributeMask;
  // the given id of this keyword 
  unsigned id;
};

struct DynamicScoringFilter : ResultsPostProcessorFilter {
  const unsigned numberOfAttributes;
  unsigned boostedAttributeMask;
  // This array has to be sorted by attributeID descending so we can
  // perform binary lookups 
  AttributeBoost *const attributeBoosts;
  void doFilter(IndexSearcher*, const Query*, QueryResults*, QueryResults*);
  AttributeBoost* getAttributeBoost(unsigned);

  DynamicScoringFilter(unsigned numberOfAttribute);
  ~DynamicScoringFilter();
};


}}
#endif /*__CORE_POSTPROCESSING_DYNAMICSCOREFILTER_H__*/
