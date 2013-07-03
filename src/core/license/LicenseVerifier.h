#ifndef __LICENSEVERIFIER_H__
#define __LICENSEVERIFIER_H__

#include <string>

namespace bimaple
{
namespace instantsearch
{

#define LicenseVerifier FooBar

class LicenseVerifier {
    public:
		static bool testWithEnvironmentVariable();
		static bool testFile(const std::string& filenameWithPath);

		static bool test(const std::string& license);
};
}}

#endif /* __LICENSEVERIFIER_H__ */
