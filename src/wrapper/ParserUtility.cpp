

#include "ParserUtility.h"
#include <string>
#include <cstdlib>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>


using boost::posix_time::time_input_facet;
using std::locale;


using namespace std;

namespace srch2
{
namespace httpwrapper
{





// trim from start
std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}

std::string &removeWhiteSpace(std::string &s) {
	s.erase( std::remove_if( s.begin(), s.end(), ::isspace ), s.end() );
	return s;
}

std::vector<std::string>  &split(std::string &s, std::string delimiter){
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

bool isInteger(const std::string & s)
{
   if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false ;

   char * p ;
   strtol(s.c_str(), &p, 10) ;

   return (*p == 0) ;
}



bool isFloat(const std::string & s)
{
   if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false ;

   char * p ;
   strtof(s.c_str(), &p);

   return (*p == 0) ;
}

bool isTime(const std::string & s)
{
	if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false ;

	char * p ;
	strtol(s.c_str(), &p, 10) ;

	return (*p == 0) ;
}


time_t convertPtimeToTimeT(boost::posix_time::ptime t)
{
       static boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
       return (t-epoch).ticks() / boost::posix_time::time_duration::ticks_per_second();
}

}
}
