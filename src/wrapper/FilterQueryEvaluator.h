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
#include <boost/algorithm/string.hpp>
#include "RegexConstants.h"
//#include "Assert.h"

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
static const string FILTERQUERY_TERM_COMPLEX = "COMPLEX";
static const string FILTERQUERY_TERM_RANGE = "RANGE";
static const string FILTERQUERY_TERM_ASSIGNMENT = "ASSIGNMENT";
class QueryExpression {
public:

    virtual bool parse(string &expressionString) = 0;

    virtual bool getBooleanValue(
            std::map<std::string, Score> nonSearchableAttributeValues)= 0;

    virtual ~QueryExpression() {
    }
    ;
    void setMessageContainer(std::vector<std::pair<MessageType, string> > *messages) {
        this->messages = messages;
    }
    std::vector<std::pair<MessageType, string> >* getMessageContainer() {
        return this->messages;
    }

private:
    std::vector<std::pair<MessageType, string> > *messages;

};
class SolrRangeQueryExpression: public QueryExpression {
public:
    SolrRangeQueryExpression(const std::string field) {
        this->attributeName = field;
        this->negative = false;
    }

    bool parse(string &expressionString) {
        // it has field. create a vector and populate container->fieldFilter.
        boost::algorithm::trim(attributeName);
        if (this->attributeName.at(0) == '-') {
            this->negative = true;
            this->attributeName = this->attributeName.substr(1);
        } else {
            this->negative = false;
        }
        string keyword = "";
        bool isParsed = parseFqRangeKeyword(expressionString, keyword); // extract the keyword
        if (!isParsed) {
            // there is parsing error
            std::vector<std::pair<MessageType, string> > *messages;
            this->getMessageContainer()->push_back(
                    make_pair(MessageError,
                            "Invalid syntax to specify range in filterquery."));
            return false;
        }
        // remove trailing ']'
        keyword = keyword.substr(0, keyword.length() - 1);
        boost::algorithm::trim(keyword);
        // get the lower and uppper values.
        return this->setLowerAndUpperValues(keyword);
    }
    bool parseFqRangeKeyword(string &input, string &output) {
        boost::regex re(FQ_RANGE_QUERY_KEYWORD_REGEX_STRING); //TODO: compile this regex when the engine starts.
        return doParse(input, re, output);
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
        return true;
    }

    ~SolrRangeQueryExpression() {
    }
    ;

private:
    std::string attributeName;
    string attributeValueLower;
    string attributeValueUpper;
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
            this->attributeValueUpper = upperVal;
            return true;
        } else {
            this->getMessageContainer()->push_back(
                    make_pair(MessageError,
                            "Invalid syntax to specify range in filterquery."));
            return false;
        }
    }
};

class SolrAssignmentQueryExpression: public QueryExpression {
public:
    SolrAssignmentQueryExpression(const std::string field) {
        this->negative = false;
        this->operation = srch2::instantsearch::EQUALS;
        this->attributeName = field;
    }

    bool parse(string &expressionString) {
        string keyword;
        // it has field. create a vector and populate container->fieldFilter.
        if (this->attributeName.at(0) == '-') {
            this->negative = true;
            this->attributeName = this->attributeName.substr(1);
        } else {
            this->negative = false;
        }
        bool isParsed = this->parseFqKeyword(expressionString, keyword);
        if(!isParsed){
            // error
            this->getMessageContainer()->push_back(make_pair(MessageError,"Invalid filter query assignment keyword."));
            return false;
        }
        boost::algorithm::trim(keyword);
        this->attributeValue = keyword;
        return true;
    }
    bool parseFqKeyword(string &input, string &output) {
        boost::regex re(FQ_ASSIGNMENT_KEYWORD_REGEX_STRING); //TODO: compile this regex when the engine starts.
        return doParse(input, re, output);
    }
    bool getBooleanValue(
            std::map<std::string, Score> nonSearchableAttributeValues) {
        bool result;
        // first find the value coming from the record
        Score value = nonSearchableAttributeValues[this->attributeName];
        //result = value == this->attributeValue;
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
    ComplexQueryExpression() {
    }
    bool parse(string &expressionString) {
        this->parsedExpression = "";
        bool isParsed = this->parseComplxExpression(expressionString, this->parsedExpression);
        if(!isParsed){
            this->getMessageContainer()->push_back(make_pair(MessageError,"Invalid expression query."));
            return false;
        }
        // remove '$'
        expressionString = expressionString.substr(1);
        boost::algorithm::trim(expressionString);
        // extract the expression, remove the 'CMPLX(' and ')' part
        this->parsedExpression = expressionString; //TODO: do exprtk parsing, based on that return true/false
        return true;
    }
    bool parseComplxExpression(string &input, string &output) {
        boost::regex re(FQ_COMPLEX_EXPRESSION_REGEX_STRING); //TODO: compile this regex when the engine starts.
        return doParse(input, re, output);
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
        this->operation = srch2::instantsearch::AND;
    }
    void setOperation(BooleanOperation op) {
        this->operation = op;
    }
    void setMessageContainer(std::vector<std::pair<MessageType, string> > *messages) {
        this->messages = messages;
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

    bool addCriterion(std::string &criteriaString, const string &type,
            const string &field) {
        if (boost::iequals(FILTERQUERY_TERM_RANGE, type)) {
            SolrRangeQueryExpression * sre = new SolrRangeQueryExpression(
                    field);
            sre->setMessageContainer(this->messages);
            bool isParsable = sre->parse(criteriaString);
            if (!isParsable) {
                return false; // TODO: get the message
            } else {
                criteria.push_back(sre);
                return true;
            }
        } else if (boost::iequals(FILTERQUERY_TERM_ASSIGNMENT, type)) {
            SolrAssignmentQueryExpression * sqe =
                    new SolrAssignmentQueryExpression(field);
            sqe->setMessageContainer(this->messages);
            bool isParsable = sqe->parse(criteriaString);
            if (!isParsable) {
                return false;
            } else {
                criteria.push_back(sqe);
                return true;
            }
        } else if (boost::iequals(FILTERQUERY_TERM_COMPLEX, type)) {
            ComplexQueryExpression * cqe = new ComplexQueryExpression();
            cqe->setMessageContainer(this->messages);
            bool isParsable = cqe->parse(criteriaString);
            if (!isParsable) {
                return false;
            } else {
                criteria.push_back(cqe);
                return true;
            }
        }
        return false;
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
    std::vector<std::pair<MessageType, string> >* messages; // stores the messages related to warnings and errors.

}
;

}
}

#endif // _WRAPPER_FILTERQUERYEVALUATOR_H_
