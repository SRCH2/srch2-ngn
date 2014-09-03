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
#include "ConfigManager.h"
#include "operation/AccessControl.h"
namespace srch2 {

namespace httpwrapper {

using srch2::instantsearch::Schema;

class QueryValidator {

public:

    QueryValidator(const Schema & schema,
            const CoreInfo_t &indexDataContainerConf,
            ParsedParameterContainer * paramContainer,
            const AttributeAccessControl & attrAcl);

    // this function goes through the summary and based on that validates the query.
    bool validate();

private:
    ParsedParameterContainer * paramContainer;
    const Schema & schema;
    const CoreInfo_t &indexDataContainerConf;
    const AttributeAccessControl& attributeAcl;

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


//    // this function validates the value stored in "value" based on the type which is passed to it by "type"
//    // for example, if the string is "123rt" and the type is UNSIGNED it returns false
//    bool validateValueWithType(srch2::instantsearch::FilterType type,
//            string & value);

};

}
}

#endif // _WRAPPER_QUERYVALIDATOR_H_
