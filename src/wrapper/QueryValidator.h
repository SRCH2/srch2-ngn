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
