// $Id: Analyzer.cpp 3219 2013-03-25 23:36:34Z iman $

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

#include "AnalyzerInternal.h"
#include "StandardAnalyzer.h"
#include "SimpleAnalyzer.h"

namespace srch2 {
namespace instantsearch {
// TODO: remove create. The constructor is called directly
Analyzer *Analyzer::create( const StemmerNormalizerFlagType &stemmerFlag,
							const std::string &stemmerFilePath,
							const std::string &stopWordFilePath,
							const std::string &synonymFilePath, const std::string &delimiters,
							const AnalyzerType &analyzerType) {

	switch (analyzerType) {
	case SIMPLE_ANALYZER:
		return new SimpleAnalyzer(stemmerFlag,
								  stemmerFilePath,
							   	  stopWordFilePath,
								  synonymFilePath,
								  delimiters);
	default:
		return new StandardAnalyzer(stemmerFlag,
									stemmerFilePath,
									stopWordFilePath,
									synonymFilePath,
									delimiters);
	}
}

}
}
