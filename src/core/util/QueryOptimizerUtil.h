//$Id$

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
			cout << "\t" ;
		}
		cout << "-\t";
	}

};


}
}


#endif // __SRCH2_UTIL_QUERYOPTIMIZERUTIL_H__
