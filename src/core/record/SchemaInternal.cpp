
// $Id: SchemaInternal.cpp 3441 2013-06-11 20:30:12Z sinakeshtkar $

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

#include "SchemaInternal.h"
#include "util/Assert.h"
#include <map>
#include <vector>
#include <numeric>
#include <string>

using std::vector;
using std::string;
using std::map;
using std::pair;

namespace bimaple
{
namespace instantsearch
{

SchemaInternal::SchemaInternal(bimaple::instantsearch::IndexType indexType, bimaple::instantsearch::PositionIndexType positionIndexType)
{
    this->indexType = indexType;
    this->positionIndexType = positionIndexType;
    this->scoringExpressionString = "1"; // DEFAULT SCORE
}

SchemaInternal::SchemaInternal(const SchemaInternal &schemaInternal)
{
    this->primaryKey = schemaInternal.primaryKey;
    this->searchableAttributeNameToId = schemaInternal.searchableAttributeNameToId;
    this->searchableAttributeBoostVector = schemaInternal.searchableAttributeBoostVector;
    this->sortableAttributeNameToId = schemaInternal.sortableAttributeNameToId;
    this->sortableAttributeTypeVector = schemaInternal.sortableAttributeTypeVector;
    this->sortableAttributeDefaultValueVector = schemaInternal.sortableAttributeDefaultValueVector;
    this->scoringExpressionString = schemaInternal.scoringExpressionString;
    this->indexType = schemaInternal.indexType;
    this->positionIndexType = schemaInternal.positionIndexType;
    this->commited = schemaInternal.commited;
}

bimaple::instantsearch::IndexType SchemaInternal::getIndexType() const
{
    return this->indexType;
}

bimaple::instantsearch::PositionIndexType SchemaInternal::getPositionIndexType() const
{
    return this->positionIndexType;
}

unsigned SchemaInternal::getBoostSumOfSearchableAttributes() const
{
    return std::accumulate( this->searchableAttributeBoostVector.begin(), this->searchableAttributeBoostVector.end(), 0 );
}

int SchemaInternal::getSearchableAttributeId(const string &attributeName) const
{
    map<string, unsigned >::const_iterator iter = this->searchableAttributeNameToId.find(attributeName.c_str());
    if (iter != this->searchableAttributeNameToId.end()) {
        return iter->second;
    }
    else {
        return -1;
    }
}

int SchemaInternal::getSortableAttributeId(const std::string &attributeName) const
{
    map<string, unsigned >::const_iterator iter = this->sortableAttributeNameToId.find(attributeName.c_str());
    if (iter != this->sortableAttributeNameToId.end()) {
        return iter->second;
    }
    else {
        return -1;
    }
}

SchemaInternal::~SchemaInternal()
{

}

void SchemaInternal::setPrimaryKey(const string &primaryKey)
{
    this->primaryKey = primaryKey;
}

const std::string* SchemaInternal::getPrimaryKey() const
{
    return &this->primaryKey;
}

int SchemaInternal::setSearchableAttribute(const string &attributeName,
        unsigned attributeBoost)
{
    //ASSERT (this->boostVector.size() <= 255);

    if( this->searchableAttributeNameToId.size() > 255)
    {
        return -1;
    }

    if (attributeBoost < 1 || attributeBoost > 100)
    {
        attributeBoost = 100;
    }

    map<string, unsigned >::iterator iter;
    iter = this->searchableAttributeNameToId.find(attributeName);
    if (iter != this->searchableAttributeNameToId.end()) {
        this->searchableAttributeBoostVector[iter->second] = attributeBoost;
    }
    else {
    	int searchAttributeMapSize= this->searchableAttributeNameToId.size();
        this->searchableAttributeNameToId[attributeName] = searchAttributeMapSize;
        this->searchableAttributeBoostVector.push_back(attributeBoost);
    }
    return this->searchableAttributeNameToId.size() - 1;
}

int SchemaInternal::setSortableAttribute(const std::string &attributeName, FilterType type, string defaultValue)
{
    if( this->sortableAttributeNameToId.size() > 255)
    {
        return -1;
    }

    map<string, unsigned >::iterator iter;
    iter = this->sortableAttributeNameToId.find(attributeName);
    if (iter != this->sortableAttributeNameToId.end()) {
        this->sortableAttributeDefaultValueVector[iter->second] = defaultValue;
        this->sortableAttributeTypeVector[iter->second] = type;
    }
    else {
        this->sortableAttributeNameToId[attributeName] = this->sortableAttributeNameToId.size() - 1;
        this->sortableAttributeDefaultValueVector.push_back(defaultValue);
        this->sortableAttributeTypeVector.push_back(type);
    }
    return this->sortableAttributeNameToId.size() - 1;
}


const std::string* SchemaInternal::getDefaultValueOfSortableAttribute(const unsigned sortableAttributeNameId) const
{
    if ( sortableAttributeNameId < this->sortableAttributeDefaultValueVector.size())
    {
        return &this->sortableAttributeDefaultValueVector[sortableAttributeNameId];
    }
    else
    {
        return NULL;
    }
}

void SchemaInternal::setScoringExpression(const std::string &scoringExpression)
{
    this->scoringExpressionString = scoringExpression;
}

const std::string SchemaInternal::getScoringExpression() const
{
    return this->scoringExpressionString;
}

/**
 * Does not do the bound checking. Caller must make sure filterAttributeNameId is within bounds.
 */
bimaple::instantsearch::FilterType SchemaInternal::getTypeOfSortableAttribute(const unsigned sortableAttributeNameId) const
{
    //if ( filterAttributeNameId < this->filterAttributeTypeVector.size())
    return this->sortableAttributeTypeVector[sortableAttributeNameId];
}

unsigned SchemaInternal::getBoostOfSearchableAttribute(const unsigned attributeId) const
{
    if ( attributeId < this->searchableAttributeBoostVector.size())
    {
        return this->searchableAttributeBoostVector[attributeId];
    }
    else
    {
        return 0;
    }
}

const std::map<std::string, unsigned>& SchemaInternal::getSearchableAttribute() const
{
	return this->searchableAttributeNameToId;
}

unsigned SchemaInternal::getNumberOfSearchableAttributes() const
{
    return this->searchableAttributeNameToId.size();
}

unsigned SchemaInternal::getNumberOfSortableAttributes() const
{
    return this->sortableAttributeNameToId.size();
}

}}
