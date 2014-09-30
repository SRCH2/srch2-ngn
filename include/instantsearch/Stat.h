// $Id: Stat.h 3456 2013-06-14 02:11:13Z jiaying $

/*
 * The Software is made available solely for use according to the License Agreement. Any reproduction
 * or redistribution of the Software not in accordance with the License Agreement is expressly prohibited
 * by law, and may result in severe civil and criminal penalties. Violators will be prosecuted to the
 * maximum extent possible.
 *
 * THE SOFTWARE IS WARRANTED, IF AT ALL, ONLY ACCORDING TO THE TERMS OF THE LICENSE AGREEMENT. EXCEPT
 * AS WARRANTED IN THE LICENSE AGREEMENT, SRCH2 INC. HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS WITH
 * REGARD TO THE SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SRCH2 INC. BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 * OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF SOFTWARE.

 * Copyright © 2010 SRCH2 Inc. All rights reserved
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
    Stat(const Stat & copy){
    	this->tstart = copy.tstart;
    	this->accumulatedTime = copy.accumulatedTime;
    	this->counter = copy.counter;
    	this->statList = copy.statList;
    }
    Stat & operator=(const Stat & rhs){
    	if(this != &rhs){
        	this->tstart = rhs.tstart;
        	this->accumulatedTime = rhs.accumulatedTime;
        	this->counter = rhs.counter;
        	this->statList = rhs.statList;
    	}
    	return *this;
    }

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
