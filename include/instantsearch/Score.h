
// $Id: Score.h 2013-06-19 02:11:13Z Jamshid $

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




#ifndef _INSTANTSEARCH_SCORE_H__
#define _INSTANTSEARCH_SCORE_H__

#include <cstring>
#include <ctime>
#include <instantsearch/Schema.h>


using namespace std;

namespace srch2
{
    namespace instantsearch
    {

    class Score
    {
    public:
    	Score(const Score& score);
    	bool operator==(const Score& score) const;
    	bool operator!=(const Score& score) const;
    	bool operator<(const Score& score) const;
    	bool operator>(const Score& score) const;
    	bool operator<=(const Score& score) const;
    	bool operator>=(const Score& score) const;
    	Score operator+(const Score& a);

    	Score(){
    	};
    	void setScore(unsigned intS);
    	void setScore(float floatS);
    	void setScore(double doubleS);
    	void setScore(string stringS);
    	void setScore(long timeS);
    	void setScore(const Score& score);
    	void setScore(FilterType type , string value);

    	FilterType getType(){
    		return type;
    	}

    	unsigned getIntScore() const;
    	float getFloatScore() const;
    	double getDoubleScore() const;
    	string getTextScore() const;
    	long getTimeScore() const;


    	Score minimumValue();

    	float castToFloat();




    	string toString() const;


    private:
    	FilterType type;
    	unsigned intScore;
    	float floatScore;
    	string stringScore;
    	long timeScore;
    };


    }

}

#endif
