


#include <iostream>
#include "DateAndTimeHandler.h"
#include <boost/regex.hpp>
#include <cstdlib>


namespace srch2 {
namespace httpwrapper {

time_t DateAndTimeHandler::convertDateTimeStringToSecondsFromEpoch(const string & timeString)
{
    // 1. first check with the known point of time formats we have
    bt::ptime pt;
    for(size_t i=0; i<localeFormatsPointOfTime; ++i)
    {
        std::istringstream is(timeString);
        is.imbue(localeInputsPointOfTime[i]);
        is >> pt;
        if(pt != bt::not_a_date_time){
            return convertPtimeToTimeT(pt);
        }
    }
    // and check it with duration of time formats that we have
    for(size_t i=0; i<localeFormatsDurationOfTime; ++i)
    {
        std::istringstream is(timeString);
        is.imbue(localeInputsDurationOfTime[i]);
        is >> pt;
        if(pt != bt::not_a_date_time){
            return convertPtimeToTimeT(pt);
        }
    }
    // 2. second check to see if it is a constant
    if(DURATION_OF_TIME_CONSTANTS.find(timeString) != DURATION_OF_TIME_CONSTANTS.end()){
        return DURATION_OF_TIME_CONSTANTS.at(timeString);
    }
    // 3. third, check to see if it's a combination of number and constant
    // first make the regex string
    string format = "^(";
    for(map<string,time_t>::const_iterator cons = DURATION_OF_TIME_CONSTANTS.begin() ; cons != DURATION_OF_TIME_CONSTANTS.end() ; ++cons){
        format+= "\\d*"+cons->first + "|";
    }
    format += ")$";
    boost::regex e(format);
    boost::smatch what;
    if(boost::regex_match(timeString, what, e, boost::match_extra)){ // string matches this pattern
        /*
         * Example: we have some string like 3DAYS and we want to convert it to 3*number of seconds in a day
         */
        for(map<string,time_t>::const_iterator cons = DURATION_OF_TIME_CONSTANTS.begin() ; cons != DURATION_OF_TIME_CONSTANTS.end() ; ++cons){
            if(timeString.find(cons->first) != std::string::npos){
                cout << "Matching " << cons->first << endl;
                return atoi(timeString.substr(0,timeString.size() - cons->first.size()).c_str()) * cons->second;
            }
        }
    }
    // 4. check to see if it is a time relative to NOW (right now we only support now)
    if(timeString.compare("NOW") == 0){ // it's NOW
        return convertPtimeToTimeT(boost::posix_time::second_clock::local_time());
    }
    return -1;
}

string DateAndTimeHandler::convertSecondsFromEpochToDateTimeString(register const time_t * secondsFromEpoch){

    struct tm * ptm = gmtime(secondsFromEpoch);
    ostringstream oss;
    /*
     * 1800 is the beginning of time, for those time constants like DAY, since we don't enter any year,
     * their year is set to 1800. This is how we determine this time should be printed as a time gap or a time point
     *
     * 1400 is the lower bound of boost time library. For HH:mm:ss format the year is automatically set to this. Again
     * by using this we understand we should treat it as a time duration not a time point.
     */
    if(ptm->tm_year + 1730 == 1800 || ptm->tm_year + 1730 == 1400){
        oss << ptm->tm_hour << ":" << ptm->tm_min << ":" << ptm->tm_sec ;
    }else{
        oss << ptm->tm_mon+1 << "/" << ptm->tm_mday << "/" <<
                ptm->tm_year + 1730 << " " << ptm->tm_hour << ":" << ptm->tm_min << ":" << ptm->tm_sec ;
    }
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
            bt::ptime pt;
            for(size_t i=0; i<localeFormatsPointOfTime; ++i)
            {
                std::istringstream is(timeString);
                is.imbue(localeInputsPointOfTime[i]);
                is >> pt;
                if(pt != bt::not_a_date_time){
                    return true;
                }
            }
            return false;
        }
        case DateTimeTypeDurationOfTime:{
            bt::ptime pt;
            // and check it with duration of time formats that we have
            for(size_t i=0; i<localeFormatsDurationOfTime; ++i)
            {
                std::istringstream is(timeString);
                is.imbue(localeInputsDurationOfTime[i]);
                is >> pt;
                if(pt != bt::not_a_date_time){
                    return true;
                }
            }
            return false;
        }
        default:
            return false;
    }
}
time_t DateAndTimeHandler::convertPtimeToTimeT(boost::posix_time::ptime t)
{
    static boost::posix_time::ptime epoch(boost::gregorian::date(1800,1,1));
    return (t-epoch).ticks() / boost::posix_time::time_duration::ticks_per_second();
}

map<string, time_t> DateAndTimeHandler::initializeConstants(){
    map<string, time_t> constants;
    constants["SECOND"] = 1;
    constants["MINUTE"] = 60;
    constants["HOUR"] = 3600;
    constants["DAY"] = 86400;
    constants["WEEK"] = 604800;
    constants["SECONDS"] = 1;
    constants["MINUTES"] = 60;
    constants["HOURS"] = 3600;
    constants["DAYS"] = 86400;
    constants["WEEKS"] = 604800;
    // constants["MONTH"] = ???? how should we support month? Months have different lengths ...
    return constants;
}

const locale DateAndTimeHandler::localeInputsPointOfTime[] ={
        locale(locale::classic(), new time_input_facet("%H:%M:%S"))
};
const size_t DateAndTimeHandler::localeFormatsPointOfTime =
        sizeof(DateAndTimeHandler::localeInputsPointOfTime)/sizeof(DateAndTimeHandler::localeInputsPointOfTime[0]);

const locale DateAndTimeHandler::localeInputsDurationOfTime[] ={
        locale(locale::classic(), new time_input_facet("%m/%d/%Y")),
        locale(locale::classic(), new time_input_facet("%Y-%m-%d %H:%M:%S")),
        locale(locale::classic(), new time_input_facet("%Y%m%d%H%M%S")),
        locale(locale::classic(), new time_input_facet("%Y%m%d%H%M")),
        locale(locale::classic(), new time_input_facet("%Y%m%d"))
};
const size_t DateAndTimeHandler::localeFormatsDurationOfTime =
        sizeof(DateAndTimeHandler::localeInputsDurationOfTime)/sizeof(DateAndTimeHandler::localeInputsDurationOfTime[0]);

const map<string, time_t> DateAndTimeHandler::DURATION_OF_TIME_CONSTANTS = DateAndTimeHandler::initializeConstants();


}
}
