
// $Id: Record.cpp 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

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


#include <string>
#include <vector>

#include <instantsearch/Schema.h>
#include <instantsearch/Record.h>
#include "util/Assert.h"

#include "LocationRecordUtil.h"

#include <iostream> //for testing
#include <sstream>

using std::vector;
using std::string;

namespace srch2
{
namespace instantsearch
{

struct Record::Impl
{
    string primaryKey;
    std::vector<vector<string> > searchableAttributeValues;
    std::vector<string> refiningAttributeValues;
    float boost;
    const Schema *schema;
    std::string inMemoryRecordString;
    char * inMemoryStoredRecord;
    unsigned inMemoryStoredRecordLen;

    //Point : The location lat,lang value of the geo object
    Point point;
    ~Impl() {
    	if (inMemoryStoredRecord) {
    		delete[] inMemoryStoredRecord;
    	}
    }
};

Record::Record(const Schema *schema):impl(new Impl)
{
    impl->schema = schema;
    vector<string> emptyStringVector;
    // first we fill these two vectors with place holders.
    impl->searchableAttributeValues.assign(impl->schema->getNumberOfSearchableAttributes(), emptyStringVector);
    impl->refiningAttributeValues.assign(impl->schema->getNumberOfRefiningAttributes(),"");
    impl->boost = 1;
    impl->primaryKey = "";
    impl->point.x = 0;
    impl->point.y = 0;
    impl->inMemoryStoredRecord = 0;
    impl->inMemoryStoredRecordLen = 0;
}


Record::~Record()
{
    if (impl != NULL) {
        impl->schema = NULL;
        delete impl;
    }
}

bool Record::setSearchableAttributeValue(const string &attributeName,
        const string &attributeValue)
{
    int attributeId = impl->schema->getSearchableAttributeId(attributeName);
    if (attributeId < 0) {
        return false;
    }
    return setSearchableAttributeValue(attributeId, attributeValue);
}

bool Record::setSearchableAttributeValues(const string &attributeName,
		const std::vector<std::string> &attributeValues)
{
    int attributeId = impl->schema->getSearchableAttributeId(attributeName);
    if (attributeId < 0) {
        return false;
    }
    return setSearchableAttributeValues(attributeId, attributeValues);
}


bool Record::setSearchableAttributeValue(const unsigned attributeId,
                    const string &attributeValue)
{
    if (attributeId >= impl->schema->getNumberOfSearchableAttributes()) {
        return false;
    }

    // This function can only be called for a single-valued attribute
    ASSERT(impl->schema->isSearchableAttributeMultiValued(attributeId) == false);

    // For a single-valued attribute, we check searchableAttributeValues[attributeId].size().
    // If it's 0, do the assignment; otherwise, do an assert() and assign it to
    // the 0-th value.
    if (impl->searchableAttributeValues[attributeId].size() == 0) {
    	impl->searchableAttributeValues[attributeId].push_back(attributeValue);
    } else {
    	ASSERT(impl->searchableAttributeValues[attributeId].size() == 1);
    	impl->searchableAttributeValues[attributeId].at(0) = attributeValue;
    }

    return true;
}

bool Record::setSearchableAttributeValues(const unsigned attributeId,
		const std::vector<std::string> &attributeValues)
{
    if (attributeId >= impl->schema->getNumberOfSearchableAttributes()) {
        return false;
    }

    // This function can only be called for a multi-valued attribute
     ASSERT(impl->schema->isSearchableAttributeMultiValued(attributeId) == true);

    impl->searchableAttributeValues[attributeId] = attributeValues;
    return true;
}


bool Record::setRefiningAttributeValue(const std::string &attributeName,
            const std::string &attributeValue){
    int attributeId = impl->schema->getRefiningAttributeId(attributeName);
    if (attributeId < 0) {
        return false;
    }
    return setRefiningAttributeValue(attributeId, attributeValue);
}




bool Record::setRefiningAttributeValue(const unsigned attributeId,
                const std::string &attributeValue){
    if (attributeId >= impl->schema->getNumberOfRefiningAttributes()) {
        return false;
    }

    impl->refiningAttributeValues[attributeId] = attributeValue;
    return true;
}


void Record::getSearchableAttributeValue(const unsigned attributeId, string & attributeValue) const
{
    if (attributeId >= impl->schema->getNumberOfSearchableAttributes())
    {
        return;
    }
    if(impl->searchableAttributeValues[attributeId].empty()){
    	return;
    }
    attributeValue = "";
    for(vector<string>::iterator attributeValueIter = impl->searchableAttributeValues[attributeId].begin() ;
    		attributeValueIter != impl->searchableAttributeValues[attributeId].end() ; ++attributeValueIter){
    	if(attributeValueIter == impl->searchableAttributeValues[attributeId].begin()){
    		attributeValue += MULTI_VALUED_ATTRIBUTES_VALUE_DELIMITER;
    	}
    	attributeValue += *attributeValueIter;
    }
}

void Record::getSearchableAttributeValues(const unsigned attributeId , std::vector<string> & attributeStringValues) const {
    if (attributeId >= impl->schema->getNumberOfSearchableAttributes())
    {
        return;
    }
    attributeStringValues = impl->searchableAttributeValues[attributeId];
    return;
}


std::string *Record::getRefiningAttributeValue(const unsigned attributeId) const
{
    if (attributeId >= impl->schema->getNumberOfRefiningAttributes())
    {
        return NULL;
    }
    return &impl->refiningAttributeValues[attributeId];
}

// add the primary key value
void Record::setPrimaryKey(const string &primaryKey)
{
    impl->primaryKey = primaryKey;
    int primaryKeyAttributeId = impl->schema->getSearchableAttributeId(*(impl->schema->getPrimaryKey()));

    ///if primaryKeyAttributeId is -1, there is no primaryKey in AttributeMap of schema,i.e primaryKey was not made searchable.
    if (primaryKeyAttributeId >= 0)
    {
        //int to string
        //std::string s;
        //std::stringstream out;
        //out << primaryKey;
        //std::string primaryKeyStringValue = out.str();
        this->setSearchableAttributeValue(primaryKeyAttributeId, impl->primaryKey);
    }
}

// add the primary key value
void Record::setPrimaryKey(const unsigned &primaryKey)
{
    std::stringstream pkey_string;
    pkey_string << primaryKey;
    this->setPrimaryKey(pkey_string.str());
}

// get the primary key value
const string& Record::getPrimaryKey() const
{
    return impl->primaryKey;
}


void Record::setShardingKey(unsigned shardingKey){
    //TODO : we should probably keep shardingKey in a new class member
}
const unsigned Record::getShardingKey(){
    std::stringstream pkey_string;
    pkey_string << impl->primaryKey;
    unsigned shardingKey = 0;
    pkey_string >> shardingKey;
    return shardingKey;
}

// set/get the boost of this record (0 - 100)
float Record::getRecordBoost() const
{
    return impl->boost;
}

// required by Analyzer Tokenize
const Schema* Record::getSchema() const
{
    return impl->schema;
}
void Record::setInMemoryData(const void * ptr, unsigned len) {
	impl->inMemoryStoredRecord = new char[len];
	memcpy(impl->inMemoryStoredRecord, ptr, len);
	impl->inMemoryStoredRecordLen = len;
}

//void Record::setInMemoryData(const std::string &inMemoryRecordString)
//{
//    impl->inMemoryRecordString = inMemoryRecordString;
//}

StoredRecordBuffer Record::getInMemoryData() const
{
	StoredRecordBuffer r = StoredRecordBuffer(impl->inMemoryStoredRecord, impl->inMemoryStoredRecordLen);
    return r;
}

void Record::setRecordBoost(const float recordBoost)
{
    impl->boost = recordBoost;
}

/**
 * Sets the location attribute value of the record.
 */
void Record::setLocationAttributeValue(const double &latitude, const double &longitude)
{
    impl->point.x = latitude;
    impl->point.y = longitude;
}

/**
 * Gets the location attribute value of the record.
 */
std::pair<double,double> Record::getLocationAttributeValue() const
{
    return std::make_pair<double,double>(impl->point.x, impl->point.y);
}

// clear the content of the record EXCEPT SCHEMA
void Record::clear()
{
    // We fill these two vectors with place holders to have the correct size.
    vector<string> emptyVector;
    impl->searchableAttributeValues.assign(impl->schema->getNumberOfSearchableAttributes(),emptyVector);
    impl->refiningAttributeValues.assign(impl->schema->getNumberOfRefiningAttributes(), "");
    impl->boost = 1;
    impl->primaryKey = "";
    impl->inMemoryRecordString = "";
    impl->point.x = 0;
    impl->point.y = 0;
    if (impl->inMemoryStoredRecord) {
    	delete[] impl->inMemoryStoredRecord;
    	impl->inMemoryStoredRecord = NULL;
    }
}

void Record::disownInMemoryData() {
	// forget about it.
	impl->inMemoryStoredRecord = NULL;
}

}}

