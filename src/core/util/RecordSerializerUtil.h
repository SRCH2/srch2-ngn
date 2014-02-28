/*
 * RecordSerializerUtil.h
 *
 *  Created on: Feb 28, 2014
 *      Author: srch2
 */

#ifndef __RECORDSERIALIZERUTIL_H__
#define __RECORDSERIALIZERUTIL_H__

#include <string>
#include <instantsearch/schema.h>
#include <instantsearch/Record.h>
#include "util/RecordSerializer.h"
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
};

} /* namespace util */
} /* namespace srch2 */
#endif /* __RECORDSERIALIZERUTIL_H__ */
