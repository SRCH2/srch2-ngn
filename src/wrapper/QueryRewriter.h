// $Id$ 07/11/13 Jamshid


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


#ifndef _WRAPPER_QUERYREQRITER_H_
#define _WRAPPER_QUERYREQRITER_H_

#include "ParsedParameterContainer.h"
#include "instantsearch/Analyzer.h"

using srch2::instantsearch::Analyzer;

namespace srch2{

namespace httpwrapper{


class QueryRewriter
{
public:
	QueryRewriter(const Analyzer & analyzer ,ParsedParameterContainer * paramContainer){
	}

	void rewrite(){
		// go through the summary and call the analyzer on the query if needed.
		// 1. Field filter should be changed from field names to field bit filter
		// 2. if a field facet type is range, if it does not have all the 3 needed pieces, fill them using configuration file
		//    if it's not even in configuration file remove that field from facets and save error message.
	}
};

}
}

#endif // _WRAPPER_QUERYREQRITER_H_

