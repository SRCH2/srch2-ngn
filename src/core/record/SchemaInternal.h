//$Id: SchemaInternal.h 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

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

#ifndef __SCHEMAINTERNAL_H__
#define __SCHEMAINTERNAL_H__

#include <instantsearch/platform.h>
#include <instantsearch/Schema.h>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include <fstream>
#include <map>
#include <vector>
#include <numeric>
#include <string>

using std::fstream;

namespace srch2 {
namespace instantsearch {

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
 *  - AttributeId: The Id of an attribute in the list of attribute triples.
 *  It depends on the order in which the atrribute were added to the list. The index start
 *  at 0 for the first added attribute, than 1 for the second added attribute and so on.
 *  Note that AttributeId is set internally and the API doesnot allow setting the AttributeId.
 *
 *  - AttributeName: Name of the attribute
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
class SchemaInternal: public Schema {
public:

    /**
     * Creates a Schema object
     */
    SchemaInternal() {
    }
    ;
    SchemaInternal(srch2::instantsearch::IndexType indexType,
            srch2::instantsearch::PositionIndexType positionIndexType);
    SchemaInternal(const SchemaInternal &schemaInternal);

    srch2::instantsearch::IndexType getIndexType() const;

    srch2::instantsearch::PositionIndexType getPositionIndexType() const;

    /**
     *  Sets the name of the primary key to primaryKey.
     */
    void setPrimaryKey(const std::string &primaryKey);

    /**
     * \ingroup RankingFunctions Sets the boost value of an attribute.
     *  @param attributeName  Name of the attribute.
     *  @param attributeBoost The boost value in the range [1-100].
     */
    int setSearchableAttribute(const std::string &attributeName,
            unsigned attributeBoost = 1);

    int setSortableAttribute(const std::string &attributeName, FilterType type,
            std::string defaultValue);

    int setNonSearchableAttribute(const std::string &attributeName,
            FilterType type, const std::string & defaultValue);

    /**
     * Returns the AttributeName of the primaryKey
     */
    const std::string* getPrimaryKey() const;

    /**
     * Gets the boost value of the attribute with
     * attributeId.  \return the boost value of the attribute;
     * "0" if the attribute does not exist.
     */
    unsigned getBoostOfSearchableAttribute(
            const unsigned searchableAttributeNameId) const;
    const std::map<std::string, unsigned>& getSearchableAttribute() const;
    /**
     * Gets the sum of all attribute boosts in the schema.  The
     * returned value can be used for the normalization of
     * attribute boosts.
     */
    unsigned getBoostSumOfSearchableAttributes() const;
    /**
     * @returns the number of attributes in the schema.
     */
    unsigned getNumberOfSearchableAttributes() const;
    /**
     * Gets the index of an attribute name by doing an internal
     * lookup. The index of an attribute depends on the order in
     * which the function setAtrribute was called. The index start
     * at 0 for the first added attribute, than 1 and so on.
     * @return the index if the attribute is found, or -1 otherwise.
     */
    int getSearchableAttributeId(
            const std::string &searchableAttributeName) const;

    const std::string* getDefaultValueOfNonSearchableAttribute(
            const unsigned searchableAttributeNameId) const;
    FilterType getTypeOfNonSearchableAttribute(
            const unsigned searchableAttributeNameId) const;
    int getNonSearchableAttributeId(
            const std::string &searchableAttributeName) const;
    unsigned getNumberOfNonSearchableAttributes() const;
    const std::map<std::string, unsigned> * getNonSearchableAttributes() const;

    int commit() {
        this->commited = 1;
        return 0;
    }

    void setSupportSwap(const bool supportSwap);
    bool getSupportSwap() const;

    void setScoringExpression(const std::string &scoringExpression);
    const std::string getScoringExpression() const;

    /**
     * Destructor to free persistent resources used by the Schema
     */
    virtual ~SchemaInternal();

    /**
     * Internal functions
     */
    static void load(SchemaInternal &schemaInternal,
            const std::string &schemaFullPathFileName) {
        // read the ForwardIndex from the file
        std::ifstream ifs(schemaFullPathFileName.c_str(), std::ios::binary);
        boost::archive::binary_iarchive ia(ifs);
        ia >> schemaInternal;
        ifs.close();
    }
    ;

    static void save(const SchemaInternal &schemaInternal,
            const std::string &schemaFullPathFileName) {
        std::ofstream ofs(schemaFullPathFileName.c_str(), std::ios::binary);
        boost::archive::binary_oarchive oa(ofs);
        oa << schemaInternal;
        ofs.close();
    }
    ;

private:
    std::string primaryKey;

    /**
     * TODO:change to bi - directional map
     * http://www.boost.org/doc/libs/1_37_0/libs/multi_index/doc/index.html
     * http://stackoverflow.com/questions/535317/checking-value-exist-in-a-stdmap-c
     */
    std::map<std::string, unsigned> searchableAttributeNameToId;

    std::vector<unsigned> searchableAttributeBoostVector;

    std::map<std::string, unsigned> nonSearchableAttributeNameToId;
    std::vector<FilterType> nonSearchableAttributeTypeVector;
    std::vector<std::string> nonSearchableAttributeDefaultValueVector;

    srch2::instantsearch::IndexType indexType;
    srch2::instantsearch::PositionIndexType positionIndexType;

    std::string scoringExpressionString;
    bool supportSwap;

    bool commited;

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & primaryKey;
        ar & scoringExpressionString;
        ar & searchableAttributeNameToId;
        ar & searchableAttributeBoostVector;
        ar & nonSearchableAttributeNameToId;
        ar & nonSearchableAttributeTypeVector;
        ar & nonSearchableAttributeDefaultValueVector;
        ar & indexType;
        ar & positionIndexType;
    }
};

}
}

#endif //__SCHEMAINTERNAL_H__
