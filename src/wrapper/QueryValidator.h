/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _WRAPPER_QUERYVALIDATOR_H_
#define _WRAPPER_QUERYVALIDATOR_H_

#include "ParsedParameterContainer.h"
#include <instantsearch/Schema.h>
#include "src/sharding/configuration/ConfigManager.h"
#include "operation/AttributeAccessControl.h"
namespace srch2 {

namespace httpwrapper {

using srch2::instantsearch::Schema;

class QueryValidator {

public:

    QueryValidator(const Schema & schema,
            const CoreInfo_t &indexDataContainerConf,
            ParsedParameterContainer * paramContainer,
            const vector<unsigned> & accessibleSearchAttrs,
            const vector<unsigned> & accessibleRefiningAttrs);

    // this function goes through the summary and based on that validates the query.
    bool validate();

private:
    ParsedParameterContainer * paramContainer;
    const Schema & schema;
    const CoreInfo_t &indexDataContainerConf;
    const vector<unsigned> & accessibleSearchAttrs;
    const vector<unsigned> & accessibleRefiningAttrs;

    /*
     * This function goes over the field names in Filter List and
     * checks to make sure all of them are in the list of searchable attributes. existence
     */
    bool validateExistenceOfAttributesInFieldList();

    bool validateExistenceOfAttributesInQueryFieldBoost();

    bool validateExistenceOfAttributesInSortFiler();

    bool validateExistenceOfAttributesInFacetFiler();

    bool validateFilterQuery();

    bool validateParseTreeBooleanStructure();

    bool validateParseTreeStructureWithRegardToComputability();
    bool isParseSubtreeComputableRecursive(ParseTreeNode * node);


	// Helper function to validate whether searchable field is accessible
	bool isSearchableFieldAccessible(const string& fieldName) const {
		unsigned fieldId = schema.getSearchableAttributeId(fieldName);
		return AttributeAccessControl::isFieldAccessible(fieldId,
				accessibleSearchAttrs, schema.getNonAclRefiningAttrIdsList());
	}

	// Helper function to validate whether refining field is accessible
	bool isRefiningFieldAccessible(const string& fieldName) const {
		unsigned fieldId = schema.getRefiningAttributeId(fieldName);
		return AttributeAccessControl::isFieldAccessible(fieldId,
				accessibleRefiningAttrs, schema.getNonAclRefiningAttrIdsList());
	}
//    // this function validates the value stored in "value" based on the type which is passed to it by "type"
//    // for example, if the string is "123rt" and the type is UNSIGNED it returns false
//    bool validateValueWithType(srch2::instantsearch::FilterType type,
//            string & value);

};

}
}

#endif // _WRAPPER_QUERYVALIDATOR_H_
