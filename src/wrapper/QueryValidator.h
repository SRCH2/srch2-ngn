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


#ifndef _WRAPPER_QUERYVALIDATOR_H_
#define _WRAPPER_QUERYVALIDATOR_H_

#include "ParsedParameterContainer.h"
#include <instantsearch/Schema.h>

namespace srch2{

namespace httpwrapper{

using srch2::instantsearch::Schema;

class QueryValidator
{

public:

	QueryValidator(const Schema & schema ,ParsedParameterContainer * paramContainer){
		this->paramContainer = paramContainer;
		this->schema = schema;
	}


	// this function goes through the sumary and based on that validates the query.
	bool validate(){


		/*
		 * things that validator needs to validate :
		 * 1. existence of fields
		 * 2. is a field sortable or not
		 * 3. is a field searchable or not
		 * 4. the value of "numberOfResultsLimit"
		 * 5. is a facet field nonSearchable
		 *
		 */
	}

private:
	ParsedParameterContainer * paramContainer;
	const Schema & schema;



};

}
}


#endif // _WRAPPER_QUERYVALIDATOR_H_
