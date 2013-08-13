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

 * Copyright ������ 2010 SRCH2 Inc. All rights reserved
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
#include "instantsearch/Schema.h"
#include "WrapperConstants.h"
#include "exprtk.hpp"
#include "boost/regex.hpp"
#include <boost/algorithm/string.hpp>
//#include "Assert.h"

#ifndef _WRAPPER_FILTERQUERYEVALUATOR_H_

#define _WRAPPER_FILTERQUERYEVALUATOR_H_

using namespace std;
using srch2::instantsearch::Score;
using srch2::instantsearch::FilterType;
using srch2::instantsearch::NonSearchableAttributeExpressionEvaluator;
using srch2::instantsearch::BooleanOperation;
using srch2::instantsearch::AttributeCriterionOperation;
using srch2::instantsearch::Schema;

namespace srch2 {
namespace httpwrapper {

class QueryExpression {
public:

    virtual bool parse() = 0;

    virtual bool validate(const Schema & schema) = 0;

    virtual bool getBooleanValue(
            std::map<std::string, Score> nonSearchableAttributeValues)= 0;

    virtual ~QueryExpression() {
    }
    ;

    std::string expressionString;
private:

};
class SolrRangeQueryExpression: public QueryExpression {
public:
    SolrRangeQueryExpression(const std::string expressionString) {
        this->expressionString = expressionString; // attr:value or attr:[value TO *] or -attr:value or ..
        negative = false;
    }

    bool parse() {
        string keyword;
        boost::smatch matches;
        string fieldKeywordDelimeterRegexString = "\\s*:\\s*";
        boost::regex fieldDelimeterRegex(fieldKeywordDelimeterRegexString);
        boost::regex_search(this->expressionString, matches,fieldDelimeterRegex);
        if (matches[0].matched) {
            // it has field. create a vector and populate container->fieldFilter.
            string fieldName = this->expressionString.substr(0,
                    matches.position()); // extract the field
            boost::algorithm::trim(fieldName);
            if (fieldName.at(0) == '-') {
                this->negative = true;
                fieldName = fieldName.substr(1);
            } else {
                this->negative = false;
            }
            this->attributeName = fieldName;
            keyword = this->expressionString.substr(
                    matches.position() + matches.length()); // extract the keyword
            boost::algorithm::trim(keyword);
            // remove '[' and ']'
            keyword = keyword.substr(1, keyword.length() - 2);
            // get the lower and uppper values.
            return this->setLowerAndUpperValues(keyword);
        } else {
            //execution should never come here.
            //ASSERT(false);
            return false;
        }
    }

    bool validate(const Schema & schema){
        //1. Check to make sure attributeName is a non-searchable attribute
        int attributeId = schema.getNonSearchableAttributeId(attributeName);
        if(attributeId < 0) return false;
        //2. Check to make sure lower and upper values are consistent with the type
        FilterType attributeType = schema.getTypeOfNonSearchableAttribute(attributeId);
        if(attributeValueLower.compare("*") != 0){
            if(! validateValueWithType(attributeType , attributeValueLower)){
                return false;
            }
        }

        if(attributeValueUpper.compare("*") != 0){
            if(! validateValueWithType(attributeType , attributeValueUpper)){
                return false;
            }
        }
    }

    bool getBooleanValue(
            std::map<std::string, Score> nonSearchableAttributeValues) {
		// first find the value coming from the record
		Score value = nonSearchableAttributeValues[this->attributeName];
		bool lowerBoundCheck = false;
		if(attributeValueLower.compare("*") == 0){
		    lowerBoundCheck = true;
		}else{
		    Score lowerBound;
		    lowerBound.setScore(value.getType() , attributeValueLower);
		    lowerBoundCheck = (lowerBound <= value);
		}

		bool upperBoundCheck = false;
		if(attributeValueUpper.compare("*") == 0){
		    upperBoundCheck = true;
		}else{
		    Score upperBound;
		    upperBound.setScore(value.getType() , attributeValueUpper);
		    upperBoundCheck = (value <= upperBound);
		}

		bool result = lowerBoundCheck && upperBoundCheck;
		if(negative){
		    result = !result;
		}
		return result;
    }

    ~SolrRangeQueryExpression() {
    }
    ;

private:
    std::string attributeName;
    string attributeValueLower;
    string attributeValueUpper;
    unsigned whichPartHasStar; // 0: non, 1:left, 2:right, 3 both
    bool negative;
    bool setLowerAndUpperValues(string &keyword) {
        boost::smatch matches;
        string fieldKeywordDelimeterRegexString = "\\s+TO\\s+";
        boost::regex fieldDelimeterRegex(fieldKeywordDelimeterRegexString);
        boost::regex_search(keyword, matches, fieldDelimeterRegex);
        if (matches[0].matched) {
            // it has field. create a vector and populate container->fieldFilter.
            string lowerVal = keyword.substr(0, matches.position()); // extract the field
            boost::algorithm::trim(lowerVal);
            this->attributeValueLower = lowerVal;
            string upperVal = keyword.substr(
                    matches.position() + matches.length()); // extract the keyword
            boost::algorithm::trim(upperVal);
            // remove '[' and ']'
            this->attributeValueUpper = lowerVal;
            return true;
        } else {
            //ASSERT(false);
            return false;
        }
    }
};

class SolrAssignmentQueryExpression: public QueryExpression {
public:
    SolrAssignmentQueryExpression(const std::string expressionString) {
        this->expressionString = expressionString; // attr:value or attr:[value TO *] or -attr:value or ..
        this->negative = false;
        this->operation = srch2::instantsearch::EQUALS;
    }

    bool parse() {
        string keyword;
        boost::smatch matches;
        string fieldKeywordDelimeterRegexString = "\\s*:\\s*";
        boost::regex fieldDelimeterRegex(fieldKeywordDelimeterRegexString);
        boost::regex_search(this->expressionString, matches,
                fieldDelimeterRegex);
        if (matches[0].matched) {
            // it has field. create a vector and populate container->fieldFilter.
            string fieldName = this->expressionString.substr(0,
                    matches.position()); // extract the field
            boost::algorithm::trim(fieldName);
            if (fieldName.at(0) == '-') {
                this->negative = true;
                fieldName = fieldName.substr(1);
            } else {
                this->negative = false;
            }
            this->attributeName = fieldName;
            keyword = this->expressionString.substr(
                    matches.position() + matches.length()); // extract the keyword
            boost::algorithm::trim(keyword);
            this->attributeValue = keyword;
            return true;
        } else {
            //execution should never come here.
            //ASSERT(false);
            return false;
        }
    }

    bool validate(const Schema & schema){
        //1. Check to make sure attributeName is a non-searchable attribute
        int attributeId = schema.getNonSearchableAttributeId(attributeName);
        if(attributeId < 0) return false;

        //2. Check the value to be consistent with type
        FilterType attributeType = schema.getTypeOfNonSearchableAttribute(attributeId);
        if(attributeValue.compare("*") != 0){
            if(! validateValueWithType(attributeType , attributeValue)){
                return false;
            }
        }
    }

    bool getBooleanValue(
            std::map<std::string, Score> nonSearchableAttributeValues) {

        if(attributeValue.compare("*") == 0){
            attributeValue = "";
            negative = ! negative;
        }
        // first find the value coming from the record
        Score value = nonSearchableAttributeValues[this->attributeName];

        Score valueToCheck;
        valueToCheck.setScore(value.getType() , attributeValue);

        bool result = (value == valueToCheck);
        if(!negative){ // no '-' in the beginning of the field
            return result;
        }else{
            return !result;
        }
    }

    ~SolrAssignmentQueryExpression() {
    }
    ;

private:
    std::string attributeName;
    string attributeValue;
    AttributeCriterionOperation operation;
    bool negative;
};

// this class gets a string which is an expression of nuon-searchable attribute names,it evluates the expression
// based on the Score value of those attributes coming from records.
// NOTE: the expressions only must be based on UNSIGNED or FLOAT non-searchable attributes.
class ComplexQueryExpression: public QueryExpression {
public:
    ComplexQueryExpression(const std::string expressionString) {
        this->expressionString = expressionString; // CMPLX(some math expression)
    }

    bool parse() {
        boost::algorithm::trim(this->expressionString);
        // extract the expression, remove the 'CMPLX(' and ')' part
        boost::smatch matches;
        string fieldKeywordDelimeterRegexString = "\\s*CMPLX\\(\\s*";
        boost::regex fieldDelimeterRegex(fieldKeywordDelimeterRegexString);
        boost::regex_search(this->expressionString, matches,
                fieldDelimeterRegex);
        string expressionString;
        if (matches[0].matched) {
            expressionString = this->expressionString.substr(
                    matches.position() + matches.length());
            // remove the last ')'
            expressionString = expressionString.substr(0, expressionString.length() - 1);
            boost::algorithm::trim(expressionString);
            this->parsedExpression = expressionString; //TODO: do exprtk parsing, based on that return true/false
            return true;
        } else {
            //ASSERT(false);
            return false;
        }
    }

    bool validate(const Schema & schema){
        // insert non-searchable attribute names to the symbol table and let
        // exprtk library do the validation

        const std::map<std::string , unsigned> * nonSearchableAttributes = schema.getNonSearchableAttributes();

        for(std::map<std::string , unsigned>::const_iterator nonSearchableAttribute = nonSearchableAttributes->begin();
                nonSearchableAttribute != nonSearchableAttributes->end() ; ++nonSearchableAttribute){
            if(schema.getTypeOfNonSearchableAttribute(nonSearchableAttribute->second) == srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED ||
                    schema.getTypeOfNonSearchableAttribute(nonSearchableAttribute->second) == srch2::instantsearch::ATTRIBUTE_TYPE_FLOAT ){
                symbolVariables.insert(std::make_pair(nonSearchableAttribute->first , 0)); // zero is just a place holder, so that a variable is allocated in the vector
                symbolTable.add_variable(nonSearchableAttribute->first , symbolVariables[nonSearchableAttribute->first] , false);
            }
        }

        // now reister the symbol table in the library
        expression.register_symbol_table(symbolTable);

        // now parse the string
        exprtk::parser<float> expressionParser;
        return expressionParser.compile(parsedExpression , expression);
    }

    bool getBooleanValue(
            std::map<std::string, Score> nonSearchableAttributeValues) {
        return getBooleanValueNumericalMode(nonSearchableAttributeValues);
    }

    ~ComplexQueryExpression() {
    }
    ;

private:
    string expressionString;
    string parsedExpression;
    exprtk::expression<float> expression;
    exprtk::symbol_table<float> symbolTable;
    std::map<string, float> symbolVariables;


    bool getBooleanValueNumericalMode(
            std::map<std::string, Score> nonSearchableAttributeValues) {

        float result = getValueNumericalMode(nonSearchableAttributeValues);

        if (result == 0) {
            return false;
        }
        return true;

    }

    float getValueNumericalMode(
            std::map<std::string, Score> nonSearchableAttributeValues) {

        // set values of variables
        for (std::map<std::string, float>::iterator symbol = symbolVariables
                .begin(); symbol != symbolVariables.end(); ++symbol) {
            symbol->second = nonSearchableAttributeValues[symbol->first]
                    .castToFloat();
        }

        return this->expression.value();

    }

};
class FilterQueryEvaluator: public NonSearchableAttributeExpressionEvaluator {
public:

    FilterQueryEvaluator() {
    }
    void setOperation(BooleanOperation op) {
        this->operation = op;
    }

    bool evaluate(std::map<std::string, Score> nonSearchableAttributeValues) {
        switch (operation) {
        case srch2::instantsearch::BooleanOperatorAND:
            for (std::vector<QueryExpression *>::iterator criterion = criteria
                    .begin(); criterion != criteria.end(); ++criterion) {
                QueryExpression * qe = *criterion;
                if (!qe->getBooleanValue(nonSearchableAttributeValues)) {
                    return false;
                }

            }
            return true;
        case srch2::instantsearch::BooleanOperatorOR:
            for (std::vector<QueryExpression *>::iterator criterion = criteria
                    .begin(); criterion != criteria.end(); ++criterion) {
                QueryExpression * qe = *criterion;
                if (qe->getBooleanValue(nonSearchableAttributeValues)) {
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

    bool addCriterion(std::string criteriaString) {
        string filedRegexString = "\\s*\\w+(\\.{0,1}\\w+)+\\s*"; //TODO: alpha numeric or underscores, minus support also
        string keywordRegexString =
                "\\s*(\\.{0,1}\\w+(\\.{0,1}\\w+)+\\.{0,1}\\*{0,1}|\\*)"; // any value that is in double quotes, remove the double quote before setting it in the members
        string validNumRegexString = "\\s*(\\d+|\\*)\\s+TO\\s+(\\d+|\\*)\\s*"; // TODO: add the string support too.
        string validDateRegexString =
                "\\s*(\\d{4}\\/\\d{2}\\/\\d{2}|\\*)\\s+TO\\s+(\\d{4}\\/\\d{2}\\/\\d{2}|\\*)\\s*"; // yyyy/mm/dd
        std::string rangeCriterionRegexString = filedRegexString + ":\\s*\\[("
                + validDateRegexString + "|" + validNumRegexString + ")\\]\\s*";
        boost::regex rangeCriterionRegex(rangeCriterionRegexString);
        std::string assignmentCriterionRegexString = filedRegexString + ":"
                + keywordRegexString;
        boost::regex assignmentCriterionRegex(assignmentCriterionRegexString);
        if (boost::regex_match(criteriaString, rangeCriterionRegex)) {
            SolrRangeQueryExpression * sqe = new SolrRangeQueryExpression(criteriaString);
            bool isParsable = sqe->parse();
            if (!isParsable) {
                return false; // TODO: get the message
            } else {
                criteria.push_back(sqe);
                return true;
            }
        } else if (boost::regex_match(criteriaString,assignmentCriterionRegex)) {
            SolrAssignmentQueryExpression * sqe = new SolrAssignmentQueryExpression(criteriaString);
            bool isParsable = sqe->parse();
            if (!isParsable) {
                return false;
            } else {
                criteria.push_back(sqe);
                return true;
            }
        } else {
            string complexExpressionRegexString = "\\s*CMPLX\\(.*\\)\\s*"; // need to be careful here. assumption is the fq regex has taken care of the right syntax
            boost::regex cmplxCriterionRegex(complexExpressionRegexString);
            if (boost::regex_match(criteriaString, cmplxCriterionRegex)) {
                ComplexQueryExpression * cqe = new ComplexQueryExpression(criteriaString);
                bool isParsable = cqe->parse();
                if (!isParsable) {
                    return false;
                } else {
                    criteria.push_back(cqe);
                    return true;
                }
            } else {
                return false; // TODO invalid filter syntax.
            }
        }
    }

    ~FilterQueryEvaluator() {
        for (std::vector<QueryExpression *>::iterator criterion =
                criteria.begin(); criterion != criteria.end(); ++criterion) {
            delete *criterion;
        }
    }
private:
    std::vector<QueryExpression *> criteria;
    BooleanOperation operation;

};

}
}

#endif // _WRAPPER_FILTERQUERYEVALUATOR_H_
