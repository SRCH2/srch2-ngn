
#ifndef __QUERY_H__
#define __QUERY_H__

#include <instantsearch/platform.h>
#include <instantsearch/Term.h>
#include <instantsearch/Constants.h>
#include "record/LocationRecordUtil.h"
#include <vector>
#include <string>


namespace srch2
{
namespace instantsearch
{

class Ranker;
class ResultsPostProcessorPlan;


/**
 * This class defines a query that is passed to the IndexSearcher. A
 * query has a list of Term objects. We can add more terms to a
 * query.  A query with multiple terms is interpreted using the "AND"
 * semantics.  The query is responsible for freeing the space of the
 * terms when the query object is deleted.  The following is an
 * example:
 *
 * Query *q = new Query();
 * Term term1 = new FuzzyTerm("pizza", PREFIX, 1, 100);
 * q.add(term1);
 *
 * Term term2 = new FuzzyTerm("delivery", PREFIX, 1, 100);
 * q.add(term2);
 *
 * Term term3 = new FuzzyTerm("irvine", PREFIX, 1, 100);
 * q.add(term3);
 *
 * Term term4 = new FuzzyTerm("free", PREFIX, 1, 100);
 * q.add(term4);
 *
 * delete q; // deletes q and also all the Term objects it holds.
 */
class MYLIB_EXPORT Query
{
public:
    /**
     * Constructs a Query object
     */
    Query(QueryType type);
    /*
     * @param Ranker is used to calculate the score of a record.
     */
    Query(QueryType type, const Ranker *ranker);

    /**
     * Destructor to free the Query object. The query is
           responsible for freeing the space of the terms. Look at
           the example code above.
     */
    virtual ~Query();

    /**
     * Adds a term to the query.
     * @param term A term that is appended to the query. For example,
     * if the query is "ullman compilor", then a
     * call add(term) with the term of "book" will
     * rewrite the query as "ullman compilor book".
     */
    void add(srch2::instantsearch::Term *term);

    /*
     * Sets a rectangle range for geo search
     */
    void setRange(const double &lat_LB, const double &lng_LB, const double &lat_RT, const double &lng_RT);
    /*
     * Sets a circle range for geo search
     */
    void setRange(const double &lat_CT, const double &lng_CT, const double &radius);

    /*
     * TODO implement the right getRange functions correcsponding
     * to different ranges above (rectangle and circle).
     *
     * The number of elements in \parameter values is used to
     * see if it is rectangle or circle.
     */
    Shape* getShape() const;


    /*
     * Used in ranking function.
     */
    void setLengthBoost(float lengthBoost);
    float getLengthBoost() const;

    /*
     * Used in ranking function.
     */
    void setPrefixMatchPenalty(float prefixMatchPenalty);
    float getPrefixMatchPenalty() const;

    const srch2::instantsearch::Ranker *getRanker() const;

    /**
     * \returns a const pointer to the vector of Term pointers.
     */
    const std::vector<srch2::instantsearch::Term* > *getQueryTerms() const;

    srch2::instantsearch::QueryType getQueryType() const;

    /*
     * TODO Should change this function's name to
     * setSortableAttributeAndOrder
     *
     * TODO split the setter function into two functions and make
     * it consistent with getters.
     */
    void setSortableAttribute(unsigned filterAttributeId, srch2::instantsearch::SortOrder order);
    unsigned getSortableAttributeId() const;



    // Test functions, to test range search filter
    void setRefiningAttributeName(std::string name);
    std::string getRefiningAttributeName() const;

    void setRefiningAttributeValue(std::string value);
    std::string getRefiningAttributeValue() const;





    void setPostProcessingPlan(ResultsPostProcessorPlan * plan);
    ResultsPostProcessorPlan * getPostProcessingPlan();


    string toString();
    /*
     * TODO Should change this function's name to
     * getSortOrder
     */
    srch2::instantsearch::SortOrder getSortableAttributeIdSortOrder() const;

private:
    struct Impl;
    Impl *impl;
};

}}
#endif //__QUERY_H__
