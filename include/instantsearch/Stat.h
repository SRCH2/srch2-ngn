/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
