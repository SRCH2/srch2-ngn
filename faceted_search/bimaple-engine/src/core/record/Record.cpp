
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
    std::vector<string> searchableAttributeValues;
    std::vector<string> nonSearchableAttributeValues;
    float boost;
    const Schema *schema;
    std::string inMemoryRecordString;

    //Point : The location lat,lang value of the geo object
    Point point;
};

//TODO: check the default boost value and primary key
Record::Record(const Schema *schema):impl(new Impl)
{
    impl->schema = schema;
    impl->searchableAttributeValues.assign(impl->schema->getNumberOfSearchableAttributes(),"");
    impl->nonSearchableAttributeValues.assign(impl->schema->getNumberOfNonSearchableAttributes(),"");
    impl->boost = 1;
    impl->primaryKey = "";
    impl->point.x = 0;
    impl->point.y = 0;
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


bool Record::setSearchableAttributeValue(const unsigned attributeId,
                    const string &attributeValue)
{
    if (attributeId >= impl->schema->getNumberOfSearchableAttributes()) {
        return false;
    }
    impl->searchableAttributeValues[attributeId] = attributeValue;
    return true;
}



bool Record::setNonSearchableAttributeValue(const std::string &attributeName,
            const std::string &attributeValue){
    int attributeId = impl->schema->getNonSearchableAttributeId(attributeName);
    if (attributeId < 0) {
        return false;
    }
    return setNonSearchableAttributeValue(attributeId, attributeValue);
}




bool Record::setNonSearchableAttributeValue(const unsigned attributeId,
                const std::string &attributeValue){
    if (attributeId >= impl->schema->getNumberOfNonSearchableAttributes()) {
        return false;
    }

    impl->nonSearchableAttributeValues[attributeId] = attributeValue;
    return true;
}


std::string *Record::getSearchableAttributeValue(const unsigned attributeId) const
{
    if (attributeId >= impl->schema->getNumberOfSearchableAttributes())
    {
        return NULL;
    }
    return &impl->searchableAttributeValues[attributeId];
}


std::string *Record::getNonSearchableAttributeValue(const unsigned attributeId) const
{
    if (attributeId >= impl->schema->getNumberOfNonSearchableAttributes())
    {
        return NULL;
    }
    return &impl->nonSearchableAttributeValues[attributeId];
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

void Record::setInMemoryData(const std::string &inMemoryRecordString)
{
    impl->inMemoryRecordString = inMemoryRecordString;
}

std::string Record::getInMemoryData() const
{
    return impl->inMemoryRecordString;
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
    impl->searchableAttributeValues.assign(impl->schema->getNumberOfSearchableAttributes(),"");
    impl->nonSearchableAttributeValues.assign(impl->schema->getNumberOfNonSearchableAttributes(), "");
    impl->boost = 1;
    impl->primaryKey = "";
    impl->inMemoryRecordString = "";
    impl->point.x = 0;
    impl->point.y = 0;
}

}}

