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
#ifndef __WRAPPER_FILTERQUERYEVALUATOR_H__
#define __WRAPPER_FILTERQUERYEVALUATOR_H__

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <stdlib.h>
#include "ParserUtility.h"
#include <instantsearch/TypedValue.h>
#include <instantsearch/ResultsPostProcessor.h>
#include "instantsearch/Schema.h"
#include "WrapperConstants.h"
#include "util/exprtk.hpp"
#include "boost/regex.hpp"
#include <boost/algorithm/string.hpp>
#include "RegexConstants.h"
#include "util/Assert.h"
#include "util/DateAndTimeHandler.h"


using namespace std;
using srch2::instantsearch::TypedValue;
using srch2::instantsearch::FilterType;
using srch2::instantsearch::RefiningAttributeExpressionEvaluator;
using srch2::instantsearch::BooleanOperation;
using srch2::instantsearch::AttributeCriterionOperation;
using srch2::instantsearch::Schema;
using namespace srch2::instantsearch;
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

    virtual bool evaluate(
            std::map<std::string, TypedValue> & nonSearchableAttributeValues)= 0;

	virtual string getUniqueStringForCaching() = 0;
    virtual ~QueryExpression() {
    }
    ;
    std::vector<std::pair<MessageType, string> >* getMessageContainer() {
        return this->messages;
    }

private:
};

/*
 * Example of a RangeQueryExpression is : price:[ 10 TO 100]
 * which means only return those results that : 10 <= result.price <= 100
 */
class RangeQueryExpression: public QueryExpression {
public:
    RangeQueryExpression(std::string field,
            std::vector<std::pair<MessageType, string> > *messages) {
        this->attributeName = field;
        this->negative = false;
        this->messages = messages;
    }

    bool parse(string &expressionString) {
        // it has field. create a vector and populate container->fieldFilter.
        /*
         * Example of this case : -price:[ 10 TO 200 ]
         * The "-" in the beginning means price is NOT in this range.
         */
        ASSERT(this->attributeName.length() > 1);
        boost::algorithm::trim(attributeName);
        if (this->attributeName.at(0) == '-') {
            ASSERT(this->attributeName.length() > 2);
            this->negative = true;
            this->attributeName = this->attributeName.substr(1);
        } else {
            this->negative = false;
        }
        string keyword = "";
        bool hasParsed = parseFqRangeKeyword(expressionString, keyword); // extract the keyword
        if (!hasParsed) {
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
        int attributeId = schema.getRefiningAttributeId(attributeName);
        if (attributeId < 0)
            return false;
        //2. Check to make sure lower and upper values are consistent with the type
        FilterType attributeType = schema.getTypeOfRefiningAttribute(
                attributeId);
        if (attributeValueLower.compare("*") != 0) {
            if (!validateValueWithType(attributeType, attributeValueLower)) {
                return false;
            }
            // now that it is validated, it should be changed to long representation.
            if(attributeType == srch2::instantsearch::ATTRIBUTE_TYPE_TIME){
            	std::stringstream buffer;
            	buffer << srch2is::DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(attributeValueLower);
            	attributeValueLower = buffer.str();
            }else{
				std::string attributeValueLowerLowercase = attributeValueLower;
				std::transform(attributeValueLowerLowercase.begin(), attributeValueLowerLowercase.end(), attributeValueLowerLowercase.begin(), ::tolower);
				attributeValueLower = attributeValueLowerLowercase;
            }
        }

        if (attributeValueUpper.compare("*") != 0) {
            if (!validateValueWithType(attributeType, attributeValueUpper)) {
                return false;
            }
            // now that it is validated, it should be changed to long representation.
            if(attributeType == srch2::instantsearch::ATTRIBUTE_TYPE_TIME){
            	std::stringstream buffer;
            	buffer << srch2is::DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(attributeValueUpper);
            	attributeValueUpper = buffer.str();
            }else{
				std::string attributeValueUpperLowercase = attributeValueUpper;
				std::transform(attributeValueUpperLowercase.begin(), attributeValueUpperLowercase.end(), attributeValueUpperLowercase.begin(), ::tolower);
				attributeValueUpper = attributeValueUpperLowercase;
            }
        }
        return true;
    }

    /*
     * Checks the value of a result record to see if
     * it is in the range or not. The interval is closed.
     * For example: a result with value 10 returns true for range [10 TO *]
     */
    bool evaluate(std::map<std::string, TypedValue> & nonSearchableAttributeValues) {
        // first find the value coming from the record
        TypedValue value = nonSearchableAttributeValues[this->attributeName];
        bool lowerBoundCheck = false;
        if (attributeValueLower.compare("*") == 0) {
            lowerBoundCheck = true;
        } else {
            TypedValue lowerBound;
            lowerBound.setTypedValue(value.getType(), attributeValueLower);
            lowerBoundCheck = (lowerBound <= value);
        }

        bool upperBoundCheck = false;
        if (attributeValueUpper.compare("*") == 0) {
            upperBoundCheck = true;
        } else {
            TypedValue upperBound;
            upperBound.setTypedValue(value.getType(), attributeValueUpper);
            upperBoundCheck = (value <= upperBound);
        }

        bool result = lowerBoundCheck && upperBoundCheck;
        if (negative) {
            result = !result;
        }
        return result;
    }

    ~RangeQueryExpression() {
    }

	string getUniqueStringForCaching() {
		stringstream ss;
		ss << attributeName.c_str();
		ss << attributeValueLower.c_str();
		ss << attributeValueUpper.c_str();
		ss << negative;
		return ss.str();
	}

private:
    // the name of the attribute which is checked against the range
    std::string attributeName;
    // the lower bound value
    string attributeValueLower;
    // the upper bound value
    string attributeValueUpper;
    // if the expression has a - in the beginning negative is true
    bool negative;
    bool setLowerAndUpperValues(string &rangeExpression) {
        boost::smatch matches;
        // we do the same thing as SOLR : TO is case sensitive.
        boost::regex fieldDelimeterRegex(FQ_FIELD_KEYWORD_DELIMETER_REGEX_STRING);
        boost::regex_search(rangeExpression, matches, fieldDelimeterRegex);
        if (matches[0].matched) {
            // it has field. create a vector and populate container->fieldFilter.
            string lowerVal = rangeExpression.substr(0, matches.position()); // extract the field
            boost::algorithm::trim(lowerVal);
            this->attributeValueLower = lowerVal;
            string upperVal = rangeExpression.substr(
                    matches.position() + matches.length()); // extract the keyword
            boost::algorithm::trim(upperVal);
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

class EqualityQueryExpression: public QueryExpression {
public:
    EqualityQueryExpression(std::string field,
            std::vector<std::pair<MessageType, string> > *messages) {
        this->negative = false;
        this->operation = srch2::instantsearch::EQUALS;
        this->attributeName = field;
        this->messages = messages;
    }

    bool parse(string &expressionString) {
        string expression;
        // it has field. create a vector and populate container->fieldFilter.
        ASSERT(this->attributeName.length() > 1);
        if (this->attributeName.at(0) == '-') {
            ASSERT(this->attributeName.length() > 2);
            this->negative = true;
            this->attributeName = this->attributeName.substr(1);
        } else {
            this->negative = false;
        }
        bool isParsed = this->parseFqKeyword(expressionString, expression);
        if (!isParsed) {
            // error
            this->getMessageContainer()->push_back(
                    make_pair(MessageError,
                            "Invalid filter query assignment keyword."));
            return false;
        }
        boost::algorithm::trim(expression);
        this->attributeValue = expression;
        return true;
    }
    bool parseFqKeyword(string &input, string &output) {
        boost::regex re(FQ_ASSIGNMENT_KEYWORD_REGEX_STRING); //TODO: compile this regex when the engine starts.
        return doParse(input, re, output);
    }

    bool validate(const Schema & schema) {
        //1. Check to make sure attributeName is a non-searchable attribute
        int attributeId = schema.getRefiningAttributeId(attributeName);
        if (attributeId < 0)
            return false;

        //2. Check the value to be consistent with type
        FilterType attributeType = schema.getTypeOfRefiningAttribute(
                attributeId);
        if (attributeValue.compare("*") != 0) {
            if (!validateValueWithType(attributeType, attributeValue)) {
                return false;
            }
            // now that it is validated, it should be changed to long representation.
            if(attributeType == srch2::instantsearch::ATTRIBUTE_TYPE_TIME){
            	std::stringstream buffer;
            	buffer << srch2is::DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(attributeValue);
            	attributeValue = buffer.str();
            }else{
				std::string attributeValueLowercase = attributeValue;
				std::transform(attributeValueLowercase.begin(), attributeValueLowercase.end(), attributeValueLowercase.begin(), ::tolower);
				attributeValue = attributeValueLowercase;
            }
        }
        return true;
    }

    bool evaluate(std::map<std::string, TypedValue> & nonSearchableAttributeValues) {

        // Compatible with SOLR : * means anything not empty.
        // Because the actual value can contain * if range bound is * we change it
        // to empty string and negate the variable negative.
        if (attributeValue.compare("*") == 0) {
            attributeValue = "";
            negative = !negative;
        }
        // first find the value coming from the record
        TypedValue value = nonSearchableAttributeValues[this->attributeName];

        if (attributeValue.compare("") == 0
                && (value.getType() == srch2is::ATTRIBUTE_TYPE_UNSIGNED
                        || value.getType() == srch2is::ATTRIBUTE_TYPE_FLOAT)) {
            attributeValue = value.minimumValue().toString() + "";
        }
        TypedValue valueToCheck;
        valueToCheck.setTypedValue(value.getType(), attributeValue);

        bool result = (value == valueToCheck);
        if (!negative) { // no '-' in the beginning of the field
            return result;
        } else {
            return !result;
        }
    }

    ~EqualityQueryExpression() {
    }

	string getUniqueStringForCaching() {
		stringstream ss;
		ss << attributeName.c_str();
		ss << attributeValue.c_str();
		ss << operation;
		ss << negative;
		return ss.str();
	}

private:
    std::string attributeName;
    string attributeValue;
    AttributeCriterionOperation operation;
    bool negative;
};

// this class gets a string which is an expression of nuon-searchable attribute names,it evluates the expression
// based on the TypedValue value of those attributes coming from records.
// NOTE: the expressions only must be based on UNSIGNED or FLOAT non-searchable attributes.
/*
 * Exmaple of ComplexQueryExpression is boolexp$price - discount < income * rate$
 * It accepts any kind of math expression which uses UNSIGNED/FLOAT non-searchable
 * attributes.
 */
class ComplexQueryExpression: public QueryExpression {
public:
    ComplexQueryExpression(
            std::vector<std::pair<MessageType, string> > *messages) {
        this->messages = messages;
    }
    /*
     * this function recevies a expression string.with a trailing '$'. ex. 'Price>10$ AND Popularity:[10 TO 100]'
     * it takes out the expression and stores that in this->parsedExpression.
     * for this example, this->parsedExpression will be 'Price>10' and the expressionString will be modified to 'AND Popularity:[10 TO 100]
     */
    bool parse(string &expressionString) {
        this->parsedExpression = "";
        bool isParsed = this->parseComplxExpression(expressionString,
                this->parsedExpression);
        if (!isParsed) {
            this->getMessageContainer()->push_back(
                    make_pair(MessageError, "Invalid expression query."));
            return false;
        }
        // expressionString is '$ AND Popularity:[10 TO 100]'. remove leading '$'
        expressionString = expressionString.substr(1); // expressionString is now ' AND Popularity:[10 TO 100]'
        boost::algorithm::trim(expressionString);
        // extract the expression, remove the 'boolexp(' and ')' part
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
                .getRefiningAttributes();

        for (std::map<std::string, unsigned>::const_iterator nonSearchableAttribute =
                nonSearchableAttributes->begin();
                nonSearchableAttribute != nonSearchableAttributes->end();
                ++nonSearchableAttribute) {
            // Since we only accept unsigned and float non-searchable attributes
            // this if-else statement only inserts these non-searchable-attributes into
            // the symbol table. This symbol table is passed to exprtk library.
            if (schema.getTypeOfRefiningAttribute(
                    nonSearchableAttribute->second)
                    == srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED
                    || schema.getTypeOfRefiningAttribute(
                            nonSearchableAttribute->second)
                            == srch2::instantsearch::ATTRIBUTE_TYPE_FLOAT) {
                symbolVariables.insert(
                        std::make_pair(nonSearchableAttribute->first, 0)); // zero is just a place holder, so that a variable is allocated in the vector
                symbolTable.add_variable(nonSearchableAttribute->first,
                        symbolVariables[nonSearchableAttribute->first], false);
            }
        }

        // now register the symbol table in the library
        expression.register_symbol_table(symbolTable);

        // now parse the string
        exprtk::parser<double> expressionParser;
        return expressionParser.compile(parsedExpression, expression);
    }

    bool evaluate(std::map<std::string, TypedValue> & nonSearchableAttributeValues) {
        // set values of variables
        for (std::map<std::string, double>::iterator symbol = symbolVariables
                .begin(); symbol != symbolVariables.end(); ++symbol) {
            symbol->second = nonSearchableAttributeValues[symbol->first]
                    .castToFloat();
        }

        float result = this->expression.value();

        if (result == 0) {
            return false;
        }
        return true;
    }

    ~ComplexQueryExpression() {
    }

	string getUniqueStringForCaching() {
		return parsedExpression.c_str();
	}

private:
    string parsedExpression;
    exprtk::expression<double> expression;
    exprtk::symbol_table<double> symbolTable;
    std::map<string, double> symbolVariables;

};
class FilterQueryEvaluator: public RefiningAttributeExpressionEvaluator {
public:

    FilterQueryEvaluator(
            std::vector<std::pair<MessageType, string> > *messages) {
        this->isFqBoolOperatorSet = false;
        this->messages = messages;
        this->termFQBooleanOperator = srch2::instantsearch::BooleanOperatorAND;
    }
    void setOperation(BooleanOperation op) {
        this->termFQBooleanOperator = op;
    }
    bool evaluate(std::map<std::string, TypedValue> & nonSearchableAttributeValues) {
        switch (this->termFQBooleanOperator) {
        case srch2::instantsearch::BooleanOperatorAND:
            for (std::vector<QueryExpression *>::iterator criterion =
                    expressions.begin(); criterion != expressions.end();
                    ++criterion) {
                QueryExpression * qe = *criterion;
                if (!qe->evaluate(nonSearchableAttributeValues)) {
                    return false;
                }
            }
            return true;
        case srch2::instantsearch::BooleanOperatorOR:
            for (std::vector<QueryExpression *>::iterator criterion =
                    expressions.begin(); criterion != expressions.end();
                    ++criterion) {
                QueryExpression * qe = *criterion;
                if (qe->evaluate(nonSearchableAttributeValues)) {
                    return true;
                }
            }
            return false;
        default:
            break;
        }
        ASSERT(false);
        return false;
    }

    bool validate(const Schema & schema) {
        for (std::vector<QueryExpression *>::iterator criterion = expressions
                .begin(); criterion != expressions.end(); ++criterion) {
            if (!(*criterion)->validate(schema)) {
                return false;
            }
        }
        return true;
    }
    /*
     * it looks to see if there is any post processing filter
     * if there is then it fills up the container accordingly
     * Detail: this function will check if a filterquery term is a range, equality or complex term.
     * it calls teh addCriterion method with appropreate trem type.
     * then, it looks for boolean operator, if found, it loops back and repeat the process for other terms.
     * example: 'fq=price:[10 TO 100] AND popularity:[* TO 100] AND Title:algorithm AND boolexp$popularity>20$'
     *
     */
    bool parseAndAddCriterion(string &fq) {
        bool parseNextTerm = true;
        while (parseNextTerm) {
            string fqField;
            /* check the length of fq
             * if lenght is 0 : it's an error.
             */
            if (fq.length() == 0) {
                // fq received here is of length 0
                // raise an error.
                Logger::info(
                        " Parsing error:: expecting filter query term, not found.");
                this->messages->push_back(
                        make_pair(MessageError,
                                "Parse error, expecting filter query term, not found."));
                return false;
            }
            // fq length is not 0.
            //NOTE:example related comments are only valid for the first time iteration inside the while loop
            bool hasParsedParameter = this->parseFqField(fq, fqField);
            if (!hasParsedParameter) {
                // it is not an equality nor a range expression
                // see if it's a complex expression
                string complexStr = "";
                hasParsedParameter = this->parseComplexExpression(fq, complexStr); // checks and removes the boolexp$ string returns true if found boolexp$
                if (!hasParsedParameter) {
                    parseNextTerm = false;
                    Logger::info(" Parsing error:: not a valid filter query");
                    this->messages->push_back(
                            make_pair(MessageError,
                                    "Parse error, not a valid filter query term."));
                    return false;
                }
                Logger::debug(
                        " 'boolexp$' found, possible complex expression query");
                string dummyField = "NO_FIELD";
                hasParsedParameter = this->addCriterion(fq,
                        FqKeywordTypeComplex, dummyField); // NO_FIELD, is a dummy parameter, that will not be used.
                if (!hasParsedParameter) {
                    return false;
                }
                boost::algorithm::trim(fq);
            } else {
                // hasParsedParameter is true, fq is now changed to : '[10 TO 100] AND popularity:[* TO 100] AND Title:algorithm AND boolexp$popularity>20$'
                // fqField is 'price:'
                // remove the ':' from 'price:'
                fqField = fqField.substr(0, fqField.length() - 1);
                boost::algorithm::trim(fqField);
                // check if it's a range query or assignment.
                if ('[' == fq.at(0)) {
                    Logger::debug(" '[' found, possible range expression");
                    string keyword = "";
                    fq = fq.substr(1);
                    hasParsedParameter = this->addCriterion(fq,
                            FqKeywordTypeRange, fqField); // it parses fq for range expression parameters
                    if (!hasParsedParameter) {
                        return false;
                    }
                } else {
                    Logger::debug(
                            " '[' not found, possible equality expression");
                    string keyword = "";
                    hasParsedParameter = this->addCriterion(fq,
                            FqKeywordTypeAssignment, fqField); // it parses fq for range expression parameters
                    if (!hasParsedParameter) {
                        return false;
                    }
                }
            }
            // so far a new term has been parsed.
            // now we will continue with parsing the next operator
            // fq should have been changed to 'AND popularity:[* TO 100] AND Title:algorithm AND boolexp$popularity>20$'
            string boolOperator = "";
            hasParsedParameter = this->parseFqBoolOperator(fq, boolOperator); //
            if (hasParsedParameter) {
                string msgStr = "boolean operator is " + boolOperator;
                Logger::debug(msgStr.c_str());
                parseNextTerm = true;
                Logger::debug("LOOPING AGAIN");
            } else {
                // no boolean operator found.
                // if the fq string length is > 0, throw an error.
                parseNextTerm = false;
                if (fq.length() > 0) {
                    // raise error message
                    Logger::info(
                            " Parsing error:: expecting boolean operator while parsing terms, not found.");
                    this->messages->push_back(
                            make_pair(MessageError,
                                    "Parse error, expecting boolean operator while parsing filter query terms."));
                    return false;
                }
            }
        }
        return true;
    }
    ~FilterQueryEvaluator() {
        for (std::vector<QueryExpression *>::iterator criterion = expressions
                .begin(); criterion != expressions.end(); ++criterion) {
            if (*criterion != NULL) {
                delete *criterion;
            }
        }
    }

	string toString(){
		stringstream ss;
		for(std::vector<QueryExpression *>::iterator queryExpression = expressions.begin();
				queryExpression != expressions.end() ; ++queryExpression){
			ss << (*queryExpression)->getUniqueStringForCaching().c_str();
		}
		return ss.str();
	}
private:
    // each expression is one term (one conjuct or disjunc) of the query
    // for example: for fq= price:[10 TO 100] AND model:JEEP, this vector contains
    // two elements.
    std::vector<QueryExpression *> expressions;
    // for example: for fq= price:[10 TO 100] AND model:JEEP, this member is AND
    bool isFqBoolOperatorSet; // to know if termFQBooleanOperator has been set or not
    std::vector<std::pair<MessageType, string> > *messages;
    BooleanOperation termFQBooleanOperator; // to store the boolean operator for the filterquery terms.

    bool parseFqField(string &input, string &field) {
        boost::regex re(FQ_FIELD_REGEX_STRING); //TODO: compile this regex when the engine starts.
        return doParse(input, re, field);
    }
    bool parseComplexExpression(string &input, string &output) {
        boost::regex re(COMPLEX_TERM_REGEX_STRING); //TODO: compile this regex when the engine starts.
        return doParse(input, re, output);
    }
    bool parseFqBoolOperator(string &input, string &output) {
        boost::regex re(FQ_TERM_BOOL_OP_REGEX_STRING); //TODO: compile this regex when the engine starts.
        bool hasBooleanOperator = doParse(input, re, output);
        if (!this->isFqBoolOperatorSet && hasBooleanOperator) {
            this->populateFilterQueryTermBooleanOperator(output);
            this->isFqBoolOperatorSet = true;
        } else if (this->isFqBoolOperatorSet && hasBooleanOperator) {
            // see if this boolean operator is compatible with the one that is already set.
            if (boost::iequals("OR", output)
                    && srch2::instantsearch::BooleanOperatorOR
                            == this->termFQBooleanOperator) {
                // do nothing.
            } else if (boost::iequals("AND", output)
                    && srch2::instantsearch::BooleanOperatorAND
                            == this->termFQBooleanOperator) {
                // do nothing
            } else {
                // raise warning.
                string termBoolOp = "OR";
                if (srch2::instantsearch::BooleanOperatorAND
                        == this->termFQBooleanOperator) {
                    termBoolOp = "AND";
                }
                this->messages->push_back(
                        make_pair(MessageWarning,
                                "we support either OR or AND but not both between filter query terms. Found"
                                        + output + ", ignoring it and using '+"
                                        + termBoolOp + "'."));
            }
        }
        return hasBooleanOperator;
    }
    void populateFilterQueryTermBooleanOperator(const string &termOperator) {
        /*
         * populates the termFQBooleanOperators in container.
         */
        Logger::debug("inside populateFilterQueryTermBooleanOperators.");
        if (boost::iequals("OR", termOperator)
                || termOperator.compare("||") == 0) {
            this->termFQBooleanOperator =
                    srch2::instantsearch::BooleanOperatorOR;
        } else if (boost::iequals("AND", termOperator)
                || termOperator.compare("&&") == 0) {
            this->termFQBooleanOperator =
                    srch2::instantsearch::BooleanOperatorAND;
        } else {
            // generate MessageWarning and use AND
            this->messages->push_back(
                    make_pair(MessageWarning,
                            "Invalid boolean operator specified as fq term boolean operator "
                                    + termOperator
                                    + ", ignoring it and using 'AND'."));
            this->termFQBooleanOperator =
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
            RangeQueryExpression * sre = new RangeQueryExpression(field,
                    this->messages);
            isParsable = sre->parse(criteriaString);
            if (!isParsable) {
                return false; // The message is already set in the parse method.
            } else {
                expressions.push_back(sre);
                return true;
            }
        }
        case FqKeywordTypeAssignment: {
            EqualityQueryExpression * sqe = new EqualityQueryExpression(field,
                    this->messages);
            isParsable = sqe->parse(criteriaString);
            if (!isParsable) {
                return false; // The message is already set in the parse method.
            } else {
                expressions.push_back(sqe);
                return true;
            }
        }
        case FqKeywordTypeComplex: {
            ComplexQueryExpression * cqe = new ComplexQueryExpression(
                    this->messages);
            isParsable = cqe->parse(criteriaString);
            if (!isParsable) {
                return false; // The message is already set in the parse method.
            } else {
                expressions.push_back(cqe);
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

#endif // __WRAPPER_FILTERQUERYEVALUATOR_H__
