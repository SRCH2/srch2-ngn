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
#include "util/exprtk.hpp"
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
using srch2::instantsearch::Schema;

namespace srch2 {
namespace httpwrapper {
typedef enum {
    FqKeywordTypeComplex, FqKeywordTypeRange, FqKeywordTypeAssignment
} FqKeywordType;
class QueryExpression {
public:
    std::vector<std::pair<MessageType, string> > *messages;
    virtual bool parse(string &expressionString) = 0;

    virtual bool validate(const Schema & schema) = 0;

    virtual bool getBooleanValue(
            std::map<std::string, Score> nonSearchableAttributeValues)= 0;

    virtual ~QueryExpression() {
    }
    ;
    std::vector<std::pair<MessageType, string> >* getMessageContainer() {
        return this->messages;
    }

private:
};
class SolrRangeQueryExpression: public QueryExpression {
public:
    SolrRangeQueryExpression(std::string field,
            std::vector<std::pair<MessageType, string> > *messages) {
        this->attributeName = field;
        this->negative = false;
        this->messages = messages;
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

    bool validate(const Schema & schema) {
        //1. Check to make sure attributeName is a non-searchable attribute
        int attributeId = schema.getNonSearchableAttributeId(attributeName);
        if (attributeId < 0)
            return false;
        //2. Check to make sure lower and upper values are consistent with the type
        FilterType attributeType = schema.getTypeOfNonSearchableAttribute(
                attributeId);
        if (attributeValueLower.compare("*") != 0) {
            if (!validateValueWithType(attributeType, attributeValueLower)) {
                return false;
            }
        }

        if (attributeValueUpper.compare("*") != 0) {
            if (!validateValueWithType(attributeType, attributeValueUpper)) {
                return false;
            }
        }
    }

    bool getBooleanValue(
            std::map<std::string, Score> nonSearchableAttributeValues) {
        // first find the value coming from the record
        Score value = nonSearchableAttributeValues[this->attributeName];
        bool lowerBoundCheck = false;
        if (attributeValueLower.compare("*") == 0) {
            lowerBoundCheck = true;
        } else {
            Score lowerBound;
            lowerBound.setScore(value.getType(), attributeValueLower);
            lowerBoundCheck = (lowerBound <= value);
        }

        bool upperBoundCheck = false;
        if (attributeValueUpper.compare("*") == 0) {
            upperBoundCheck = true;
        } else {
            Score upperBound;
            upperBound.setScore(value.getType(), attributeValueUpper);
            upperBoundCheck = (value <= upperBound);
        }

        bool result = lowerBoundCheck && upperBoundCheck;
        if (negative) {
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
    SolrAssignmentQueryExpression(std::string field,
            std::vector<std::pair<MessageType, string> > *messages) {
        this->negative = false;
        this->operation = srch2::instantsearch::EQUALS;
        this->attributeName = field;
        this->messages = messages;
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
        if (!isParsed) {
            // error
            this->getMessageContainer()->push_back(
                    make_pair(MessageError,
                            "Invalid filter query assignment keyword."));
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

    bool validate(const Schema & schema) {
        //1. Check to make sure attributeName is a non-searchable attribute
        int attributeId = schema.getNonSearchableAttributeId(attributeName);
        if (attributeId < 0)
            return false;

        //2. Check the value to be consistent with type
        FilterType attributeType = schema.getTypeOfNonSearchableAttribute(
                attributeId);
        if (attributeValue.compare("*") != 0) {
            if (!validateValueWithType(attributeType, attributeValue)) {
                return false;
            }
        }
    }

    bool getBooleanValue(
            std::map<std::string, Score> nonSearchableAttributeValues) {

        if (attributeValue.compare("*") == 0) {
            attributeValue = "";
            negative = !negative;
        }
        // first find the value coming from the record
        Score value = nonSearchableAttributeValues[this->attributeName];

        Score valueToCheck;
        valueToCheck.setScore(value.getType(), attributeValue);

        bool result = (value == valueToCheck);
        if (!negative) { // no '-' in the beginning of the field
            return result;
        } else {
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
    ComplexQueryExpression(
            std::vector<std::pair<MessageType, string> > *messages) {
        this->messages = messages;
    }
    bool parse(string &expressionString) {
        this->parsedExpression = "";
        bool isParsed = this->parseComplxExpression(expressionString,
                this->parsedExpression);
        if (!isParsed) {
            this->getMessageContainer()->push_back(
                    make_pair(MessageError, "Invalid expression query."));
            return false;
        }
        // remove leading '$'
        expressionString = expressionString.substr(1);
        boost::algorithm::trim(expressionString);
        // extract the expression, remove the 'CMPLX$' and ')' part
        return true;
    }
    bool parseComplxExpression(string &input, string &output) {
        boost::regex re(FQ_COMPLEX_EXPRESSION_REGEX_STRING); //TODO: compile this regex when the engine starts.
        return doParse(input, re, output);
    }

    bool validate(const Schema & schema) {
        // insert non-searchable attribute names to the symbol table and let
        // exprtk library do the validation

        const std::map<std::string, unsigned> * nonSearchableAttributes = schema
                .getNonSearchableAttributes();

        for (std::map<std::string, unsigned>::const_iterator nonSearchableAttribute =
                nonSearchableAttributes->begin();
                nonSearchableAttribute != nonSearchableAttributes->end();
                ++nonSearchableAttribute) {
            if (schema.getTypeOfNonSearchableAttribute(
                    nonSearchableAttribute->second)
                    == srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED
                    || schema.getTypeOfNonSearchableAttribute(
                            nonSearchableAttribute->second)
                            == srch2::instantsearch::ATTRIBUTE_TYPE_FLOAT) {
                symbolVariables.insert(
                        std::make_pair(nonSearchableAttribute->first, 0)); // zero is just a place holder, so that a variable is allocated in the vector
                symbolTable.add_variable(nonSearchableAttribute->first,
                        symbolVariables[nonSearchableAttribute->first], false);
            }
        }

        // now reister the symbol table in the library
        expression.register_symbol_table(symbolTable);

        // now parse the string
        exprtk::parser<double> expressionParser;
        return expressionParser.compile(parsedExpression, expression);
    }

    bool getBooleanValue(
            std::map<std::string, Score> nonSearchableAttributeValues) {
        return getBooleanValueNumericalMode(nonSearchableAttributeValues);
    }

    ~ComplexQueryExpression() {
    }
    ;

private:
    string parsedExpression;
    exprtk::expression<double> expression;
    exprtk::symbol_table<double> symbolTable;
    std::map<string, double> symbolVariables;

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
        for (std::map<std::string, double>::iterator symbol = symbolVariables
                .begin(); symbol != symbolVariables.end(); ++symbol) {
            symbol->second = nonSearchableAttributeValues[symbol->first]
                    .castToFloat();
        }

        return this->expression.value();

    }

};
class FilterQueryEvaluator: public NonSearchableAttributeExpressionEvaluator {
public:

    FilterQueryEvaluator(std::vector<std::pair<MessageType, string> > *messages,
            BooleanOperation *termFQBooleanOperator) {
        this->operation = srch2::instantsearch::BooleanOperatorAND;
        this->isFqBoolOperatorSet = false;
        this->messages = messages;
        this->termFQBooleanOperator = termFQBooleanOperator;
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

    bool parseAndAddCriterion(string &fq) {
        bool parseNextTerm = true;
        while (parseNextTerm) {
            string fqField;
            bool isParsed = this->parseFqField(fq, fqField);
            if (!isParsed) {
                // it is not a assignment not a range
                // see if it's a complx query
                string complxStr = "";
                isParsed = this->parseComplx(fq, complxStr); // checks and removes the CMPLX$ string returns true if found CMPLX$
                if (!isParsed) {
                    parseNextTerm = false;
                    Logger::info(" Parsing error:: not a valid filter query");
                    this->messages->push_back(
                            make_pair(MessageError,
                                    "Parse error, not a valid filter query term."));
                    return false;
                } else {
                    Logger::debug(
                            " 'CMPLX$' found, possible complex expression query");
                    string dummyField = "NO_FIELD";
                    isParsed = this->addCriterion(fq, FqKeywordTypeComplex, dummyField); // NO_FIELD, is a dummy parameter, that will not be used.
                    if (!isParsed) {
                        return false;
                    }
                    boost::algorithm::trim(fq);
                }
            } else {
                // remove the ':'
                fqField = fqField.substr(0, fqField.length() - 1);
                // check if it's a range query or asignment.
                if ('[' == fq.at(0)) {
                    Logger::debug(" '[' found, possible range query");
                    string keyword = "";
                    fq = fq.substr(1);
                    isParsed = this->addCriterion(fq, Range, fqField); // it parses fq for range query parameters
                    if (!isParsed) {
                        //this->isParsedError = true;
                        return false;
                    }
                } else {
                    Logger::debug(" '[' not found, possible assignment query");
                    string keyword = "";
                    isParsed = this->addCriterion(fq, Assignment, fqField); // it parses fq for range query parameters
                    if (!isParsed) {
                        //this->isParsedError = true; //TODO:
                        return false;
                    }
                }
            }
            string boolOperator = "";
            isParsed = this->parseFqBoolOperator(fq, boolOperator);
            if (isParsed) {
                this->setOperation(*this->termFQBooleanOperator);
                string msgStr = "boolean operator is " + boolOperator;
                Logger::debug(msgStr.c_str());
                parseNextTerm = true;
                Logger::debug("LOOPING AGAIN");
            } else {
                // no boolean operator found.
                // if the fq string length is >0 throw error.
                parseNextTerm = false;
                if (fq.length() > 0) {
                    // raise error message
                    Logger::info(
                            " Parsing error:: expecting boolean operator while parsing terms, not found.");
                    this->messages->push_back(
                            make_pair(MessageError,
                                    "Parse error, expecting boolean operator while parsing filter query terms."));
                    //this->isParsedError = true; // TODO:
                    return false;
                }
            }
        }
        return true;
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
    bool isFqBoolOperatorSet;
    std::vector<std::pair<MessageType, string> > *messages;
    BooleanOperation *termFQBooleanOperator;
    bool parseFqField(string &input, string &field) {
        boost::regex re(FQ_FIELD_REGEX_STRING); //TODO: compile this regex when the engine starts.
        return doParse(input, re, field);
    }
    bool parseComplx(string &input, string &output) {
        boost::regex re(COMPLEX_TERM_REGEX_STRING); //TODO: compile this regex when the engine starts.
        return doParse(input, re, output);
    }
    bool parseFqBoolOperator(string &input, string &output) {
        boost::regex re(FQ_TERM_BOOL_OP_REGEX_STRING); //TODO: compile this regex when the engine starts.
        bool isParsed = doParse(input, re, output);
        if (!this->isFqBoolOperatorSet && isParsed) {
            this->populateFilterQueryTermBooleanOperator(output);
            this->isFqBoolOperatorSet = true;
        }
        return isParsed;
    }
    void populateFilterQueryTermBooleanOperator(const string &termOperator) {
        /*
         * populates teh termFQBooleanOperators in container.
         */
        // TODO: check for && and || also
        Logger::debug("inside populateFilterQueryTermBooleanOperators.");
        if (boost::iequals("OR", termOperator)
                || termOperator.compare("||") == 0) {
            *this->termFQBooleanOperator =
                    srch2::instantsearch::BooleanOperatorOR;
        } else if (boost::iequals("AND", termOperator)
                || termOperator.compare("&&") == 0) {
            *this->termFQBooleanOperator =
                    srch2::instantsearch::BooleanOperatorAND;
        } else {
            // generate MessageWarning and use AND
            this->messages->push_back(
                    make_pair(MessageWarning,
                            "Invalid boolean operator specified as term boolean operator "
                                    + termOperator
                                    + ", ignoring it and using 'AND'."));
            *this->termFQBooleanOperator =
                    srch2::instantsearch::BooleanOperatorAND;
        }
        Logger::debug(
                "returning from populateFilterQueryTermBooleanOperators.");
    }
    bool addCriterion(std::string &criteriaString, FqKeywordType type,
            string &field) {
        bool isParsable = false;
        switch (type) {
        case FqKeywordTypeRange: {
            SolrRangeQueryExpression * sre = new SolrRangeQueryExpression(field,
                    this->messages);
            isParsable = sre->parse(criteriaString);
            if (!isParsable) {
                return false; // TODO: get the message
            } else {
                criteria.push_back(sre);
                return true;
            }
        }
        case FqKeywordTypeAssignment: {
            SolrAssignmentQueryExpression * sqe =
                    new SolrAssignmentQueryExpression(field, this->messages);
            isParsable = sqe->parse(criteriaString);
            if (!isParsable) {
                return false;
            } else {
                criteria.push_back(sqe);
                return true;
            }
        }
        case FqKeywordTypeComplex: {
            ComplexQueryExpression * cqe = new ComplexQueryExpression(
                    this->messages);
            isParsable = cqe->parse(criteriaString);
            if (!isParsable) {
                return false;
            } else {
                criteria.push_back(cqe);
                return true;
            }
        }
        default:
            return false;

        }
    }
}
;

}
}

#endif // _WRAPPER_FILTERQUERYEVALUATOR_H_
