

#include "QueryOptimizer.h"


namespace srch2 {
namespace instantsearch {

QueryOptimizer::QueryOptimizer(ForwardIndex * forwardIndex ,
		InvertedIndex * invertedIndex,
		Trie * trie,
		CatalogManager * catalogManager,
		const LogicalPlan * logicalPlan): logicalPlan(logicalPlan){
	this->forwardIndex = forwardIndex;
	this->invertedIndex = invertedIndex;
	this->trie = trie;
	this->catalogManager = catalogManager;
}

/*
 *
 * this function builds PhysicalPlan and optimizes it
 * ---- 1. Builds the Physical plan by mapping each Logical operator to a/multiple Physical operator(s)
 *           and makes sure inputs and outputs of operators are consistent.
 * ---- 2. Applies optimization rules on the physical plan
 */
void QueryOptimizer::buildAndOptimizePhysicalPlan(PhysicalPlan & physicalPlan){

	// Build physical plan
	buildPhysicalPlanFirstVersion(physicalPlan);

	// apply optimization rules
	applyOptimizationRulesOnThePlan(physicalPlan);

}


/*
 * This function maps LogicalPlan nodes to Physical nodes and builds a very first
 * version of the PhysicalPlan. This plan will optimizer in next steps.
 */
void QueryOptimizer::buildPhysicalPlanFirstVersion(PhysicalPlan & physicalPlan){
	//TODO
}

/*
 * this function applies optimization rules (funtions starting with Rule_) on the plan one by one
 */
void QueryOptimizer::applyOptimizationRulesOnThePlan(PhysicalPlan & physicalPlan){
	//TODO
	// calls the optimization rules one by one
	Rule_1(physicalPlan);
	//Rule_2(physicalPlan);
	//Rule_3(physicalPlan);
	//...
}

/*
 * An example of an optimization rule function.
 */
void QueryOptimizer::Rule_1(PhysicalPlan & physicalPlan){
	//TODO
}

}
}
