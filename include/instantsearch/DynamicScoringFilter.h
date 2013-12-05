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

#include "instantsearch/ResultsPostProcessor.h"

#ifndef __CORE_POSTPROCESSING_DYNAMICSCOREFILTER_H__
#define __CORE_POSTPROCESSING_DYNAMICSCOREFILTER_H__


namespace srch2 {
namespace instantsearch {

struct AttributeBoost {
  unsigned attributeMask;
  unsigned boostFactor;
  unsigned hitCount;
};

struct KeywordBoost {
  float score;
  unsigned attributeMask;
  unsigned id;
};

struct DynamicScoringFilter : ResultsPostProcessorFilter {
  const unsigned numberOfAttributes;
  unsigned boostedAttributeMask;
  AttributeBoost attribute[0];
 	void doFilter(IndexSearcher*, const Query*, QueryResults*, QueryResults*);
  AttributeBoost* getAttributeBoost(unsigned);

  DynamicScoringFilter(unsigned numberOfAttribute);
};


}}
#endif /*__CORE_POSTPROCESSING_DYNAMICSCOREFILTER_H__*/
