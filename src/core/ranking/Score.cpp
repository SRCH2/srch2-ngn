
// $Id: Score.cpp 2013-06-19 02:11:13Z Jamshid $

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



#include <instantsearch/Score.h>
#include <sstream>
#include <cstdlib>
#include "util/Assert.h"
#include <limits>
#include <cmath>


namespace srch2
{
    namespace instantsearch
    {



	bool Score::operator==(const Score& score) const{
		ASSERT(valueType == score.valueType);
    	switch (valueType) {
			case ATTRIBUTE_TYPE_UNSIGNED:
				return intScore == score.intScore;
			case ATTRIBUTE_TYPE_FLOAT:
				return floatScore == score.floatScore;
			case ATTRIBUTE_TYPE_TEXT:
				return stringScore == score.stringScore;
			case ATTRIBUTE_TYPE_TIME:
				return timeScore == score.timeScore;
		}
    	return false;
	}
	bool Score::operator!=(const Score& score) const{
		ASSERT(valueType == score.valueType);
		return !(*this == score);
	}

	bool Score::operator<(const Score& score) const{
		ASSERT(valueType == score.valueType);
    	switch (valueType) {
			case ATTRIBUTE_TYPE_UNSIGNED:
				return intScore < score.intScore;
			case ATTRIBUTE_TYPE_FLOAT:
				return floatScore < score.floatScore;
			case ATTRIBUTE_TYPE_TEXT:
				return stringScore < score.stringScore;
			case ATTRIBUTE_TYPE_TIME:
				return timeScore < score.timeScore;
		}
    	return false;
	}

	bool Score::operator<=(const Score& score) const {
		ASSERT(valueType == score.valueType);
		return (*this < score) || (*this == score);
	}

	bool Score::operator>(const Score& score) const{
		ASSERT(valueType == score.valueType);
		return !(*this <= score);
	}

	bool Score::operator>=(const Score& score) const{
		ASSERT(valueType == score.valueType);
		return !(*this < score);
	}
	Score Score::operator+(const Score& a){
		ASSERT(a.valueType == this->valueType);
    	Score result;
    	switch (this->valueType) {
			case ATTRIBUTE_TYPE_UNSIGNED:
				result.setScore(a.getIntScore() + this->getIntScore());
				break;
			case ATTRIBUTE_TYPE_FLOAT:
				result.setScore(a.getFloatScore() + this->getFloatScore());
				break;
			case ATTRIBUTE_TYPE_TEXT:
				result.setScore(a.getTextScore() + this->getTextScore());
				break;
			case ATTRIBUTE_TYPE_TIME:
				result.setScore(a.getTimeScore() + this->getTimeScore());
				break;
		}

    	return result;
	}




	void Score::setScore(unsigned intScore){
		valueType = ATTRIBUTE_TYPE_UNSIGNED;
		this->intScore = intScore;
	}
	void Score::setScore(float floatScore){
		valueType = ATTRIBUTE_TYPE_FLOAT;
		this->floatScore = floatScore;
	}
	void Score::setScore(double doubleScore){
		setScore((float)doubleScore);
	}
	void Score::setScore(string stringScore){
		valueType = ATTRIBUTE_TYPE_TEXT;
		this->stringScore = stringScore;
	}
	void Score::setScore(long timeScore){
		valueType = ATTRIBUTE_TYPE_TIME;
		this->timeScore = timeScore;
	}

	void Score::setScore(const Score& score){
    	valueType = score.valueType;
    	switch (valueType) {
			case ATTRIBUTE_TYPE_UNSIGNED:
				intScore = score.intScore;
				break;
			case ATTRIBUTE_TYPE_FLOAT:
				floatScore = score.floatScore;
				break;
			case ATTRIBUTE_TYPE_TEXT:
				stringScore = score.stringScore;
				break;
			case ATTRIBUTE_TYPE_TIME:
				timeScore = score.timeScore;
				break;
		}
	}

	void Score::setScore(FilterType type , string value){
	    // TODO : do some validation to make sure engine does not crash.
	    //        NOTE: The input to this function is supposed to be validated once...
		this->valueType = type;
		switch (type) {
			case ATTRIBUTE_TYPE_UNSIGNED:
				this->setScore((unsigned)atoi(value.c_str()));
				break;
			case ATTRIBUTE_TYPE_FLOAT:
				this->setScore(atof(value.c_str()));
				break;
			case ATTRIBUTE_TYPE_TEXT:
				this->setScore(value);
				break;
			case ATTRIBUTE_TYPE_TIME:

				this->setScore(atol(value.c_str()));
				break;
		}
	}

	unsigned Score::getIntScore() const{
		return intScore;
	}
	float Score::getFloatScore() const{
		return floatScore;
	}
	double Score::getDoubleScore() const{
		return (double)getFloatScore();
	}
	string Score::getTextScore() const{
		return stringScore;
	}
	long Score::getTimeScore() const{
		return timeScore;
	}
	Score Score::minimumValue(){
		Score result ;
		switch (valueType) {
				case ATTRIBUTE_TYPE_UNSIGNED:
					result.setScore((unsigned)std::numeric_limits<unsigned>::min());
					break;
				case ATTRIBUTE_TYPE_FLOAT:
					result.setScore((float)std::numeric_limits<float>::min());
					break;
				case ATTRIBUTE_TYPE_TEXT:
					result.setScore("NO_MINIMUM_FOR_TEXT");
					break;
				case ATTRIBUTE_TYPE_TIME:
					result.setScore((long)std::numeric_limits<long>::min());
					break;
			}

		return result;
	}

	float Score::castToFloat(){
		float result ;
		switch (valueType) {
				case ATTRIBUTE_TYPE_UNSIGNED:
					result = (float) getIntScore();
					break;
				case ATTRIBUTE_TYPE_FLOAT:
					result = getFloatScore();
					break;
				case ATTRIBUTE_TYPE_TEXT:
					result = -1;
					ASSERT(false);
					break;
				case ATTRIBUTE_TYPE_TIME:
					result = (float) getTimeScore();
					break;
			}

		return result;
	}

	// Example
	// if start = 10, end = 100, gap = 10
	// this->value = 1 => returns 0
	// this->value = 10 => returns 1
	// this->value = 21 => returns 2
    unsigned Score::findIndexOfContainingInterval(Score & start , Score & end, Score & gap) const{
        float thisScore = 0;
        float startScore = 0;
        float endScore = 0;
        float gapScore = 0;
        switch (this->getType()) {
            case ATTRIBUTE_TYPE_UNSIGNED:
                // first bucket which covers less-than-start values is zero
                if(this->intScore < start.getIntScore()){
                    return 0;
                }
                thisScore = this->getIntScore();
                startScore = start.getIntScore();
                endScore = end.getIntScore();
                gapScore = gap.getIntScore();
                break;
            case ATTRIBUTE_TYPE_FLOAT:
                // first bucket which covers less-than-start values is zero
                if(this->floatScore < start.getFloatScore()){
                    return 0;
                }
                thisScore = this->getFloatScore();
                startScore = start.getFloatScore();
                endScore = end.getFloatScore();
                gapScore = gap.getFloatScore();
                break;
            default:
                ASSERT(false);
                return -1; // invalid group id
        }
        // generating group id for intervals, the buckets given to values greater than start are grater than or equal to 1
        return floor((thisScore - startScore) / gapScore) + 1 ;
    }

    Score::Score(const Score& score){
    	setScore(score);
    }

	string Score::toString()  const{
		std::stringstream ss;

    	switch (valueType) {
			case ATTRIBUTE_TYPE_UNSIGNED:
				ss << intScore ;
				break;
			case ATTRIBUTE_TYPE_FLOAT:
				ss << floatScore;
				break;
			case ATTRIBUTE_TYPE_TEXT:
				ss << stringScore ;
				break;
			case ATTRIBUTE_TYPE_TIME:

				ss << timeScore ;
				break;
			default:
				ss << "";
				break;
		}
    	return ss.str();
	}


    }


}
