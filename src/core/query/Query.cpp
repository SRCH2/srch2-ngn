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

// $Id: Query.cpp 3513 2013-06-29 00:27:49Z jamshid.esmaelnezhad $

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

#include <vector>
//#include <instantsearch/Query.h>
//#include <instantsearch/Term.h>
#include <instantsearch/Ranker.h>
#include <instantsearch/ResultsPostProcessor.h>
#include "util/Logger.h"
#include "util/SerializationHelper.h"
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
    unsigned sortableAttributeId;
    float lengthBoost;
    float prefixMatchPenalty;
    QueryType type;
    srch2::instantsearch::SortOrder order;
    std::string refiningAttributeName ;
    std::string refiningAttributeValue ;
    vector<Term* > *terms;
    Shape *range;
    Ranker *ranker;
    ResultsPostProcessorPlan *  plan;

    /*
     * Serialization Scheme :
     * | sortableAttributeId | lengthBoost | prefixMatchPenalty | type | order | refiningAttributeName |  \
     *  refiningAttributeValue | isNULL | isNULL | [terms] | [range] | ranker |
     *  NOTE : we do not serialize plan because it's not used anymore and it must be deleted from codebase
     */
    void * serializeForNetwork(void * buffer){
    	// sortableAttributeId, lengthBoost, prefixMatchPenalty, type, order, refiningAttributeName and refiningAttributeValue
    	buffer = srch2::util::serializeFixedTypes(sortableAttributeId, buffer);
    	buffer = srch2::util::serializeFixedTypes(lengthBoost, buffer);
    	buffer = srch2::util::serializeFixedTypes(prefixMatchPenalty, buffer);
    	buffer = srch2::util::serializeFixedTypes((uint32_t)type, buffer);
    	buffer = srch2::util::serializeFixedTypes((uint32_t)order, buffer);
    	buffer = srch2::util::serializeString(refiningAttributeName, buffer);
    	buffer = srch2::util::serializeString(refiningAttributeValue, buffer);

       	buffer = srch2::util::serializeFixedTypes(bool(terms != NULL), buffer); // isNULL
       	buffer = srch2::util::serializeFixedTypes(bool(range != NULL), buffer); // isNULL
    	// terms vector
		if(terms != NULL){
			// size of vector
			buffer = srch2::util::serializeFixedTypes(uint32_t(terms->size()), buffer);
			for(unsigned termIndex = 0; termIndex < terms->size(); ++termIndex){
				ASSERT(terms->at(termIndex) != NULL);
				buffer = terms->at(termIndex)->serializeForNetwork(buffer);
			}
		}
		if(range != NULL){
			buffer = range->serializeForNetwork(buffer);
		}

    	// TODO ignore ResultsPostProcessorPlan for now
    	return buffer;
    }

    /*
     * Serialization Scheme :
     * | sortableAttributeId | lengthBoost | prefixMatchPenalty | type | order | refiningAttributeName |  \
     *  refiningAttributeValue | isNULL | isNULL | [terms] | [range] | ranker |
     *  NOTE : we do not serialize plan because it's not used anymore and it must be deleted from codebase
     */
    void * deserializerForNetwork(void * buffer){
       	// sortableAttributeId, lengthBoost, prefixMatchPenalty, type, order, refiningAttributeName and refiningAttributeValue
    	buffer = srch2::util::deserializeFixedTypes(buffer, sortableAttributeId);
    	buffer = srch2::util::deserializeFixedTypes(buffer, lengthBoost);
    	buffer = srch2::util::deserializeFixedTypes(buffer, prefixMatchPenalty);
    	uint32_t intVar = 0;
    	buffer = srch2::util::deserializeFixedTypes(buffer, intVar);
    	type = (QueryType)intVar;
    	buffer = srch2::util::deserializeFixedTypes(buffer, intVar);
    	order = (srch2::instantsearch::SortOrder)intVar;
    	buffer = srch2::util::deserializeString(buffer, refiningAttributeName);
    	buffer = srch2::util::deserializeString(buffer, refiningAttributeValue);
    	// terms vector
		bool isTermsNotNull = false;
		buffer = srch2::util::deserializeFixedTypes(buffer, isTermsNotNull);
		bool isRangeNotNull = false;
		buffer = srch2::util::deserializeFixedTypes(buffer, isRangeNotNull);

		if(isTermsNotNull){
			//TODO : leak
			terms = new vector<Term *>();
			unsigned numberOfTerms = 0;
			buffer = srch2::util::deserializeFixedTypes(buffer, numberOfTerms);
			for(unsigned termIndex = 0; termIndex < numberOfTerms; ++termIndex){
				Term * term = new Term("", TERM_TYPE_NOT_SPECIFIED, 0,0,0);
				buffer = Term::deserializeForNetwork(*term, buffer);
				terms->push_back(term);
			}
		}
		if(isRangeNotNull){
			buffer = Shape::deserializeForNetwork(range, buffer);
		}

		if(ranker != NULL){
			delete ranker;
		}
		switch ( type )
		{
			case srch2::instantsearch::SearchTypeTopKQuery:
				ranker = new DefaultTopKRanker();
				break;
			case srch2::instantsearch::SearchTypeGetAllResultsQuery:
				ranker = new GetAllResultsRanker();
				break;
			default:
				ranker = new DefaultTopKRanker();
				break;
		};
    	// TODO ignore ResultsPostProcessorPlan for now

    	return buffer;
    }

    /*
     * Serialization Scheme :
     * | sortableAttributeId | lengthBoost | prefixMatchPenalty | type | order | refiningAttributeName |  \
     *  refiningAttributeValue | isNULL | isNULL | [terms] | [range] | ranker |
     *  NOTE : we do not serialize plan because it's not used anymore and it must be deleted from codebase
     */
    unsigned getNumberOfBytesForNetwork(){
    	unsigned numberOfBytes = 0;
    	numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(sortableAttributeId);
    	numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(lengthBoost);
    	numberOfBytes += srch2::util::getNumberOfBytesFixedTypes(prefixMatchPenalty);
    	numberOfBytes += srch2::util::getNumberOfBytesFixedTypes((uint32_t)type);
    	numberOfBytes += srch2::util::getNumberOfBytesFixedTypes((uint32_t)order);
    	numberOfBytes += srch2::util::getNumberOfBytesString(refiningAttributeName);
       	numberOfBytes += srch2::util::getNumberOfBytesString(refiningAttributeValue);

    	numberOfBytes += srch2::util::getNumberOfBytesFixedTypes((bool)(terms != NULL)) * 2; // whether terms and range are null or not

    	if(terms != NULL){
    		numberOfBytes += sizeof(uint32_t); // vector size
    		for(unsigned termIndex = 0; termIndex < terms->size() ; ++termIndex){
    			numberOfBytes += terms->at(termIndex)->getNumberOfBytesForSerializationForNetwork();
    		}
    	}
    	if(range != NULL){
    		numberOfBytes += range->getNumberOfBytesForSerializationForNetwork();
    	}
    	// TODO : ignore ResultsPostProcessorPlan for now
    	return numberOfBytes;
    }


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

    Impl(const Query::Impl & impl){
        this->sortableAttributeId = impl.sortableAttributeId;
        this->lengthBoost = impl.lengthBoost;
        this->prefixMatchPenalty = impl.prefixMatchPenalty;
        this->type = impl.type;
        this->order = impl.order;
        this->refiningAttributeName = impl.refiningAttributeName;
        this->refiningAttributeValue = impl.refiningAttributeValue;
        if(impl.terms == NULL){
        	this->terms = NULL;
        }else{
        	this->terms = new vector<Term *>();
        	for(unsigned termIdx = 0 ; termIdx < impl.terms->size(); ++termIdx){
        		this->terms->push_back(new Term(*(impl.terms->at(termIdx))));
        	}
        }
        if(impl.range == NULL){
        	this->range = NULL;
        }else{
        	switch (impl.range->getShapeType()) {
				case Shape::TypeRectangle:
					this->range = new Rectangle(*((Rectangle *)impl.range));
					break;
				case Shape::TypeCircle:
					this->range = new Circle(*((Circle *)impl.range));
					break;
				default:
					ASSERT(false);
					break;
			}
        }
        if(impl.ranker == NULL){
        	this->ranker = NULL;
        }else{
        	this->ranker = new Ranker();
        	// TODO : always one type of ranker ?
        }
        if(impl.plan == NULL){
        	this->plan = NULL;
        }else{
        	this->plan = new ResultsPostProcessorPlan(*(impl.plan));
        }

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
    //impl->ranker = new Ranker(ranker);
}

Query::Query(const Query & query){
	impl = new Impl(*(query.impl));
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

void * Query::serializeForNetwork(void * buffer){
	return impl->serializeForNetwork(buffer);
}
void * Query::deserializeForNetwork(Query & query, void * buffer){
	return query.impl->deserializerForNetwork(buffer);
}
unsigned Query::getNumberOfBytesForSerializationForNetwork(){
	return impl->getNumberOfBytesForNetwork();
}

srch2::instantsearch::SortOrder Query::getSortableAttributeIdSortOrder() const
{
    return this->impl->order;
}

}}
