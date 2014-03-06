
#include "PhysicalOperators.h"
#include "UnionLowestLevelTermVirtualListOperator.h"
#include "UnionLowestLevelSimpleScanOperator.h"
#include "UnionLowestLevelSuggestionOperator.h"
#include "MergeTopKOperator.h"
#include "FilterQueryOperator.h"
#include "PhraseSearchOperator.h"

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

FilterQueryOperator * PhysicalOperatorFactory::createFilterQueryOperator(RefiningAttributeExpressionEvaluator * filterQueryEvaluator){
	FilterQueryOperator * filterQueryOp = new FilterQueryOperator(filterQueryEvaluator);
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

}
}
