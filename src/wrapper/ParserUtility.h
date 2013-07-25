

#ifndef _WRAPPER_PARSERUTILITY_H_
#define _WRAPPER_PARSERUTILITY_H_
#include <string>

using namespace std;

namespace srch2
{
namespace httpwrapper
{



void parsePair(const string & input, string * key, string * value){

}


// trim from start
static inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}

static inline std::string &removeWhiteSpace(std::string &s) {
	s.erase( std::remove_if( s.begin(), s.end(), ::isspace ), s.end() );
	return s;
}

static inline std::vector<std::string>  &split(std::string &s, std::string delimiter){
	std::vector<std::string>  result;
	std::string source = s;


	std::string::size_type lastpos = source.find(delimiter , 0);

	if(lastpos == std::string::npos){
		result.push_back(source);
		return result;
	}
	if(lastpos != 0){
		result.push_back(source.substr(0,lastpos));
		source = source.substr(lastpos, source.size()-lastpos);
	}
	// Skip delimiters at beginning.
	lastpos = lastpos + delimiter.size();
	// Find first "non-delimiter".
	std::string::size_type pos  = source.find(delimiter , lastpos);

	while (std::string::npos != pos && std::string::npos != lastpos)
	{
		// Found a token, add it to the vector.
		result.push_back(source.substr(lastpos, pos - lastpos));
		// Skip delimiters.  Note the "not_of"
		lastpos = lastpos + delimiter.size();
		// Find next "non-delimiter"
		pos = source.find(delimiter , lastpos);
	}
	return result;

}

}
}


#endif // _WRAPPER_PARSERUTILITY_H_
