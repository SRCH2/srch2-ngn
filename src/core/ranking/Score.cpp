
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
#include <util/Assert.h>
#include <limits>


namespace srch2
{
    namespace instantsearch
    {



	bool Score::operator==(const Score& score) const{
		ASSERT(type == score.type);
    	switch (type) {
			case UNSIGNED:
				return intScore == score.intScore;
				break;
			case FLOAT:
				return floatScore == score.floatScore;
				break;
			case TEXT:
				return stringScore == score.stringScore;
				break;
			case TIME:
				return timeScore == score.timeScore;
				break;
			default:
				break;
		}
    	return false;
	}
	bool Score::operator!=(const Score& score) const{
		ASSERT(type == score.type);
		return !(*this == score);
	}

	bool Score::operator<(const Score& score) const{
		ASSERT(type == score.type);
    	switch (type) {
			case UNSIGNED:
				return intScore < score.intScore;
				break;
			case FLOAT:
				return floatScore < score.floatScore;
				break;
			case TEXT:
				return stringScore < score.stringScore;
				break;
			case TIME:
				return timeScore < score.timeScore;
				break;
			default:
				break;
		}
    	return false;
	}

	bool Score::operator<=(const Score& score) const {
		ASSERT(type == score.type);
		return (*this < score) || (*this == score);
	}

	bool Score::operator>(const Score& score) const{
		ASSERT(type == score.type);
		return !(*this <= score);
	}

	bool Score::operator>=(const Score& score) const{
		ASSERT(type == score.type);
		return !(*this < score);
	}
	Score Score::operator+(const Score& a){
		ASSERT(a.type == this->type);
    	Score result;
    	switch (this->type) {
			case UNSIGNED:
				result.setScore(a.getIntScore() + this->getIntScore());
				break;
			case FLOAT:
				result.setScore(a.getFloatScore() + this->getFloatScore());
				break;
			case TEXT:
				result.setScore(a.getTextScore() + this->getTextScore());
				break;
			case TIME:
				result.setScore(a.getTimeScore() + this->getTimeScore());
				break;
		}

    	return result;
	}




	void Score::setScore(unsigned intS){
		type = UNSIGNED;
		intScore = intS;
	}
	void Score::setScore(float floatS){
		type = FLOAT;
		floatScore = floatS;
	}
	void Score::setScore(double doubleS){
		setScore((float)doubleS);
	}
	void Score::setScore(string stringS){
		type = TEXT;
		stringScore = stringS;
	}
	void Score::setScore(long timeS){
		type = TIME;
		timeScore = timeS;
	}

	void Score::setScore(const Score& score){
    	type = score.type;
    	switch (type) {
			case UNSIGNED:
				intScore = score.intScore;
				break;
			case FLOAT:
				floatScore = score.floatScore;
				break;

			case TEXT:
				stringScore = score.stringScore;
				break;

			case TIME:
				timeScore = score.timeScore;
				break;
			default:
				break;
		}
	}

	void Score::setScore(FilterType type , string value){
		this->type = type;
		switch (type) {
			case UNSIGNED:
				this->setScore((unsigned)atoi(value.c_str()));
				break;
			case FLOAT:
				this->setScore(atof(value.c_str()));
				break;
			case TEXT:
				this->setScore(value);
				break;
			case TIME:
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
		switch (type) {
				case UNSIGNED:
					result.setScore((unsigned)std::numeric_limits<unsigned>::min());
					break;
				case FLOAT:
					result.setScore((float)std::numeric_limits<float>::min());
					break;
				case TEXT:
					result.setScore("NO_MINIMUM_FOR_TEXT");
					break;
				case TIME:
					result.setScore((long)std::numeric_limits<long>::min());
					break;
			}

		return result;
	}

	float Score::castToFloat(){
		float result ;
		switch (type) {
				case UNSIGNED:
					result = (float) getIntScore();
					break;
				case FLOAT:
					result = getFloatScore();
					break;
				case TEXT:
					result = -1;
					ASSERT(false);
					break;
				case TIME:
					result = (float) getTimeScore();
					break;
			}

		return result;
	}

    Score::Score(const Score& score){
    	setScore(score);
    }

	string Score::toString()  const{
		std::stringstream ss;

    	switch (type) {
			case UNSIGNED:
				ss << intScore ;
				break;
			case FLOAT:
				ss << floatScore;
				break;
			case TEXT:
				ss << stringScore ;
				break;
			case TIME:
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
