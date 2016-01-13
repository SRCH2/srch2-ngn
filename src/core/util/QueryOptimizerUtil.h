
#ifndef __SRCH2_UTIL_QUERYOPTIMIZERUTIL_H__
#define __SRCH2_UTIL_QUERYOPTIMIZERUTIL_H__

#include <iostream>
using namespace std;

namespace srch2 {
namespace util {

class QueryOptimizerUtil {
public:
	/*
	 * Example :
	 * domains :             1,3,3
	 * currentProduct :      0,1,2
	 * nextProduct will be : 0,2,2 (0+1 !< 1 so gives us a carry which increments the middle 1 to 2)
	 */
	static void increment(unsigned numberOfDomains, unsigned * domains, unsigned * currentProduct , unsigned * outputProduct){

		unsigned carry = 1; // because we want to increment

		for(unsigned d = 0 ; d < numberOfDomains ; ++d){
			unsigned outputDigit = 0;
			if(currentProduct[d] + carry < domains[d]){
				outputDigit = currentProduct[d] + carry;
				carry = 0;
			}else{
				carry = 1;
			}
			outputProduct[d] = outputDigit;
		}
	}

	/*
	 * Example :
	 * numberOfDomains = 3;
	 * domains = [2,3,1]
	 * catProductResults should be [0,0,0 ,0,1,0 ,0,2,0 ,1,0,0 ,1,1,0 ,1,2,0]
	 */
	static void cartesianProduct(unsigned numberOfDomains, unsigned * domains, unsigned * cartProductResults , unsigned totalNumberOfProducts){
		// initialize the first product (all zero)
		for(unsigned d = 0; d < numberOfDomains ; ++d){
			cartProductResults[d] = 0;
		}
		unsigned * currentProduct = cartProductResults;
		for(unsigned p = 0; p < totalNumberOfProducts - 1 ; ++p){
			increment(numberOfDomains,domains,currentProduct + (numberOfDomains * p) , currentProduct + (numberOfDomains * (p+1)) );
		}
	}

	static void printIndentations(unsigned indent){
		if(indent < 1) return;
		for(int i=0;i<indent-1;i++){
			Logger::info("\t");
		}
		Logger::info("-\t");
	}

};


}
}


#endif // __SRCH2_UTIL_QUERYOPTIMIZERUTIL_H__
