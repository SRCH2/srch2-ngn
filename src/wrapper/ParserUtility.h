

#ifndef _WRAPPER_PARSERUTILITY_H_
#define _WRAPPER_PARSERUTILITY_H_
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

inline bool isInteger(const std::string & s)
{
   if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false ;

   char * p ;
   strtol(s.c_str(), &p, 10) ;

   return (*p == 0) ;
}



inline bool isFloat(const std::string & s)
{
   if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false ;

   char * p ;
   strtof(s.c_str(), &p);

   return (*p == 0) ;
}

inline bool isTime(const std::string & s)
{
	return true; // FIXME ???????????????????????????????
}


const locale inputs[] = {
    locale(locale::classic(), new time_input_facet("%m/%d/%Y")),
    locale(locale::classic(), new time_input_facet("%Y-%m-%d %H:%M:%S")),
    locale(locale::classic(), new time_input_facet("%Y%m%d%H%M%S")),
    locale(locale::classic(), new time_input_facet("%Y%m%d%H%M")),
    locale(locale::classic(), new time_input_facet("%Y%m%d")) };
const size_t formats = sizeof(inputs)/sizeof(inputs[0]);

time_t ptime_to_time_t(boost::posix_time::ptime t)
{
       static boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
       return (t-epoch).ticks() / boost::posix_time::time_duration::ticks_per_second();
}
int main()
{
       std::string msg = "2010-08-04 08:34:12";

       for(size_t i=0; i<formats; ++i)
       {
           std::istringstream ss(msg);
           ss.imbue(inputs[i]);
           boost::posix_time::ptime this_time;
           ss >> this_time;

           if(this_time != boost::posix_time::not_a_date_time)
               std::cout << this_time << " or " << ptime_to_time_t(this_time) << std::endl;
       }
}

}
}


#endif // _WRAPPER_PARSERUTILITY_H_
