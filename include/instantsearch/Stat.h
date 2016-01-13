
#ifndef __STAT_H__
#define __STAT_H__

#include <vector>
#include <iostream>
#include "util/mytime.h"

namespace srch2
{
namespace instantsearch
{

class Stat
{
public:
    Stat() {
        counter = 0;
        accumulatedTime = 0.0;
        //clock_gettime(CLOCK_REALTIME, &tstart);
    };
    virtual ~Stat() { statList.clear(); };

    void startMessage()
    {
        clock_gettime(CLOCK_REALTIME, &tstart);
    }

    void endMessage()
    {
        timespec tend;
        clock_gettime(CLOCK_REALTIME, &tend);
        double time = (double)((tend.tv_sec - tstart.tv_sec) * 1000) + (double)((tend.tv_nsec - tstart.tv_nsec) / 1000000);
        std::cout << "time: " << time << std::endl;
        accumulatedTime += time;
        std::cout << "accumulatedTime: " << accumulatedTime << std::endl;
        counter++;
    }

    void printForXiang() const
    {
        std::cout << "counter: " << counter << std::endl;
        std::cout << "accumulatedTime: " << accumulatedTime << std::endl;
    }
    void addMessage(std::string &message)
    {
        return;
        struct timespec tend;
        clock_gettime(CLOCK_REALTIME, &tend);
        unsigned time = (tend.tv_sec - tstart.tv_sec) * 1000 + (tend.tv_nsec - tstart.tv_nsec) / 1000000;

        this->statList.push_back( std::make_pair<std::string,unsigned>(message, time));
    }

    void addMessage(const char * msg)
    {
        std::string str = std::string(msg);
        this->addMessage(str);
    }

    void print() const
    {
        return;
        for (std::vector< std::pair<std::string,unsigned> >::const_iterator iter = this->statList.begin();
                iter != this->statList.end();
                ++iter)
        {
            std::cout << "[" << iter->first << "][" << iter->second << "]" << std::endl;
        }
    }

private:
    timespec tstart;
    unsigned counter;
    double accumulatedTime;
    std::vector< std::pair<std::string,unsigned> > statList;

};

}}

#endif /* __STAT_H__ */
