/*
 * RecordSerializerUtil.h
 *
 *  Created on: Feb 28, 2014
 *      Author: srch2
 */

#ifndef __RECORDSERIALIZERUTIL_H__
#define __RECORDSERIALIZERUTIL_H__

#include <string>
#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include "util/RecordSerializer.h"
#include "instantsearch/TypedValue.h"
#include <vector>

using namespace std;
using namespace srch2::instantsearch;

namespace srch2 {
namespace util {

class RecordSerializerUtil {
public:
	static void populateStoredSchema(Schema* storedSchema, const Schema *schema);
	static void convertCompactToJSONString(Schema * storedAttrSchema, StoredRecordBuffer buffer,
			const string& externalRecordId, string& jsonBuffer);
	static void convertCompactToJSONString(Schema * storedAttrSchema, StoredRecordBuffer buffer,
			const string& externalRecordId, string& jsonBuffer, const vector<string>* attrToReturn);
private:
	static void cleanAndAppendToBuffer(const string& in, string& out);
	RecordSerializerUtil();
	virtual ~RecordSerializerUtil();

public:

    // gets values of attributes in iters in Score objects. iters must be ascending.
    static void getBatchOfAttributes(
            const std::vector<string> & nonSearchableAttributeIndexs,
            const Schema * schema, const Byte * data, std::vector<TypedValue> * scores);

private:

    static FilterType getAttributeType(const string& name, const Schema * schema) ;

    static unsigned convertByteArrayToUnsigned(unsigned startOffset, const Byte * data) ;

    static float convertByteArrayToFloat(unsigned startOffset, const Byte * data) ;

    static long convertByteArrayToLong(unsigned startOffset, const Byte * data) ;

    static void convertByteArrayToTypedValue(const string& name, bool isMultiValued, const FilterType& type,
    		RecordSerializer& recSerializer, const Byte * data, TypedValue * result) ;

};

} /* namespace util */
} /* namespace srch2 */
#endif /* __RECORDSERIALIZERUTIL_H__ */
