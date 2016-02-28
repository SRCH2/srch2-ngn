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
            Logger::warn("Ranking expression defined in config file is not valid, so the engine will use the default expression");
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
    
        double score = expression.value();

        return static_cast<float>(score);
    }

    string getExpressionString() const
    {
        return expr_string;
    }
    
};

#endif /* RANKEREXPRESSION_H_ */
