


#include <iostream>
#include "DateAndTimeHandler.h"
#include "instantsearch/DateTime.h"
#include <boost/regex.hpp>
#include <cstdlib>


namespace srch2 {
namespace instantsearch {

time_t DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(const string & timeString)
{
    // 1. first check with the known point of time formats we have
    bt::ptime pt;
    for(size_t i=0; i<localeFormatsPointOfTime; ++i)
    {
        boost::regex e(regexInputsPointOfTime[i]);
        boost::smatch what;
        if(boost::regex_match(timeString, what, e, boost::match_extra))
        {
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
string DateAndTimeHandler::convertSecondsFromEpochToDateTimeString(register const time_t * secondsFromEpoch){

//    struct tm * ptm = gmtime(secondsFromEpoch);
    ostringstream oss;
    /*
     * 1800 is the beginning of time, for those time constants like DAY, since we don't enter any year,
     * their year is set to 1800. This is how we determine this time should be printed as a time gap or a time point
     *
     * 1400 is the lower bound of boost time library. For HH:mm:ss format the year is automatically set to this. Again
     * by using this we understand we should treat it as a time duration not a time point.
     */
//    if(ptm->tm_year + 1730 == 1800 || ptm->tm_year + 1730 == 1400){
//        oss << ptm->tm_hour << ":" << ptm->tm_min << ":" << ptm->tm_sec ;
//    }else{
//        oss << ptm->tm_mon+1 << "/" << ptm->tm_mday << "/" <<
//                ptm->tm_year + 1730 << " " << ptm->tm_hour << ":" << ptm->tm_min << ":" << ptm->tm_sec ;
//    }
    boost::posix_time::ptime pTime = boost::posix_time::from_time_t(*secondsFromEpoch);
    oss << pTime;
    return oss.str() ;
}

bool DateAndTimeHandler::verifyDateTimeString(const string & timeString, DateTimeType dateTimeType){
    switch (dateTimeType) {
        case DateTimeTypeNow:
            if(timeString.compare("NOW")){
                return true;
            }else{
                return false;
            }
        case DateTimeTypePointOfTime:{
            for(size_t i=0; i<localeFormatsPointOfTime; ++i)
            {
                boost::regex e(regexInputsPointOfTime[i]);
                boost::smatch what;
                if(boost::regex_match(timeString, what, e, boost::match_extra))
                {
                    return true;
                }
            }
            return false;
        }
        case DateTimeTypeDurationOfTime:{
        	// 1. first see if it's a known duration format
            for(size_t i=0; i<localeFormatsDurationOfTime; ++i)
            {
                boost::regex e(regexInputsDurationOfTime[i]);
                boost::smatch what;
                if(boost::regex_match(timeString, what, e, boost::match_extra))
                {
                    return true;
                }
            }
            // 2. second check to see if it is a constant
            if(std::find(DURATION_OF_TIME_CONSTANTS.begin() , DURATION_OF_TIME_CONSTANTS.end() , timeString) != DURATION_OF_TIME_CONSTANTS.end()){
            	return true;
            }
            // 3. third, check to see if it's a combination of number and constant
            // first make the regex string
            string format = "^(";
            for(vector<string>::const_iterator cons = DURATION_OF_TIME_CONSTANTS.begin() ; cons != DURATION_OF_TIME_CONSTANTS.end() ; ++cons){
                format+= "\\d*"+*cons + "|";
            }
            format += ")$";
            boost::regex e2(format);
            boost::smatch what2;
            if(boost::regex_match(timeString, what2, e2, boost::match_extra)){ // string matches this pattern
                return true;
            }
            return false;
        }
        default:
            return false;
    }
}
time_t DateAndTimeHandler::convertPtimeToSecondsFromEpoch(boost::posix_time::ptime t){
    static boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
    return (t-epoch).ticks() / boost::posix_time::time_duration::ticks_per_second();
}

boost::posix_time::ptime DateAndTimeHandler::convertSecondsFromEpochToPTime(time_t t){
    return boost::posix_time::from_time_t(t);
}

/*
 * In this function the string should be parsed and TimeDuration object must be built
 */
TimeDuration DateAndTimeHandler::convertDurationTimeStringToTimeDurationObject(const string & timeString){
    // 1. if it is a constant, parse and save as a TimeDuration object
	vector<string>::const_iterator constantIter = std::find(DURATION_OF_TIME_CONSTANTS.begin() , DURATION_OF_TIME_CONSTANTS.end() , timeString);
    if(constantIter != DURATION_OF_TIME_CONSTANTS.end()){
    	string constant = *constantIter;
    	if(constant.compare("SECOND") == 0 || constant.compare("SECONDS") == 0  ){
    		TimeDuration td;
    		td.smallerDurationsList.push_back(boost::posix_time::seconds(1));
    		return td;
    	}
    	if(constant.compare("MINUTE") == 0 || constant.compare("MINUTES") == 0  ){
    		TimeDuration td;
    		td.smallerDurationsList.push_back(boost::posix_time::minutes(1));
    		return td;
    	}
    	if(constant.compare("HOUR") == 0 || constant.compare("HOURS") == 0  ){
    		TimeDuration td;
    		td.smallerDurationsList.push_back(boost::posix_time::hours(1));
    		return td;
    	}
    	if(constant.compare("DAY") == 0 || constant.compare("DAYS") == 0  ){
    		TimeDuration td;
    		td.daysList.push_back(boost::gregorian::days(1));
    		return td;
    	}
    	if(constant.compare("WEEK") == 0 || constant.compare("WEEKS") == 0  ){
    		TimeDuration td;
    		td.weeksList.push_back(boost::gregorian::weeks(1));
    		return td;
    	}
    	if(constant.compare("MONTH") == 0 || constant.compare("MONTHS") == 0  ){
    		TimeDuration td;
    		td.monthsList.push_back(boost::gregorian::months(1));
    		return td;
    	}
    	if(constant.compare("YEAR") == 0 || constant.compare("YEARS") == 0  ){
    		TimeDuration td;
    		td.yearsList.push_back(boost::gregorian::years(1));
    		return td;
    	}

    }
    // 2. if it's a combination of number and constant
    // first make the regex string
    string format = "^(";
    for(vector<string>::const_iterator cons = DURATION_OF_TIME_CONSTANTS.begin() ; cons != DURATION_OF_TIME_CONSTANTS.end() ; ++cons){
        format+= "\\d*"+*cons + "|";
    }
    format += ")$";
    boost::regex e2(format);
    boost::smatch what2;
    if(boost::regex_match(timeString, what2, e2, boost::match_extra)){ // string matches this pattern
        for(vector<string>::const_iterator cons = DURATION_OF_TIME_CONSTANTS.begin() ; cons != DURATION_OF_TIME_CONSTANTS.end() ; ++cons){
        	string constant = *cons;
            if(timeString.find(constant) != std::string::npos){
            	// TODO: cahnge atoi
                int numberOfThisUnit =  atoi(timeString.substr(0,timeString.size() - constant.size()).c_str());
            	if(constant.compare("SECOND") == 0 || constant.compare("SECONDS") == 0  ){
            		TimeDuration td;
            		td.smallerDurationsList.push_back(boost::posix_time::seconds(numberOfThisUnit));
            		return td;
            	}
            	if(constant.compare("MINUTE") == 0 || constant.compare("MINUTES") == 0  ){
            		TimeDuration td;
            		td.smallerDurationsList.push_back(boost::posix_time::minutes(numberOfThisUnit));
            		return td;
            	}
            	if(constant.compare("HOUR") == 0 || constant.compare("HOURS") == 0  ){
            		TimeDuration td;
            		td.smallerDurationsList.push_back(boost::posix_time::hours(numberOfThisUnit));
            		return td;
            	}
            	if(constant.compare("DAY") == 0 || constant.compare("DAYS") == 0  ){
            		TimeDuration td;
            		td.daysList.push_back(boost::gregorian::days(numberOfThisUnit));
            		return td;
            	}
            	if(constant.compare("WEEK") == 0 || constant.compare("WEEKS") == 0  ){
            		TimeDuration td;
            		td.weeksList.push_back(boost::gregorian::weeks(numberOfThisUnit));
            		return td;
            	}
            	if(constant.compare("MONTH") == 0 || constant.compare("MONTHS") == 0  ){
            		TimeDuration td;
            		td.monthsList.push_back(boost::gregorian::months(numberOfThisUnit));
            		return td;
            	}
            	if(constant.compare("YEAR") == 0 || constant.compare("YEARS") == 0  ){
            		TimeDuration td;
            		td.yearsList.push_back(boost::gregorian::years(numberOfThisUnit));
            		return td;
            	}
            }
        }
    }
	// 3. if it's a known duration format
    for(size_t i=0; i<localeFormatsDurationOfTime; ++i)
    {
        boost::regex e(regexInputsDurationOfTime[i]);
        boost::smatch what;
        if(boost::regex_match(timeString, what, e, boost::match_extra))
        {
            TimeDuration td;
            td.smallerDurationsList.push_back(boost::posix_time::duration_from_string(timeString));
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
    // constants["MONTH"] = ???? how should we support month? Months have different lengths ...
    return constants;
}
const string DateAndTimeHandler::regexInputsDurationOfTime[] = {
        "^\\d{2}:\\d{2}:\\d{2}$"
};
const locale DateAndTimeHandler::localeInputsDurationOfTime[] ={
        locale(locale::classic(), new time_input_facet("%H:%M:%S"))
};
const size_t DateAndTimeHandler::localeFormatsDurationOfTime =
        sizeof(DateAndTimeHandler::localeInputsDurationOfTime)/sizeof(DateAndTimeHandler::localeInputsDurationOfTime[0]);

const string DateAndTimeHandler::regexInputsPointOfTime[] = {
		"^\\d{2}:\\d{2}:\\d{2}$",
        "^\\d{2}/\\d{2}/\\d{4}$",
        "^\\d{4}-\\d{2}-\\d{2}\\s\\d{2}:\\d{2}:\\d{2}$",
        "^\\d{4}\\d{2}\\d{2}\\s\\d{2}\\d{2}\\d{2}$",
        "^\\d{4}-\\d{2}-\\d{2}\\s\\d{2}:\\d{2}$",
        "^\\d{4}-\\d{2}-\\d{2}$"
};

const locale DateAndTimeHandler::localeInputsPointOfTime[] ={
		locale(locale::classic(), new time_input_facet("%H:%M:%S")),
        locale(locale::classic(), new time_input_facet("%m/%d/%Y")),
        locale(locale::classic(), new time_input_facet("%Y-%m-%d %H:%M:%S")),
        locale(locale::classic(), new time_input_facet("%Y%m%d%H%M%S")),
        locale(locale::classic(), new time_input_facet("%Y%m%d%H%M")),
        locale(locale::classic(), new time_input_facet("%Y%m%d"))
};
const size_t DateAndTimeHandler::localeFormatsPointOfTime =
        sizeof(DateAndTimeHandler::localeInputsPointOfTime)/sizeof(DateAndTimeHandler::localeInputsPointOfTime[0]);

const vector<string> DateAndTimeHandler::DURATION_OF_TIME_CONSTANTS = DateAndTimeHandler::initializeConstants();


boost::posix_time::ptime TimeDuration::operator+(const boost::posix_time::ptime & pTime) const{
	boost::posix_time::ptime timeResult;
	timeResult = pTime;

	for(vector<boost::gregorian::days>::const_iterator day = daysList.begin();
			day != daysList.end(); ++day){
		timeResult = timeResult + *day;
	}

	for(vector<boost::gregorian::weeks>::const_iterator week = weeksList.begin();
			week != weeksList.end(); ++week){
		timeResult = timeResult + *week;
	}

	for(vector<boost::gregorian::months>::const_iterator month = monthsList.begin();
			month != monthsList.end(); ++month){
		timeResult = timeResult + *month;
	}

	for(vector<boost::gregorian::years>::const_iterator year = yearsList.begin();
			year != yearsList.end(); ++year){
		timeResult = timeResult + *year;
	}

	for(vector<boost::posix_time::time_duration>::const_iterator hour_minute_second = smallerDurationsList.begin();
			hour_minute_second != smallerDurationsList.end(); ++hour_minute_second){
		timeResult = timeResult + *hour_minute_second;
	}

	return timeResult;
}

long TimeDuration::operator+(const long pTime) const{
	boost::posix_time::ptime pTimeObj = DateAndTimeHandler::convertSecondsFromEpochToPTime(pTime);
	return DateAndTimeHandler::convertPtimeToSecondsFromEpoch(*this + pTimeObj); // it uses the other implementation for operator +
}


TimeDuration TimeDuration::operator+(const TimeDuration & timeDuration) const{
	TimeDuration result;

	result.smallerDurationsList.insert(result.smallerDurationsList.begin() , timeDuration.smallerDurationsList.begin() , timeDuration.smallerDurationsList.end());
	result.smallerDurationsList.insert(result.smallerDurationsList.begin() , this->smallerDurationsList.begin() , this->smallerDurationsList.end());

	result.daysList.insert(result.daysList.begin() , timeDuration.daysList.begin() , timeDuration.daysList.end());
	result.daysList.insert(result.daysList.begin() , this->daysList.begin() , this->daysList.end());

	result.monthsList.insert(result.monthsList.begin() , timeDuration.monthsList.begin() , timeDuration.monthsList.end());
	result.monthsList.insert(result.monthsList.begin() , this->monthsList.begin() , this->monthsList.end());

	result.weeksList.insert(result.weeksList.begin() , timeDuration.weeksList.begin() , timeDuration.weeksList.end());
	result.weeksList.insert(result.weeksList.begin() , this->weeksList.begin() , this->weeksList.end());

	result.yearsList.insert(result.yearsList.begin() , timeDuration.yearsList.begin() , timeDuration.yearsList.end());
	result.yearsList.insert(result.yearsList.begin() , this->yearsList.begin() , this->yearsList.end());

	return result;
}

string TimeDuration::toString() const{
	return "GAP"; // TODO
}


}
}
