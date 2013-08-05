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

namespace srch2 {
namespace httpwrapper {

class QueryExpression {
public:

    virtual bool parse() = 0;

    virtual bool getBooleanValue(
            std::map<std::string, Score> nonSearchableAttributeValues)= 0;

    virtual ~QueryExpression()=0;

    std::string expressionString;
private:

};
class SolrRangeQueryExpression: public QueryExpression {
public:
    SolrRangeQueryExpression(const std::string expressionString) {
        this->expressionString = expressionString; // attr:value or attr:[value TO *] or -attr:value or ..

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
            // remove '[' and ']'
            keyword = keyword.substr(1, keyword.length() - 2);
            // get the lower and uppper values.
            return this->setLowerAndUpperValues(keyword);
        } else {
            //execution should never come here.
            return false;
        }
    }

    bool getBooleanValue(
            std::map<std::string, Score> nonSearchableAttributeValues) {
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

    ~SolrRangeQueryExpression() {
    }
    ;

private:
    std::string attributeName;
    Score attributeValueLower;
    Score attributeValueUpper;
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
            return false;
        }
    }

    bool getBooleanValue(
            std::map<std::string, Score> nonSearchableAttributeValues) {
        bool result;
        // first find the value coming from the record
        Score value = nonSearchableAttributeValues[this->attributeName];
        result = value == this->attributeValue;

        if (this->negative) {
            return !result;
        }
        return result;
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
        string keyword;
        if (matches[0].matched) {
            // it has field. create a vector and populate container->fieldFilter.
            keyword = this->expressionString.substr(
                    matches.position() + matches.length());
            // remove the last ')'
            keyword = keyword.substr(0, keyword.length() - 1);
            boost::algorithm::trim(keyword);
            this->parsedExpression = keyword;
            return true;
        } else {
            return false;
        }
    }

    bool getBooleanValue(
            std::map<std::string, Score> nonSearchableAttributeValues) {

        return getBooleanValueNumericalMode(nonSearchableAttributeValues);

    }

    Score getScoreValue(
            std::map<std::string, Score> nonSearchableAttributeValues) {

        return getScoreValueNumericalMode(nonSearchableAttributeValues);

    }

    ~ComplexQueryExpression() {
    }
    ;

private:
    string expressionString;
    string parsedExpression;
    exprtk::expression<float> expression;
    std::map<std::string, float> symbolVariables;

    bool parseNumerical() {
        // initialize things related to numerical part

        /*exprtk::symbol_table<float> symbol_table;

         for(unsigned i =0;i<nonSearchableAttributeNames.size();i++){
         if(nonSearchableAttributeTypes[i] != srch2::instantsearch::TEXT){
         symbolVariables[nonSearchableAttributeNames[i]] = 0;
         symbol_table.add_variable(nonSearchableAttributeNames[i],symbolVariables[nonSearchableAttributeNames[i]]);
         }
         }
         symbol_table.add_constants();

         this->expression.register_symbol_table(symbol_table);

         exprtk::parser<float> parser;*/
        //parser.compile(this->expressionString,this->expression);
        return false;
    }

    bool getBooleanValueNumericalMode(
            std::map<std::string, Score> nonSearchableAttributeValues) {

        float result = getValueNumericalMode(nonSearchableAttributeValues);

        if (result == 0) {
            return false;
        }
        return true;

    }

    Score getScoreValueNumericalMode(
            std::map<std::string, Score> nonSearchableAttributeValues) {
        float result = getValueNumericalMode(nonSearchableAttributeValues);

        Score resultScore;
        resultScore.setScore(result);

        return resultScore;
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
        case srch2::instantsearch::AND:
            for (std::vector<QueryExpression *>::iterator criterion = criteria
                    .begin(); criterion != criteria.end(); ++criterion) {
                QueryExpression * qe = *criterion;
                if (!qe->getBooleanValue(nonSearchableAttributeValues)) {
                    return false;
                }

            }
            return true;
        case srch2::instantsearch::OR:
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
        string filedRegexString = "\\s*\\w+(\\.{0,1}\\w+)+\\s*";
        string keywordRegexString =
                "\\s*(\\.{0,1}\\w+(\\.{0,1}\\w+)+\\.{0,1}\\*{0,1}|\\*)";
        string validNumRegexString = "\\s*(\\d+|\\*)\\s+TO\\s+(\\d+|\\*)\\s*";
        string validDateRegexString =
                "\\s*(\\d{4}\\/\\d{2}\\/\\d{2}|\\*)\\s+TO\\s+(\\d{4}\\/\\d{2}\\/\\d{2}|\\*)\\s*"; // yyyy/mm/dd
        std::string rangeCriterionRegexString = filedRegexString + ":\\s*\\[("
                + validDateRegexString + "|" + validNumRegexString + ")\\]\\s*";
        boost::regex rangeCriterionRegex(rangeCriterionRegexString);
        std::string assignmentCriterionRegexString = filedRegexString + ":"
                + keywordRegexString;
        boost::regex assignmentCriterionRegex(assignmentCriterionRegexString);
        if (boost::regex_match(criteriaString, rangeCriterionRegex)) {
            SolrRangeQueryExpression * sqe = new SolrRangeQueryExpression(
                    criteriaString);
            bool isParsable = sqe->parse();
            if (!isParsable) {
                return false;
            } else {
                criteria.push_back(sqe);
                return true;
            }
        } else if (boost::regex_match(criteriaString,
                assignmentCriterionRegex)) {
            SolrAssignmentQueryExpression * sqe =
                    new SolrAssignmentQueryExpression(criteriaString);
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
                ComplexQueryExpression * cqe = new ComplexQueryExpression(
                        criteriaString);
                bool isParsable = cqe->parse();
                if (!isParsable) {
                    return false;
                } else {
                    criteria.push_back(cqe);
                    return true;
                }
            }else{
                return false;
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
