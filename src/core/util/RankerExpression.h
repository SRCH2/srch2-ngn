
// $Id: RankerExpression.h 3294 2013-05-01 03:45:51Z jiaying $

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

#ifndef RANKEREXPRESSION_H_
#define RANKEREXPRESSION_H_

#include "exprtk.hpp"
#include <string>
// NOT THREADSAFE
struct RankerExpression
{
    exprtk::expression<double> expression;
    exprtk::symbol_table<double> symbol_table;

    double doc_length;
    double doc_boost;
    double idf_score;

    std::string expr_string;

    RankerExpression(const std::string &expr_string)
    {
        this->doc_length = 1;
        this->doc_boost = 1;
        this->idf_score = 1;
        this->expr_string = expr_string;
        
        symbol_table.add_variable("doc_length", this->doc_length);
        symbol_table.add_variable("doc_boost", this->doc_boost);
        symbol_table.add_variable("idf_score", this->idf_score);
        
        expression.register_symbol_table(symbol_table);
        exprtk::parser<double> parser;
        if (!parser.compile(expr_string, expression))
        {
            parser.compile("1", expression); // Default ranking function in the case of bad ranking function.
        }
    }
    
    float applyExpression(const float &doc_length,
              const float &doc_boost,
              const float &idf_score)
    {
        this->doc_length = doc_length;
        this->doc_boost = doc_boost;
        this->idf_score = idf_score;
    
        /*
        std::cout << "doc_length:[" << this->doc_length << "] | "
                  << "doc_boost: [" << this->doc_boost  << "] | "
                  << "idf_score: [" << this->idf_score  << "] | "
                  << "expression: [" << this->expr_string  << "] | "
                  << std::endl;
        */
    
        double score = expression.value();

        //std::cout << "score:[" << score << "]" << std::endl;
    
        return static_cast<float>(score);
    }

    string getExpressionString() const
    {
        return expr_string;
    }
    
};

#endif /* RANKEREXPRESSION_H_ */
