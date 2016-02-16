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

#include "PhysicalOperators.h"
#include "UnionLowestLevelTermVirtualListOperator.h"
#include "UnionLowestLevelSimpleScanOperator.h"
#include "UnionLowestLevelSuggestionOperator.h"
#include "MergeTopKOperator.h"
#include "FilterQueryOperator.h"
#include "PhraseSearchOperator.h"
#include "FeedbackRankingOperator.h"

namespace srch2 {
namespace instantsearch {
PhysicalOperatorFactory::~PhysicalOperatorFactory(){
	for( vector<PhysicalPlanNode *>::iterator node = executionNodes.begin(); node != executionNodes.end(); ++node ){
		delete (*node);
	}
	for( vector<PhysicalPlanOptimizationNode *>::iterator node = optimizationNodes.begin(); node != optimizationNodes.end(); ++node ){
		delete (*node);
	}
}


RandomAccessVerificationTermOperator * PhysicalOperatorFactory::createRandomAccessVerificationTermOperator(){
	RandomAccessVerificationTermOperator * op = new RandomAccessVerificationTermOperator();
	executionNodes.push_back(op);
	return op;
}
RandomAccessVerificationTermOptimizationOperator * PhysicalOperatorFactory::createRandomAccessVerificationTermOptimizationOperator(){
	RandomAccessVerificationTermOptimizationOperator * op = new RandomAccessVerificationTermOptimizationOperator();
	optimizationNodes.push_back(op);
	return op;
}
RandomAccessVerificationAndOperator * PhysicalOperatorFactory::createRandomAccessVerificationAndOperator(){
	RandomAccessVerificationAndOperator * op = new RandomAccessVerificationAndOperator();
	executionNodes.push_back(op);
	return op;
}
RandomAccessVerificationAndOptimizationOperator * PhysicalOperatorFactory::createRandomAccessVerificationAndOptimizationOperator(){
	RandomAccessVerificationAndOptimizationOperator * op = new RandomAccessVerificationAndOptimizationOperator();
	optimizationNodes.push_back(op);
	return op;
}
RandomAccessVerificationOrOperator * PhysicalOperatorFactory::createRandomAccessVerificationOrOperator(){
	RandomAccessVerificationOrOperator * op = new RandomAccessVerificationOrOperator();
	executionNodes.push_back(op);
	return op;
}
RandomAccessVerificationOrOptimizationOperator * PhysicalOperatorFactory::createRandomAccessVerificationOrOptimizationOperator(){
	RandomAccessVerificationOrOptimizationOperator *  op = new RandomAccessVerificationOrOptimizationOperator();
	optimizationNodes.push_back(op);
	return op;
}
RandomAccessVerificationNotOperator * PhysicalOperatorFactory::createRandomAccessVerificationNotOperator(){
	RandomAccessVerificationNotOperator *  op = new RandomAccessVerificationNotOperator();
	executionNodes.push_back(op);
	return op;
}
RandomAccessVerificationNotOptimizationOperator * PhysicalOperatorFactory::createRandomAccessVerificationNotOptimizationOperator(){
	RandomAccessVerificationNotOptimizationOperator *  op = new RandomAccessVerificationNotOptimizationOperator();
	optimizationNodes.push_back(op);
	return op;
}
RandomAccessVerificationGeoOperator * PhysicalOperatorFactory::createRandomAccessVerificationGeoOperator(){
	RandomAccessVerificationGeoOperator * op = new RandomAccessVerificationGeoOperator();
	executionNodes.push_back(op);
	return op;
}
RandomAccessVerificationGeoOptimizationOperator * PhysicalOperatorFactory::createRandomAccessVerificationGeoOptimizationOperator(){
	RandomAccessVerificationGeoOptimizationOperator * op = new RandomAccessVerificationGeoOptimizationOperator();
	optimizationNodes.push_back(op);
	return op;
}
SortByIdOperator * PhysicalOperatorFactory::createSortByIdOperator(){
	SortByIdOperator *  op = new SortByIdOperator();
	executionNodes.push_back(op);
	return op;
}
SortByIdOptimizationOperator * PhysicalOperatorFactory::createSortByIdOptimizationOperator(){
	SortByIdOptimizationOperator *  op = new SortByIdOptimizationOperator();
	optimizationNodes.push_back(op);
	return op;
}
SortByScoreOperator* PhysicalOperatorFactory::createSortByScoreOperator(){
	SortByScoreOperator*  op = new SortByScoreOperator();
	executionNodes.push_back(op);
	return op;
}
SortByScoreOptimizationOperator* PhysicalOperatorFactory::createSortByScoreOptimizationOperator(){
	SortByScoreOptimizationOperator*  op = new SortByScoreOptimizationOperator();
	optimizationNodes.push_back(op);
	return op;
}
MergeTopKOperator * PhysicalOperatorFactory::createMergeTopKOperator(){
	MergeTopKOperator *  op =  new MergeTopKOperator();
	executionNodes.push_back(op);
	return op;
}
MergeTopKOptimizationOperator * PhysicalOperatorFactory::createMergeTopKOptimizationOperator(){
	MergeTopKOptimizationOperator *  op = new MergeTopKOptimizationOperator();
	optimizationNodes.push_back(op);
	return op;
}
MergeSortedByIDOperator * PhysicalOperatorFactory::createMergeSortedByIDOperator(){
	MergeSortedByIDOperator *  op = new MergeSortedByIDOperator();
	executionNodes.push_back(op);
	return op;
}
MergeSortedByIDOptimizationOperator * PhysicalOperatorFactory::createMergeSortedByIDOptimizationOperator(){
	MergeSortedByIDOptimizationOperator *  op = new MergeSortedByIDOptimizationOperator();
	optimizationNodes.push_back(op);
	return op;
}
MergeByShortestListOperator * PhysicalOperatorFactory::createMergeByShortestListOperator(){
	MergeByShortestListOperator * op = new MergeByShortestListOperator();
	executionNodes.push_back(op);
	return op;
}
MergeByShortestListOptimizationOperator * PhysicalOperatorFactory::createMergeByShortestListOptimizationOperator(){
	MergeByShortestListOptimizationOperator *  op = new MergeByShortestListOptimizationOperator();
	optimizationNodes.push_back(op);
	return op;
}
UnionSortedByIDOperator * PhysicalOperatorFactory::createUnionSortedByIDOperator(){
	UnionSortedByIDOperator *  op = new UnionSortedByIDOperator();
	executionNodes.push_back(op);
	return op;
}
UnionSortedByIDOptimizationOperator * PhysicalOperatorFactory::createUnionSortedByIDOptimizationOperator(){
	UnionSortedByIDOptimizationOperator *  op = new UnionSortedByIDOptimizationOperator();
	optimizationNodes.push_back(op);
	return op;
}
UnionLowestLevelTermVirtualListOperator * PhysicalOperatorFactory::createUnionLowestLevelTermVirtualListOperator(){
	UnionLowestLevelTermVirtualListOperator * op = new UnionLowestLevelTermVirtualListOperator();
	executionNodes.push_back(op);
	return op;
}
UnionLowestLevelTermVirtualListOptimizationOperator * PhysicalOperatorFactory::createUnionLowestLevelTermVirtualListOptimizationOperator(){
	UnionLowestLevelTermVirtualListOptimizationOperator *  op = new UnionLowestLevelTermVirtualListOptimizationOperator();
	optimizationNodes.push_back(op);
	return op;
}
UnionLowestLevelSimpleScanOperator * PhysicalOperatorFactory::createUnionLowestLevelSimpleScanOperator(){
	UnionLowestLevelSimpleScanOperator * op = new UnionLowestLevelSimpleScanOperator();
	executionNodes.push_back(op);
	return op;
}
UnionLowestLevelSimpleScanOptimizationOperator * PhysicalOperatorFactory::createUnionLowestLevelSimpleScanOptimizationOperator(){
	UnionLowestLevelSimpleScanOptimizationOperator *  op = new UnionLowestLevelSimpleScanOptimizationOperator();
	optimizationNodes.push_back(op);
	return op;
}

UnionLowestLevelSuggestionOperator * PhysicalOperatorFactory::createUnionLowestLevelSuggestionOperator(){
	UnionLowestLevelSuggestionOperator * op = new UnionLowestLevelSuggestionOperator();
	executionNodes.push_back(op);
	return op;
}
UnionLowestLevelSuggestionOptimizationOperator * PhysicalOperatorFactory::createUnionLowestLevelSuggestionOptimizationOperator(){
	UnionLowestLevelSuggestionOptimizationOperator *  op = new UnionLowestLevelSuggestionOptimizationOperator();
	optimizationNodes.push_back(op);
	return op;
}

FilterQueryOperator * PhysicalOperatorFactory::createFilterQueryOperator(RefiningAttributeExpressionEvaluator * filterQueryEvaluator, string & roleId){
	FilterQueryOperator * filterQueryOp = new FilterQueryOperator(filterQueryEvaluator, roleId);
	executionNodes.push_back(filterQueryOp);
	return filterQueryOp;
}

FilterQueryOptimizationOperator * PhysicalOperatorFactory::createFilterQueryOptimizationOperator(){
	FilterQueryOptimizationOperator * filterQueryOpOp = new FilterQueryOptimizationOperator();
	optimizationNodes.push_back(filterQueryOpOp);
	return filterQueryOpOp;
}

PhraseSearchOperator * PhysicalOperatorFactory::createPhraseSearchOperator(PhraseInfo * phraseSearchInfo) {
	PhraseSearchOperator * object = new PhraseSearchOperator(*phraseSearchInfo);
	executionNodes.push_back(object);
	return object;
}
PhraseSearchOptimizationOperator * PhysicalOperatorFactory::createPhraseSearchOptimzationOperator() {
	PhraseSearchOptimizationOperator * object = new PhraseSearchOptimizationOperator();
	optimizationNodes.push_back(object);
	return object;
}

GeoNearestNeighborOperator * PhysicalOperatorFactory::createGeoNearestNeighborOperator(){
	GeoNearestNeighborOperator * op = new GeoNearestNeighborOperator();
	executionNodes.push_back(op);
	return op;
}
GeoNearestNeighborOptimizationOperator * PhysicalOperatorFactory::createGeoNearestNeighborOptimizationOperator(){
	GeoNearestNeighborOptimizationOperator * op = new GeoNearestNeighborOptimizationOperator();
	optimizationNodes.push_back(op);
	return op;
}
GeoSimpleScanOperator * PhysicalOperatorFactory::createGeoSimpleScanOperator(){
	GeoSimpleScanOperator * op = new GeoSimpleScanOperator();
	executionNodes.push_back(op);
	return op;
}
GeoSimpleScanOptimizationOperator * PhysicalOperatorFactory::createGeoSimpleScanOptimizationOperator(){
	GeoSimpleScanOptimizationOperator * op = new GeoSimpleScanOptimizationOperator();
	optimizationNodes.push_back(op);
	return op;
}
}
}
