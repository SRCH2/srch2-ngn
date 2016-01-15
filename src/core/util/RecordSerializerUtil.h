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
/*
 * RecordSerializerUtil.h
 *
 *  Created on: Feb 28, 2014
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

    static int convertByteArrayToInt(unsigned startOffset, const Byte * data) ;

    static long convertByteArrayToLong(unsigned startOffset, const Byte * data) ;

    static float convertByteArrayToFloat(unsigned startOffset, const Byte * data) ;

    static double convertByteArrayToDouble(unsigned startOffset, const Byte * data) ;

    static void convertByteArrayToTypedValue(const string& name, bool isMultiValued, const FilterType& type,
    		RecordSerializer& recSerializer, const Byte * data, TypedValue * result) ;

};

} /* namespace util */
} /* namespace srch2 */
#endif /* __RECORDSERIALIZERUTIL_H__ */
