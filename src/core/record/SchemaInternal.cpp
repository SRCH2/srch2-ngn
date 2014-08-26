// $Id: SchemaInternal.cpp 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

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

namespace srch2 {
namespace instantsearch {

SchemaInternal::SchemaInternal(srch2::instantsearch::IndexType indexType,
        srch2::instantsearch::PositionIndexType positionIndexType) {
    this->indexType = indexType;
    this->positionIndexType = positionIndexType;
    this->scoringExpressionString = "1"; // DEFAULT SCORE
    this->refiningAttributeNameToId.clear();
    this->refiningAttributeDefaultValueVector.clear();
    this->refiningAttributeTypeVector.clear();
    this->refiningAttributeIsMultiValuedVector.clear();
    this->supportSwapInEditDistance = true;
    this->nameOfLatitudeAttribute = "";
    this->nameOfLongitudeAttribute = "";
}

SchemaInternal::SchemaInternal(const SchemaInternal &schemaInternal) {
    this->primaryKey = schemaInternal.primaryKey;
    this->searchableAttributeNameToId =
            schemaInternal.searchableAttributeNameToId;
    this->searchableAttributeTypeVector =
            schemaInternal.searchableAttributeTypeVector;
    this->searchableAttributeBoostVector =
            schemaInternal.searchableAttributeBoostVector;
    this->searchableAttributeIsMultiValuedVector =
            schemaInternal.searchableAttributeIsMultiValuedVector;
    this->refiningAttributeNameToId = schemaInternal.refiningAttributeNameToId;
    this->refiningAttributeTypeVector =
            schemaInternal.refiningAttributeTypeVector;
    this->refiningAttributeDefaultValueVector =
            schemaInternal.refiningAttributeDefaultValueVector;
    this->refiningAttributeIsMultiValuedVector =
            schemaInternal.refiningAttributeIsMultiValuedVector;
    this->searchableAttributeHighlightEnabled =
            schemaInternal.searchableAttributeHighlightEnabled;
    this->scoringExpressionString = schemaInternal.scoringExpressionString;
    this->indexType = schemaInternal.indexType;
    this->positionIndexType = schemaInternal.positionIndexType;
    this->commited = schemaInternal.commited;
    this->supportSwapInEditDistance = schemaInternal.supportSwapInEditDistance;

    this->aclRefiningAttrIds =  schemaInternal.aclRefiningAttrIds;
    this->nonAclRefiningAttrIds = schemaInternal.nonAclRefiningAttrIds;
    this->aclSearchableAttrIds =  schemaInternal.aclSearchableAttrIds;
    this->nonAclSearchableAttrIds = schemaInternal.nonAclSearchableAttrIds;

    this->nameOfLatitudeAttribute = schemaInternal.nameOfLatitudeAttribute;
    this->nameOfLongitudeAttribute = schemaInternal.nameOfLongitudeAttribute;
}

srch2::instantsearch::IndexType SchemaInternal::getIndexType() const {
    return this->indexType;
}

srch2::instantsearch::PositionIndexType SchemaInternal::getPositionIndexType() const {
    return this->positionIndexType;
}

unsigned SchemaInternal::getBoostSumOfSearchableAttributes() const {
    return std::accumulate(this->searchableAttributeBoostVector.begin(),
            this->searchableAttributeBoostVector.end(), 0);
}

int SchemaInternal::getSearchableAttributeId(
        const string &attributeName) const {
    map<string, unsigned>::const_iterator iter =
            this->searchableAttributeNameToId.find(attributeName.c_str());
    if (iter != this->searchableAttributeNameToId.end()) {
        return iter->second;
    } else {
        return -1;
    }
}

int SchemaInternal::getRefiningAttributeId(
        const std::string &refiningAttributeName) const {

    map<string, unsigned>::const_iterator iter =
            this->refiningAttributeNameToId.find(refiningAttributeName.c_str());
    if (iter != this->refiningAttributeNameToId.end()) {
        return iter->second;
    } else {
        return -1;
    }
}

SchemaInternal::~SchemaInternal() {

}

void SchemaInternal::setPrimaryKey(const string &primaryKey) {
    this->primaryKey = primaryKey;
}

void SchemaInternal::setNameOfLatitudeAttribute(const string &nameOfLatitudeAttribute){
	this->nameOfLatitudeAttribute = nameOfLatitudeAttribute;
}

void SchemaInternal::setNameOfLongitudeAttribute(const string &nameOfLongitudeAttribute){
	this->nameOfLongitudeAttribute = nameOfLongitudeAttribute;
}

const std::string* SchemaInternal::getPrimaryKey() const {
    return &this->primaryKey;
}

const std::string* SchemaInternal::getNameOfLatituteAttribute() const{
	return &this->nameOfLatitudeAttribute;
}

const std::string* SchemaInternal::getNameOfLongitudeAttribute() const{
	return &this->nameOfLongitudeAttribute;
}

int SchemaInternal::setSearchableAttribute(const string &attributeName,
        unsigned attributeBoost, bool isMultiValued, bool enableHiglight) {

    if (attributeBoost < 1 || attributeBoost > 100) {
        attributeBoost = 100;
    }

    // As of now (08/07/14), we assume each searchable attribute has a TEXT type.
    FilterType type = ATTRIBUTE_TYPE_TEXT;

    map<string, unsigned>::iterator iter;
    iter = this->searchableAttributeNameToId.find(attributeName);
    if (iter != this->searchableAttributeNameToId.end()) {
        this->searchableAttributeTypeVector[iter->second] = type;
        this->searchableAttributeBoostVector[iter->second] = attributeBoost;
        this->searchableAttributeIsMultiValuedVector[iter->second] =
                isMultiValued;
        this->searchableAttributeHighlightEnabled[iter->second] =
                enableHiglight;
        return iter->second;
    } else {
        int searchAttributeMapSize = this->searchableAttributeNameToId.size();
        this->searchableAttributeNameToId[attributeName] =
                searchAttributeMapSize;
        this->searchableAttributeTypeVector.push_back(type);
        this->searchableAttributeBoostVector.push_back(attributeBoost);
        this->searchableAttributeIsMultiValuedVector.push_back(isMultiValued);
        this->searchableAttributeHighlightEnabled.push_back(enableHiglight);
    }
    return this->searchableAttributeNameToId.size() - 1;
}

int SchemaInternal::setRefiningAttribute(const std::string &attributeName,
        FilterType type, const std::string & defaultValue, bool isMultiValued) {

    map<string, unsigned>::iterator iter;
    iter = this->refiningAttributeNameToId.find(attributeName);
    if (iter != this->refiningAttributeNameToId.end()) {
        this->refiningAttributeDefaultValueVector[iter->second] = defaultValue;
        this->refiningAttributeTypeVector[iter->second] = type;
        this->refiningAttributeIsMultiValuedVector[iter->second] =
                isMultiValued;
        return iter->second;
    } else {
        unsigned sizeNonSearchable = this->refiningAttributeNameToId.size();
        this->refiningAttributeNameToId[attributeName] = sizeNonSearchable;

        this->refiningAttributeDefaultValueVector.push_back(defaultValue);
        this->refiningAttributeTypeVector.push_back(type);
        this->refiningAttributeIsMultiValuedVector.push_back(isMultiValued);
    }
    return this->refiningAttributeNameToId.size() - 1;

}

const std::string* SchemaInternal::getDefaultValueOfRefiningAttribute(
        const unsigned nonSearchableAttributeNameId) const {
    if (nonSearchableAttributeNameId
            < this->refiningAttributeDefaultValueVector.size()) {
        return &this->refiningAttributeDefaultValueVector[nonSearchableAttributeNameId];
    } else {
        return NULL;
    }
    return NULL;
}

void SchemaInternal::setSupportSwapInEditDistance(
        const bool supportSwapInEditDistance) {
    this->supportSwapInEditDistance = supportSwapInEditDistance;
}

bool SchemaInternal::getSupportSwapInEditDistance() const {
    return supportSwapInEditDistance;
}

void SchemaInternal::setScoringExpression(
        const std::string &scoringExpression) {
    this->scoringExpressionString = scoringExpression;
}

const std::string SchemaInternal::getScoringExpression() const {
    return this->scoringExpressionString;
}

FilterType SchemaInternal::getTypeOfRefiningAttribute(
        const unsigned refiningAttributeNameId) const {

    if (refiningAttributeNameId >= this->refiningAttributeTypeVector.size()) {
        ASSERT(false);
        return srch2::instantsearch::ATTRIBUTE_TYPE_TEXT;
    }
    return this->refiningAttributeTypeVector[refiningAttributeNameId];

}

FilterType SchemaInternal::getTypeOfSearchableAttribute(
        const unsigned searchableAttributeNameId) const {

    if (searchableAttributeNameId
            >= this->searchableAttributeTypeVector.size()) {
        ASSERT(false);
        return srch2::instantsearch::ATTRIBUTE_TYPE_TEXT;
    }
    return this->searchableAttributeTypeVector[searchableAttributeNameId];
}

const std::map<std::string, unsigned> * SchemaInternal::getRefiningAttributes() const {
    return &this->refiningAttributeNameToId;
}

bool SchemaInternal::isRefiningAttributeMultiValued(
        const unsigned refiningAttributeNameId) const {
    if (refiningAttributeNameId
            >= this->refiningAttributeIsMultiValuedVector.size()) {
        ASSERT(false);
        return false;
    }
    return this->refiningAttributeIsMultiValuedVector[refiningAttributeNameId];
}

unsigned SchemaInternal::getBoostOfSearchableAttribute(
        const unsigned attributeId) const {
    if (attributeId < this->searchableAttributeBoostVector.size()) {
        return this->searchableAttributeBoostVector[attributeId];
    } else {
        return 0;
    }
}

/*
 * Returns true if this searchable attribute is multivalued
 */
bool SchemaInternal::isSearchableAttributeMultiValued(
        const unsigned searchableAttributeNameId) const {
    if (searchableAttributeNameId
            < this->searchableAttributeBoostVector.size()) {
        return this->searchableAttributeIsMultiValuedVector[searchableAttributeNameId];
    } else {
        ASSERT(false);
        return false;
    }
}

const std::map<std::string, unsigned>& SchemaInternal::getSearchableAttribute() const {
    return this->searchableAttributeNameToId;
}

unsigned SchemaInternal::getNumberOfSearchableAttributes() const {
    return this->searchableAttributeNameToId.size();
}

unsigned SchemaInternal::getNumberOfRefiningAttributes() const {

    return this->refiningAttributeNameToId.size();
}

bool SchemaInternal::isHighlightEnabled(unsigned attributeId) const {
    if (attributeId < this->searchableAttributeHighlightEnabled.size()) {
        return this->searchableAttributeHighlightEnabled[attributeId];
    } else {
        return false;
    }
}

void SchemaInternal::setPositionIndexType(PositionIndexType positionIndexType) {
	this->positionIndexType = positionIndexType;
}

void SchemaInternal::setAclSearchableAttrIdsList(const std::vector<unsigned>& aclSearchableAttrIds){
	this->aclSearchableAttrIds = aclSearchableAttrIds;
}
void SchemaInternal::setNonAclSearchableAttrIdsList(const std::vector<unsigned>& nonAclSearchableAttrIds){
	this->nonAclSearchableAttrIds = nonAclSearchableAttrIds;
}

void SchemaInternal::setAclRefiningAttrIdsList(const std::vector<unsigned>& aclRefiningAttrIds){
	this->aclRefiningAttrIds = aclRefiningAttrIds;
}
void SchemaInternal::setNonAclRefiningAttrIdsList(const std::vector<unsigned>& nonAclRefiningAttrIds){
	this->nonAclRefiningAttrIds = nonAclRefiningAttrIds;
}

const std::vector<unsigned>& SchemaInternal::getAclSearchableAttrIdsList() const{
	return this->aclSearchableAttrIds;
}
const std::vector<unsigned>& SchemaInternal::getNonAclSearchableAttrIdsList() const{
	return this->nonAclSearchableAttrIds;
}
const std::vector<unsigned>& SchemaInternal::getAclRefiningAttrIdsList() const{
	return this->aclRefiningAttrIds;
}
const std::vector<unsigned>& SchemaInternal::getNonAclRefiningAttrIdsList() const{
	return this->nonAclRefiningAttrIds;
}

}
}
