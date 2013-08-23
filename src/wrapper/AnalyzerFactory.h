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

        class ConfigManager;

        class AnalyzerFactory {
        public:
            static srch2is::Analyzer* createAnalyzer(const ConfigManager* conf);
        private:
            AnalyzerFactory();
        };

        class AnalyzerHelper {
		public:
			static void initializeAnalyzerResource(const ConfigManager* conf);
			static void loadAnalyzerResource(const ConfigManager* conf);
			static void saveAnalyzerResource(const ConfigManager* conf);
		private:
			AnalyzerHelper(){}
		};
    } // namespace wrapper
} /* namespace srch2 */
#endif /* __WRAPPER_ANALYZERFACTORY_H__ */
