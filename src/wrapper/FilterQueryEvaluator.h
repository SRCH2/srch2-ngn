//$Id: ResultsPostProcessor.h 3456 2013-06-26 02:11:13Z Jamshid $

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


#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <stdlib.h>
#include "ParserUtility.h"
#include <instantsearch/Score.h>
#include <instantsearch/ResultsPostProcessor.h>
#include <instantsearch/NonSearchableAttributeExpressionFilter.h>
#include "WrapperConstants.h"
#include "exprtk.hpp"
#include "boost/regex.hpp"

#ifndef _WRAPPER_FILTERQUERYEVALUATOR_H_

#define _WRAPPER_FILTERQUERYEVALUATOR_H_

using namespace std;
using srch2::instantsearch::Score;
using srch2::instantsearch::FilterType;
using srch2::instantsearch::NonSearchableAttributeExpressionEvaluator;
using srch2::instantsearch::BooleanOperation;
using srch2::instantsearch::AttributeCriterionOperation;

namespace srch2
{
namespace httpwrapper
{



class QueryExpression
{
public:

	virtual bool parse() = 0;


	virtual bool getBooleanValue(std::map<std::string, Score> nonSearchableAttributeValues)= 0;

	virtual Score getScoreValue(std::map<std::string, Score> nonSearchableAttributeValues) = 0;

	virtual ~QueryExpression()=0;

	std::vector< std::string > nonSearchableAttributeNames;
	std::vector< FilterType > nonSearchableAttributeTypes;
	std::string expressionString;
private:

};
class SolrRangeQueryExpression : public QueryExpression
{
public:
	SolrRangeQueryExpression(const std::vector< std::string > nonSearchableAttributeNames ,
					  const std::vector< FilterType > nonSearchableAttributeTypes ,
					  const std::string expressionString ){
		this->nonSearchableAttributeNames = nonSearchableAttributeNames;
		this->nonSearchableAttributeTypes = nonSearchableAttributeTypes;
		this->expressionString = expressionString; // attr:value or attr:[value TO *] or -attr:value or ..


	}

	bool parse(){

//		std::string attributeName = this->expressionString.substr(0,this->expressionString.find(":",0));
//		if(attributeName.at(0) == '-'){
//			this->negative = true;
//			attributeName = attributeName.substr(1,attributeName.size()-1);
//		}
//		std::string attributeValue = this->expressionString.substr(
//				this->expressionString.find(":",0)+1,this->expressionString.size()-(this->expressionString.find(":",0)+1));
//		if(std::find(this->nonSearchableAttributeNames.begin(),this->nonSearchableAttributeNames.end(),attributeName) ==
//				this->nonSearchableAttributeNames.end()){ // if a name is used which is not in attributes
//			return false;
//		}
//
//		// find the type of this attribute
//		FilterType type = this->nonSearchableAttributeTypes.at(
//				std::find(this->nonSearchableAttributeNames.begin(),this->nonSearchableAttributeNames.end(),attributeName)
//		- this->nonSearchableAttributeNames.begin());
//		this->attributeName = attributeName;
//		this->attributeValue.setScore(type, attributeValue);
//		this->operation = EQUALS;

		// TODO : NOT FINISHED YET


		return true;
	}


	bool getBooleanValue(std::map<std::string, Score> nonSearchableAttributeValues){
//		bool result ;
//		// first find the value coming from the record
//		Score value = nonSearchableAttributeValues[this->attributeName];
//
//
//		switch (whichPartHasStar) {
//			case 0:
//
//				break;
//			case 0:
//
//				break;
//			case 0:
//
//				break;
//			case 0:
//
//				break;
//		}
//		return result;
	}

	Score getScoreValue(std::map<std::string, Score> nonSearchableAttributeValues){
		return attributeValueLower;
	}

	~SolrRangeQueryExpression(){};

private:
	std::string attributeName ;
	Score attributeValueLower;
	Score attributeValueUpper;
	unsigned whichPartHasStar; // 0: non, 1:left, 2:right, 3 both
	bool negative;


};

class SolrAssignmentQueryExpression : public QueryExpression
{
public:
	SolrAssignmentQueryExpression(const std::vector< std::string > nonSearchableAttributeNames ,
					  const std::vector< FilterType > nonSearchableAttributeTypes ,
					  const std::string expressionString ){
		this->nonSearchableAttributeNames = nonSearchableAttributeNames;
		this->nonSearchableAttributeTypes = nonSearchableAttributeTypes;
		this->expressionString = expressionString; // attr:value or attr:[value TO *] or -attr:value or ..


	}

	bool parse(){

		std::string attributeName = this->expressionString.substr(0,this->expressionString.find(":",0));
		if(attributeName.at(0) == '-'){
			this->negative = true;
			attributeName = attributeName.substr(1,attributeName.size()-1);
		}
		std::string attributeValue = this->expressionString.substr(
				this->expressionString.find(":",0)+1,this->expressionString.size()-(this->expressionString.find(":",0)+1));
		if(std::find(this->nonSearchableAttributeNames.begin(),this->nonSearchableAttributeNames.end(),attributeName) ==
				this->nonSearchableAttributeNames.end()){ // if a name is used which is not in attributes
			return false;
		}

		// find the type of this attribute
		FilterType type = this->nonSearchableAttributeTypes.at(
				std::find(this->nonSearchableAttributeNames.begin(),this->nonSearchableAttributeNames.end(),attributeName)
		- this->nonSearchableAttributeNames.begin());
		this->attributeName = attributeName;
		this->attributeValue.setScore(type, attributeValue);
		this->operation = srch2::instantsearch::EQUALS;


		return true;
	}


	bool getBooleanValue(std::map<std::string, Score> nonSearchableAttributeValues){
		bool result ;
		// first find the value coming from the record
		Score value = nonSearchableAttributeValues[this->attributeName];
		result = value == this->attributeValue;

		if(this->negative){
			return !result;
		}
		return result;
	}

	Score getScoreValue(std::map<std::string, Score> nonSearchableAttributeValues){
		return attributeValue;
	}

	~SolrAssignmentQueryExpression(){};

private:
	std::string attributeName ;
	Score attributeValue;
	AttributeCriterionOperation operation;
	bool negative;


};

// this class gets a string which is an expression of nuon-searchable attribute names,it evluates the expression
// based on the Score value of those attributes coming from records.
// NOTE: the expressions only must be based on UNSIGNED or FLOAT non-searchable attributes.
class ComplexQueryExpression : public QueryExpression
{
public:
	ComplexQueryExpression(const std::vector< std::string > nonSearchableAttributeNames ,
					  const std::vector< FilterType > nonSearchableAttributeTypes ,
					  const std::string expressionString ){
		this->nonSearchableAttributeNames = nonSearchableAttributeNames;
		this->nonSearchableAttributeTypes = nonSearchableAttributeTypes;
		this->expressionString = expressionString; // CMPLX(some math expression)


	}


	bool parse(){
		return parseNumerical();
	}


	bool getBooleanValue(std::map<std::string, Score> nonSearchableAttributeValues){

		return getBooleanValueNumericalMode(nonSearchableAttributeValues);

	}

	Score getScoreValue(std::map<std::string, Score> nonSearchableAttributeValues){

		return getScoreValueNumericalMode(nonSearchableAttributeValues);

	}

	~ComplexQueryExpression(){};

private:
	exprtk::expression<float> expression;
	std::map<std::string , float> symbolVariables;

	bool parseNumerical(){
		// initialize things related to numerical part

		exprtk::symbol_table<float> symbol_table;

		for(unsigned i =0;i<nonSearchableAttributeNames.size();i++){
			if(nonSearchableAttributeTypes[i] != srch2::instantsearch::TEXT){
				symbolVariables[nonSearchableAttributeNames[i]] = 0;
				symbol_table.add_variable(nonSearchableAttributeNames[i],symbolVariables[nonSearchableAttributeNames[i]]);
			}
		}
	   symbol_table.add_constants();

	   this->expression.register_symbol_table(symbol_table);

	   exprtk::parser<float> parser;
	   parser.compile(this->expressionString,this->expression);

	}

	bool getBooleanValueNumericalMode(std::map<std::string, Score> nonSearchableAttributeValues){

		float result = getValueNumericalMode(nonSearchableAttributeValues);

		if(result == 0){
			return false;
		}
		return true;

	}

	Score getScoreValueNumericalMode(std::map<std::string, Score> nonSearchableAttributeValues){
		float result = getValueNumericalMode(nonSearchableAttributeValues);

		Score resultScore;
		resultScore.setScore(result);

		return resultScore;
	}


	float getValueNumericalMode(std::map<std::string, Score> nonSearchableAttributeValues){


		// set values of variables
		for(std::map<std::string , float>::iterator symbol = symbolVariables.begin();
				symbol != symbolVariables.end(); ++symbol){
			symbol->second = nonSearchableAttributeValues[symbol->first].castToFloat();
		}

		return this->expression.value();

	}


};
class FilterQueryEvaluator : public NonSearchableAttributeExpressionEvaluator
{
public:

	FilterQueryEvaluator(const std::vector< std::string > nonSearchableAttributeNames ,
					  const std::vector< FilterType > nonSearchableAttributeTypes,
					  BooleanOperation op){
		this->nonSearchableAttributeNames = nonSearchableAttributeNames;
		this->nonSearchableAttributeTypes = nonSearchableAttributeTypes;
		this->operation = op;
	}

	bool evaluate(std::map<std::string, Score> nonSearchableAttributeValues){
		switch (operation) {
			case srch2::instantsearch::AND:
				for(std::vector<QueryExpression * >::iterator criterion = criteria.begin();
								criterion != criteria.end() ; ++criterion){
					QueryExpression * qe = *criterion;
					if(! qe->getBooleanValue(nonSearchableAttributeValues)){
						return false;
					}

				}
				return true;
			case srch2::instantsearch::OR:
				for(std::vector<QueryExpression * >::iterator criterion = criteria.begin();
								criterion != criteria.end() ; ++criterion){
					QueryExpression * qe = *criterion;
					if(qe->getBooleanValue(nonSearchableAttributeValues)){
						return true;
					}

				}
				return false;
				break;
			default:
				break;
		}
		return false; // TODO : should change to ASSERT(false); because it should never reach here
	}

	bool addCriterion(std::string criteriaString){
		std::string assignmentCriterionRegexString = ".*:.*";
		std::string rangeCriterionRegexString = ".*:\[.*TO.*\]";

		boost::regex assignmentCriterionRegex(assignmentCriterionRegexString);
		boost::regex rangeCriterionRegex(rangeCriterionRegexString);

		if(boost::regex_match(criteriaString,assignmentCriterionRegex)){
			SolrAssignmentQueryExpression * sqe = new SolrAssignmentQueryExpression(nonSearchableAttributeNames,nonSearchableAttributeTypes,criteriaString);
			bool isParsable = sqe->parse();
			if(!isParsable){
				return false;
			}else{
				criteria.push_back(sqe);
				return true;
			}

		}else if(boost::regex_match(criteriaString,rangeCriterionRegex)){
			SolrRangeQueryExpression * sqe = new SolrRangeQueryExpression(nonSearchableAttributeNames,nonSearchableAttributeTypes,criteriaString);
			bool isParsable = sqe->parse();
			if(!isParsable){
				return false;
			}else{
				criteria.push_back(sqe);
				return true;
			}
		}else{
			ComplexQueryExpression * cqe = new ComplexQueryExpression(nonSearchableAttributeNames,nonSearchableAttributeTypes,criteriaString);
			bool isParsable = cqe->parse();
			if(!isParsable){
				return false;
			}else{
				criteria.push_back(cqe);
				return true;
			}
		}
	}

	~FilterQueryEvaluator(){
		for(std::vector<QueryExpression * >::iterator criterion = criteria.begin();
				criterion != criteria.end(); ++criterion){
				delete *criterion;
		}
	}
private:
	std::vector< std::string > nonSearchableAttributeNames;
	std::vector< FilterType > nonSearchableAttributeTypes;

	std::vector<QueryExpression * > criteria;
	BooleanOperation operation;

};

}
}

#endif // _WRAPPER_FILTERQUERYEVALUATOR_H_
