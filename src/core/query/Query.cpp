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
 */

#include <vector>
#include <instantsearch/Ranker.h>
#include "util/Logger.h"
#include <string>
#include <sstream>

#include <iostream>

using std::vector;
namespace srch2
{
namespace instantsearch
{
class Term;

struct Query::Impl
{
    QueryType type;
    vector<Term* > *terms;
    unsigned sortableAttributeId;
    float lengthBoost;
    float prefixMatchPenalty;
    srch2::instantsearch::SortOrder order;
    Shape *range;
    Ranker *ranker;

    std::string refiningAttributeName ;
    std::string refiningAttributeValue ;


    ResultsPostProcessorPlan *  plan;

    string toString(){
    	stringstream ss;
    	ss << type;
    	if(terms != NULL){
			for(unsigned i = 0 ; i < terms->size(); ++i){
				ss << terms->at(i)->toString().c_str();
			}
    	}
    	ss << sortableAttributeId;
    	ss << lengthBoost ;
    	ss << prefixMatchPenalty;
    	return ss.str();
    }

    Impl()
    {
        sortableAttributeId = 0;
        order = srch2::instantsearch::SortOrderDescending;
        lengthBoost = 0.5;
        prefixMatchPenalty = 0.90;

        range = NULL;

        plan = NULL;
    }

    virtual ~Impl()
    {
        if (this->terms != NULL)
        {
            for (vector<Term *>::iterator iter = this->terms->begin(); iter != this->terms->end(); iter++)
            {
                if (*iter != NULL)
                    delete *iter;
            }
            delete this->terms;
        }
        delete this->ranker;
        
        if (range != NULL)
            delete range;
    }
};

const srch2::instantsearch::Ranker *Query::getRanker() const
{
    return this->impl->ranker;
}

void Query::add(Term *term)
{
    impl->terms->push_back(term);
}

void Query::setRange(const double &lat_LB, const double &lng_LB, const double &lat_RT, const double &lng_RT)
{
    if (this->impl->range != NULL)
        delete this->impl->range;

    Rectangle *rect = new Rectangle();
    rect->min.x = lat_LB;
    rect->min.y = lng_LB;
    rect->max.x = lat_RT;
    rect->max.y = lng_RT;

    this->impl->range = rect;
}

void Query::setRange(const double &lat_CT, const double &lng_CT, const double &radius)
{
    if (this->impl->range != NULL)
        delete this->impl->range;

    Point p;
    p.x = lat_CT;
    p.y = lng_CT;

    Circle *circle = new Circle(p, radius);

    this->impl->range = circle;
}

void Query::setLengthBoost(float lengthBoost)
{
    if (lengthBoost > 0.0 && lengthBoost < 1.0)
    {
        this->impl->lengthBoost = lengthBoost;
    }
    else
    {
        this->impl->lengthBoost = 0.5;
    }
}

float Query::getLengthBoost() const
{
    return this->impl->lengthBoost;
}

void Query::setPrefixMatchPenalty(float prefixMatchPenalty)
{
    if (prefixMatchPenalty > 0.0 && prefixMatchPenalty < 1.0) {
        this->impl->prefixMatchPenalty = prefixMatchPenalty;
    }
    else {
        this->impl->prefixMatchPenalty = 0.95;
    }

}

float Query::getPrefixMatchPenalty() const
{
    return this->impl->prefixMatchPenalty;
}
    

Query::Query(QueryType type):impl(new Impl)
{
    impl->type = type;
    impl->terms = new vector<Term* >();

    switch ( impl->type )
    {
        case srch2::instantsearch::SearchTypeTopKQuery:
            impl->ranker = new DefaultTopKRanker();
            break;
        case srch2::instantsearch::SearchTypeGetAllResultsQuery:
            impl->ranker = new GetAllResultsRanker();
            break;
        default:
            impl->ranker = new DefaultTopKRanker();
    };
}

Query::Query(QueryType type, const Ranker *ranker)
{
    impl->type = type;
    impl->terms = new vector<Term* >();
}

Query::~Query()
{
    if (impl != NULL)
    {
        delete impl;
    }
}

const vector<Term* >* Query::getQueryTerms() const
{
    return impl->terms;
}

QueryType Query::getQueryType() const
{
    return impl->type;
}

Shape* Query::getShape() const
{
    return impl->range;
}

void Query::setSortableAttribute(unsigned sortableAttributeId, srch2::instantsearch::SortOrder order)
{
    this->impl->sortableAttributeId = sortableAttributeId;
    this->impl->order = order;
}

unsigned Query::getSortableAttributeId() const
{
    return this->impl->sortableAttributeId;
}

// TODO temperory functions, to test range search filter
void Query::setRefiningAttributeName(std::string name){
	this->impl->refiningAttributeName = name;
}
std::string Query::getRefiningAttributeName() const{
	return this->impl->refiningAttributeName;
}

void Query::setRefiningAttributeValue(std::string value){
	this->impl->refiningAttributeValue = value;
}
std::string Query::getRefiningAttributeValue() const{
	return this->impl->refiningAttributeValue;
}



void Query::setPostProcessingPlan(ResultsPostProcessorPlan * plan){
	this->impl->plan = plan;
}
ResultsPostProcessorPlan * Query::getPostProcessingPlan(){
	return this->impl->plan;
}


string Query::toString(){
	return this->impl->toString();
}

srch2::instantsearch::SortOrder Query::getSortableAttributeIdSortOrder() const
{
    return this->impl->order;
}

}}
