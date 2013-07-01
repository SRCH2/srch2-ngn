/*
 * StandardAnalyzer.cpp
 *
 *  Created on: 2013-5-17
 */

#include "StandardAnalyzer.h"
#include "StandardTokenizer.h"
#include "LowerCaseFilter.h"

namespace srch2
{
namespace instantsearch
{

// create operator flow and link share pointer to the data
TokenOperator * StandardAnalyzer::createOperatorFlow()
{
	TokenOperator *tokenOperator = new StandardTokenizer();
	tokenOperator = new LowerCaseFilter(tokenOperator);
	this->sharedToken = tokenOperator->sharedToken;
	return tokenOperator;
}

StandardAnalyzer::~StandardAnalyzer() {
	// TODO Auto-generated destructor stub
}



}}
