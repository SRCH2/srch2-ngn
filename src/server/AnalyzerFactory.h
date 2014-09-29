/*
 * AnalyzerFactory.h
 *
 *  Created on: Aug 21, 2013
 *      Author: sbisht
 */

#ifndef __WRAPPER_ANALYZERFACTORY_H_
#define __WRAPPER_ANALYZERFACTORY_H_

namespace srch2 { namespace instantsearch { class Analyzer; } }
namespace srch2is = srch2::instantsearch;

namespace srch2 {
    namespace httpwrapper {

        class CoreInfo_t;

        class AnalyzerFactory {
        public:
            static srch2is::Analyzer* createAnalyzer(const CoreInfo_t* config, bool isSearcherThread = false);
            static srch2is::Analyzer* getCurrentThreadAnalyzer(const CoreInfo_t* config);
            static srch2is::Analyzer* getCurrentThreadAnalyzerWithSynonyms(const CoreInfo_t* config);
        private:
            AnalyzerFactory();
        };

        class AnalyzerHelper {
		public:
			static void initializeAnalyzerResource(const CoreInfo_t* conf);
			static void loadAnalyzerResource(const CoreInfo_t* conf);
			static void saveAnalyzerResource(const CoreInfo_t* conf);
		private:
			AnalyzerHelper(){}
		};
    } // namespace wrapper
} /* namespace srch2 */
#endif /* __WRAPPER_ANALYZERFACTORY_H__ */
