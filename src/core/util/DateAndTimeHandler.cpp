


#include "DateAndTimeHandler.h"
#include "instantsearch/DateTime.h"
#include <iostream>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <cstdlib>
#include <util/Assert.h>

namespace srch2 {
namespace instantsearch {

/*
 * Example:
 * timeString = 02/03/2000
 * output will be the number of seconds from 01/01/1970 to timeString.
 * The format of this string must match one of the followings :
 * 		"%H:%M:%S",
        "%m/%d/%Y",
        "%Y-%m-%d %H:%M:%S",
        "%Y%m%d%H%M%S",
        "%Y%m%d%H%M",
        "%Y%m%d"
 * or
 * 		"NOW" >> which will be interpreted as the current time.
 */
time_t DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(const string & timeString)
{
    // 1. first check with the known point of time formats we have
    bt::ptime pt;
    for(size_t i=0; i<localeFormatsPointOfTime; ++i)
    {
        boost::regex regexEngine(regexInputsPointOfTime[i]);
        boost::smatch whatIsMatched;
        if(boost::regex_match(timeString, whatIsMatched, regexEngine, boost::match_extra))
        {
        	// This piece of code is taken from :
        	// http://stackoverflow.com/questions/2612343/basic-boost-date-time-input-format-question
        	std::istringstream is(timeString);
        	is.imbue(localeInputsPointOfTime[i]);
        	is >> pt;
        	if(pt != bt::not_a_date_time){
        		return convertPtimeToSecondsFromEpoch(pt);
        	}
        }
    }

    // 4. check to see if it is a time relative to NOW (right now we only support now)
    if(timeString.compare("NOW") == 0){ // it's NOW
        return convertPtimeToSecondsFromEpoch(boost::posix_time::second_clock::local_time());
    }
    return -1;
}

/*
 * this function prints the date determined by secondsFromEpoch seconds from 1/1/1970
 * in a proper human readable format.
 */
string DateAndTimeHandler::convertSecondsFromEpochToDateTimeString(register const time_t * secondsFromEpoch){

    ostringstream oss;
    boost::posix_time::ptime pTime = boost::posix_time::from_time_t(*secondsFromEpoch);
    oss << pTime;
    return oss.str() ;
}

/*
 * This functions verifies whether the input string is compatible with time formats that we have or not.
 */
bool DateAndTimeHandler::verifyDateTimeString(const string & timeStringInput, DateTimeType dateTimeType){
	if(timeStringInput.compare("") == 0){
		return false;
	}
	string timeString = boost::to_upper_copy(timeStringInput);
    switch (dateTimeType) {
    	// timeString == NOW
        case DateTimeTypeNow:
            if(timeString.compare("NOW") == 0){
                return true;
            }else{
                return false;
            }
        // timeString exmaple : 02/03/2001 12:23:34
        case DateTimeTypePointOfTime:{
            for(size_t i=0; i<localeFormatsPointOfTime; ++i){
                boost::regex regexEngine(regexInputsPointOfTime[i]);
                boost::smatch whatIsMatched;
                if(boost::regex_match(timeString, whatIsMatched, regexEngine, boost::match_extra)){
                    return true;
                }
            }
            return false;
        }
        // timeString example : 2YEARS or 12:01:01 (12 hours and 1 minutes and 1 seconds)
        case DateTimeTypeDurationOfTime:{
        	// 1. first see if it's a known duration format
            for(size_t i=0; i<localeFormatsDurationOfTime; ++i){
                boost::regex regexEngine(regexInputsDurationOfTime[i]);
                boost::smatch whatIsMatched;
                if(boost::regex_match(timeString, whatIsMatched, regexEngine, boost::match_extra)){
                    return true;
                }
            }
            // 2. second check to see if it is a constant
            if(std::find(DURATION_OF_TIME_CONSTANTS.begin() , DURATION_OF_TIME_CONSTANTS.end() , timeString) != DURATION_OF_TIME_CONSTANTS.end()){
            	return true;
            }
            // 3. third, check to see if it's a combination of number and constant
            // first make the regex string
            // The regex will be like "^(\\d*SECOND|\\d*SECONDS|\\d*MINUTE|\d*MINUTES)$"
            // which will be matched with any string like 23DAYS
            string format = "^(";
            for(vector<string>::const_iterator cons = DURATION_OF_TIME_CONSTANTS.begin() ; cons != DURATION_OF_TIME_CONSTANTS.end() ; ++cons){
                format+= "\\d*"+*cons + "|";
            }
            format += ")$";
            boost::regex regexEngine2(format);
            boost::smatch whatIsMatched2;
            if(boost::regex_match(timeString, whatIsMatched2, regexEngine2, boost::match_extra)){ // string matches this pattern
                return true;
            }
            return false;
        }
        default:
            return false;
    }
}

/*
 * This function converts a ptime (boost date/time object) to the number of seconds
 * from 01/01/1970.
 */
time_t DateAndTimeHandler::convertPtimeToSecondsFromEpoch(boost::posix_time::ptime t){
    static boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
    return (t-epoch).ticks() / boost::posix_time::time_duration::ticks_per_second();
}

/*
 * This function converts the number of seconds from 01/01/1970 to a ptime
 * (boost library date/time object) object.
 */
boost::posix_time::ptime DateAndTimeHandler::convertSecondsFromEpochToPTime(time_t t){
    return boost::posix_time::from_time_t(t);
}

/*
 * In this function the string should be parsed and TimeDuration object must be built
 */
TimeDuration DateAndTimeHandler::convertDurationTimeStringToTimeDurationObject(const string & timeStringInput){
	string timeString = boost::to_upper_copy(timeStringInput);
    // 1. if it is a constant, parse and save as a TimeDuration object
	vector<string>::const_iterator constantIter = std::find(DURATION_OF_TIME_CONSTANTS.begin() , DURATION_OF_TIME_CONSTANTS.end() , timeString);
	if(constantIter != DURATION_OF_TIME_CONSTANTS.end()){
		timeString = "1"+timeString;
	}
    // 2. if it's a combination of number and constant
    // first make the regex string
    string format = "^(";
    for(vector<string>::const_iterator cons = DURATION_OF_TIME_CONSTANTS.begin() ; cons != DURATION_OF_TIME_CONSTANTS.end() ; ++cons){
        format+= "\\d*"+*cons + "|";
    }
    format += ")$";
    boost::regex regexEngine2(format);
    boost::smatch whatIsMatched2;
    if(boost::regex_match(timeString, whatIsMatched2, regexEngine2, boost::match_extra)){ // string matches this pattern
        for(vector<string>::const_iterator cons = DURATION_OF_TIME_CONSTANTS.begin() ; cons != DURATION_OF_TIME_CONSTANTS.end() ; ++cons){
        	string constant = *cons;
        	/*
        	 * Example : 12WEEKS or 1SECOND
        	 */
        	// The constant must match the end of timeString. Also the size of timeString must be larger than constant because
        	// we know certainly that it starts with a number.
            if(timeString.length() > constant.length() &&
            		0 == timeString.compare (timeString.length() - constant.length(), constant.length(), constant)){
            	int numberOfThisUnit = boost::lexical_cast<int>(timeString.substr(0,timeString.size() - constant.size()));
            	if(constant.compare("SECOND") == 0 || constant.compare("SECONDS") == 0  ){
            		TimeDuration td;
            		td.secondMinuteHourDuration = boost::posix_time::seconds(numberOfThisUnit);
            		return td;
            	}
            	if(constant.compare("MINUTE") == 0 || constant.compare("MINUTES") == 0  ){
            		TimeDuration td;
            		td.secondMinuteHourDuration = boost::posix_time::minutes(numberOfThisUnit);
            		return td;
            	}
            	if(constant.compare("HOUR") == 0 || constant.compare("HOURS") == 0  ){
            		TimeDuration td;
            		td.secondMinuteHourDuration = boost::posix_time::hours(numberOfThisUnit);
            		return td;
            	}
            	if(constant.compare("DAY") == 0 || constant.compare("DAYS") == 0  ){
            		TimeDuration td;
            		td.dayWeekDuration = boost::gregorian::days(numberOfThisUnit);
            		return td;
            	}
            	if(constant.compare("WEEK") == 0 || constant.compare("WEEKS") == 0  ){
            		TimeDuration td;
            		td.dayWeekDuration = boost::gregorian::weeks(numberOfThisUnit);
            		return td;
            	}
            	if(constant.compare("MONTH") == 0 || constant.compare("MONTHS") == 0  ){
            		TimeDuration td;
            		td.monthDuration = boost::gregorian::months(numberOfThisUnit);
            		return td;
            	}
            	if(constant.compare("YEAR") == 0 || constant.compare("YEARS") == 0  ){
            		TimeDuration td;
            		td.yearDuration = boost::gregorian::years(numberOfThisUnit);
            		return td;
            	}
            }
        }
    }
	// 3. if it's a known duration format
    // Example : 12:13:14 meaning a gap of 12 hours and 13 minutes and 14 seconds
    for(size_t i=0; i<localeFormatsDurationOfTime; ++i){
        boost::regex regexEngine(regexInputsDurationOfTime[i]);
        boost::smatch whatIsMatched;
        if(boost::regex_match(timeString, whatIsMatched, regexEngine, boost::match_extra)){
            TimeDuration td;
            td.secondMinuteHourDuration = boost::posix_time::duration_from_string(timeString);
            return td;
        }
    }

    TimeDuration td;
    return td;
}

vector<string> DateAndTimeHandler::initializeConstants(){
    vector<string> constants;
    constants.push_back("SECOND");
    constants.push_back("MINUTE");
    constants.push_back("HOUR");
    constants.push_back("DAY");
    constants.push_back("WEEK");
    constants.push_back("MONTH");
    constants.push_back("YEAR");

    constants.push_back("SECONDS");
    constants.push_back("MINUTES");
    constants.push_back("HOURS");
    constants.push_back("DAYS");
    constants.push_back("WEEKS");
    constants.push_back("MONTHS");
    constants.push_back("YEARS");
    return constants;
}
// This regex array is parallel to localeInputsDurationOfTime and that variable is a good
// explanation for this array
const string DateAndTimeHandler::regexInputsDurationOfTime[] = {
        "^\\d{2}:\\d{2}:\\d{2}$"
};
const locale DateAndTimeHandler::localeInputsDurationOfTime[] ={
        locale(locale::classic(), new time_input_facet("%H:%M:%S"))
};
const size_t DateAndTimeHandler::localeFormatsDurationOfTime =
        sizeof(DateAndTimeHandler::localeInputsDurationOfTime)/sizeof(DateAndTimeHandler::localeInputsDurationOfTime[0]);

// This regex array is parallel to localeInputsPointOfTime and that variable is a good
// explanation for this array
// NOTE related to multi-valued attributes :
// Date/Time formats should NOT contain ',' in them.
// ',' is kept in constant Constant.h::MULTI_VALUED_ATTRIBUTES_VALUE_DELIMITER
const string DateAndTimeHandler::regexInputsPointOfTime[] = {
		"^\\d{2}:\\d{2}:\\d{2}$",
        "^\\d{2}/\\d{2}/\\d{4}$",
        "^\\d{4}-\\d{2}-\\d{2}\\s\\d{2}:\\d{2}:\\d{2}$",
        "^\\d{4}-\\d{2}-\\d{2}$",
        "^\\d{4}\\d{2}\\d{2}\\s\\d{2}\\d{2}\\d{2}$",
        "^\\d{4}-\\d{2}-\\d{2}\\s\\d{2}:\\d{2}$",
        "^\\d{4}-\\d{2}-\\d{2}$"
};

const locale DateAndTimeHandler::localeInputsPointOfTime[] ={
		locale(locale::classic(), new time_input_facet("%H:%M:%S")),
        locale(locale::classic(), new time_input_facet("%m/%d/%Y")),
        locale(locale::classic(), new time_input_facet("%Y-%m-%d %H:%M:%S")),
        locale(locale::classic(), new time_input_facet("%Y-%m-%d")),
        locale(locale::classic(), new time_input_facet("%Y%m%d%H%M%S")),
        locale(locale::classic(), new time_input_facet("%Y%m%d%H%M")),
        locale(locale::classic(), new time_input_facet("%Y%m%d"))
};
const size_t DateAndTimeHandler::localeFormatsPointOfTime =
        sizeof(DateAndTimeHandler::localeInputsPointOfTime)/sizeof(DateAndTimeHandler::localeInputsPointOfTime[0]);

const vector<string> DateAndTimeHandler::DURATION_OF_TIME_CONSTANTS = DateAndTimeHandler::initializeConstants();

/*
 * This function implements operator + for (TimeDuration + ptime)
 * This means if TimeDuration is on left and ptime is on right this function
 * returns a ptime which is equal to input + TimeDuration
 */
boost::posix_time::ptime TimeDuration::operator+(const boost::posix_time::ptime & pTime) const{
	boost::posix_time::ptime timeResult;
	timeResult = pTime;

	timeResult = timeResult + secondMinuteHourDuration;
	timeResult = timeResult + dayWeekDuration;
	timeResult = timeResult + monthDuration;
	timeResult = timeResult + yearDuration;

	return timeResult;
}

/*
 * This function implements operator + for (TimeDuration + long)
 * This means if TimeDuration is on left and ling (number of seconds from epoch) is on right this function
 * returns a ptime which is equal to input + TimeDuration
 */
long TimeDuration::operator+(const long pTime) const{
	boost::posix_time::ptime pTimeObj = DateAndTimeHandler::convertSecondsFromEpochToPTime(pTime);
	return DateAndTimeHandler::convertPtimeToSecondsFromEpoch(*this + pTimeObj); // it uses the other implementation for operator +
}

/*
 * This function adds two TimeDuration objects to make a larger interval.
 */
TimeDuration TimeDuration::operator+(const TimeDuration & timeDuration) const{
	TimeDuration result;

	result.secondMinuteHourDuration = this->secondMinuteHourDuration + timeDuration.secondMinuteHourDuration;
	result.dayWeekDuration = this->dayWeekDuration + timeDuration.dayWeekDuration;
	result.monthDuration = this->monthDuration + timeDuration.monthDuration;
	result.yearDuration = this->yearDuration + timeDuration.yearDuration;


	return result;
}

string TimeDuration::toString() const{
	ASSERT(false);
	return "WARNING!!!!";
}


}
}
