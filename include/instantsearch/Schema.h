//$Id: Schema.h 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

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

#ifndef __SCHEMA_H__
#define __SCHEMA_H__

#include <instantsearch/platform.h>
#include <instantsearch/Constants.h>
#include <string>
#include <map>
#include <vector>

namespace srch2
{
namespace instantsearch
{



/**
 * This class defines a schema of records, which describes the
 * structure of records put into an index. It consists of a
 * "PrimaryKey" and a set of [AttributeId, AttributeName, AttributeBoost] triples.
 * The PrimaryKey is an string that uniquely identifies a record.
 * The PrimaryKey can also be made searchable by adding it as one
 * of the attributes.
 *
 * A schema can have at most 255 attributes, including the primary
 * key.
 *
 *  - AttributeId: The index of an attribute in the list of attribte triples.
 *  It depends on the order in which the attribute were added to the list. The index start
 *  at 0 for the first added attribute, then 1 for the second added attribute and so on.
 *  Note that AttributeId is set internally and the API doesnot allow setting the AttributeId.
 *
 *  -AttributeName: Name of the attribute
 *
 *  - AttributeBoost: Boost value as the importance of this field. A
 * larger AttributeBoost value represents a higher importance of
 * the attribute. A boost value is in the range [1,100].
 *
 *
 * For example, consider a collection of customer records with the
 * following schema:
 *
 *  - PhoneNumber: the PrimaryKey.
 *
 *  - [0, CustName, 50], [1, Address, 10], [2, emailID, 20] are a set of
 *    [AttributeId, AttributeName, AttributeBoost] triples. Here, CustName has a
 *    boost value of 50.
 *
 *  - If we want to search for records using PhoneNumber, we can
 *    make the PrimaryKey searchable by adding it as one of the 
 *    [AttributeId, AttributeName, AttributeBoost] triple, say [3, PhoneNumber, 60].
 */
class MYLIB_EXPORT Schema
{
public:

    /**
     * Creates a Schema object
     */
    static Schema *create(srch2::instantsearch::IndexType indexType, srch2::instantsearch::PositionIndexType positionIndexType = srch2::instantsearch::POSITION_INDEX_NONE);
    //    static Schema *create(srch2::instantsearch::IndexType indexType, srch2::instantsearch::PositionIndexType positionIndexType = srch2::instantsearch::FULLPOSITIONINDEX);

    virtual srch2::instantsearch::IndexType getIndexType() const = 0;

    virtual srch2::instantsearch::PositionIndexType getPositionIndexType() const = 0;

    /**
     *  Sets the name of the primary key to primaryKey.
     */
    virtual void setPrimaryKey(const std::string &primaryKey) = 0;

    /**
     * \ingroup RankingFunctions Sets the boost value of an attribute.
     *  @param attributeName  Name of the attribute.
     *  @param attributeBoost The boost value in the range [1-100].
     */
    virtual int setSearchableAttribute(const std::string &attributeName,
            unsigned attributeBoost = 1, bool isMultiValued = false) = 0;




    virtual int setRefiningAttribute(const std::string &attributeName,
            FilterType type, const std::string & defaultValue, bool isMultiValued = false) = 0;

    /**
     * Returns the AttributeName of the primaryKey
     */
    virtual const std::string* getPrimaryKey() const = 0;


    // get searchable attribute
    virtual const std::map<std::string, unsigned>& getSearchableAttribute() const = 0;
    /**
     * Gets the index of an attribute name by doing an internal
     * lookup. The index of an attribute depends on the order in
     * which the function setAtrribute was called. The index starts
     * at 0 for the first added attribute, than 1 and so on.
     * @return the index if the attribute is found, or -1 otherwise.
     */
    virtual int getSearchableAttributeId(const std::string &searchableAttributeName) const = 0;
    /**
     * @returns the number of attributes in the schema.
     */
    virtual unsigned getNumberOfSearchableAttributes() const = 0;
    /**
     * Gets the boost value of the attribute with
     * attributeId.  \return the boost value of the attribute;
     * "0" if the attribute does not exist.
     *
     *  @param searchableAttributeNameId : A zero-based id according to the order
     *  searchable attributes are defined.
     */
    virtual unsigned getBoostOfSearchableAttribute(const unsigned searchableAttributeNameId) const = 0;

    /*
     * Returns true if this searchable attribute is multivalued
     */
    virtual bool isSearchableAttributeMultiValued(const unsigned searchableAttributeNameId) const = 0;

    /**
     * Gets the sum of all attribute boosts in the schema.  The
     * returned value can be used for the normalization of
     * attribute boosts.
     */
    virtual unsigned getBoostSumOfSearchableAttributes() const = 0;






    // non Searchable attributes
    virtual const std::string* getDefaultValueOfRefiningAttribute(const unsigned refiningAttributeNameId) const = 0;
    virtual FilterType getTypeOfRefiningAttribute(const unsigned refiningAttributeNameId) const = 0;
    virtual int getRefiningAttributeId(const std::string &refiningAttributeName) const = 0;
    virtual unsigned getNumberOfRefiningAttributes() const = 0;
    virtual const std::map<std::string , unsigned> * getRefiningAttributes() const  = 0;
    virtual bool isRefiningAttributeMultiValued(const unsigned refiningAttributeNameId) const = 0;




    /**
	 * Sets the expression which is used to calculate the score of a record
	 * for ranking.
     */
    virtual void setScoringExpression(const std::string &scoringExpression) = 0;
    virtual const std::string getScoringExpression() const = 0;

    // set if support swap operation for edit distance
    virtual void setSupportSwapInEditDistance(const bool supportSwapInEditDistance) = 0;
    virtual bool getSupportSwapInEditDistance() const = 0;

    /**
     * Writes the schema to a folder specified in schema::create(...) function.
     * After commit(), no more changes can be added.
     */
    virtual int commit() = 0;

    /**
     * Destructor to free persistent resources used by the Schema
     */
    virtual ~Schema() {};
};

}}

#endif //__SCHEMA_H__
