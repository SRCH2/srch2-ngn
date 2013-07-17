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

 * Copyright © 2013 SRCH2 Inc. All rights reserved
 */

#include "util/VariableLengthAttributeContainer.h"
#include "instantsearch/Score.h"

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include "util/Assert.h"


using namespace std;
using namespace srch2::instantsearch;

void printData(unsigned char * data , unsigned size){
	std::cout << "Data :";
	for(int i=0;i<size; ++i){
		if(i % 4 == 0) std::cout << "|";
		else std::cout << ",";
		std::cout << (unsigned)data[i] ;
	}
	std::cout << std::endl;
}
void printData(std::vector<unsigned char> data){
	std::cout << "Data :";
	for(int i=0;i<data.size(); ++i){
		if(i % 4 == 0) std::cout << "|";
		else std::cout << ",";
		std::cout << (unsigned)data[i];
	}
	std::cout << std::endl;
}

// test convertors
void test_1(){
	VariableLengthAttributeContainer vlac;

	for(int i=0;i<100;i++){
		unsigned number = i * 1000;
		std::vector<unsigned char> numberCharVector = vlac.convertUnsignedToCharVector(number);
		ASSERT(vlac.convertCharVectorToUnsigned(numberCharVector , numberCharVector.begin(), numberCharVector.end())
				 == number);
	}

	for(int i=0;i<100;i++){
		float number = i + (i*1.0) / 1000;
		std::vector<unsigned char> numberCharVector = vlac.convertFloatToCharVector(number);
		ASSERT(vlac.convertCharVectorToFloat(numberCharVector , numberCharVector.begin(), numberCharVector.end())
				 == number);
	}

	for(int i=0;i<100;i++){
		long number = i + 1000000000;
		std::vector<unsigned char> numberCharVector = vlac.convertLongToCharVector(number);
		ASSERT(vlac.convertCharVectorToLong(numberCharVector , numberCharVector.begin(), numberCharVector.end())
				 == number);
	}

	for(int i=0;i<100;i++){
		std::stringstream ss;
		for(int j=0;j< 2*i+5 ; j++){
			ss << "a" ;
		}
		std::string value = ss.str();
		std::vector<unsigned char> charVector = vlac.convertStringToCharVector(TEXT, value);
		ASSERT(value == vlac.convertCharVectorToString(TEXT, charVector));
	}

}

// test getter and setters
// schema : TEXT,	UNSIGNED,	TIME,	STRING,	FLOAT,	UNSIGNED,	TEXT,	TEXT,	UNSIGNED,	UNSIGNED
// semantic:NAME,	AGE,		bdate,	title,	weight,	salary,		fname,	lname,	ID,			EID
void test_2(){

    ///Create Schema
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);

    schema->setNonSearchableAttribute("text1" , srch2::instantsearch::TEXT, "text1", true);
    schema->setNonSearchableAttribute("int1", srch2::instantsearch::UNSIGNED, "1" , true);
    schema->setNonSearchableAttribute("time1", srch2::instantsearch::TIME, "0" , false);
    schema->setNonSearchableAttribute("text2", srch2::instantsearch::TEXT, "text2" , true);
    schema->setNonSearchableAttribute("float1", srch2::instantsearch::FLOAT, "0.1" , false);
    schema->setNonSearchableAttribute("int2", srch2::instantsearch::UNSIGNED, "2" , true);
    schema->setNonSearchableAttribute("text3" , srch2::instantsearch::TEXT, "text3", true);
    schema->setNonSearchableAttribute("text4" , srch2::instantsearch::TEXT, "text4", false);
    schema->setNonSearchableAttribute("int3", srch2::instantsearch::UNSIGNED, "3" , true);
    schema->setNonSearchableAttribute("int4", srch2::instantsearch::UNSIGNED, "4" , false);



	VariableLengthAttributeContainer vlac;

	std::string record1[10] =
	{"John Smith" , "23" , "2000344567" , "Doctor" , "70.4567" , "12000" , "John" , "Smith" , "12345" , "34567"};

	for(int i=0;i<10;i++){
		vlac.setAttribute(i,schema,record1[i]);
	}

//	printData(vlac.data , 69);

	for(int i=0;i<10;i++){


		ASSERT( record1[i] == vlac.getAttribute(i , schema) );
	}

	//////////////////////////////

	std::string record2[10] =
	{"John Black Patterson" , "0" , "22220344567" , "Professor" , "70.4567" , "12000" , "John" , "" , "1235" , "3467"};

	for(int i=0;i<10;i++){
		vlac.setAttribute(i,schema,record2[i]);
	}


	for(int i=0;i<10;i++){

		ASSERT( record2[i] == vlac.getAttribute(i , schema) );
	}

	/////////////////////////////////

	std::string record3[10] =
	{"" , "0" , "22220344567" , "Professor" , "-70.4567" , "12000" , "John" , "Smith Patterson" , "9835" , "3467"};

	for(int i=0;i<10;i++){
		vlac.setAttribute(i,schema,record3[i]);
	}

	for(int i=0;i<10;i++){

		ASSERT( record3[i] == vlac.getAttribute(i , schema) );
	}

	//////////////////////////////////////////////

	std::string record4[10] =
	{"" , "0" , "22220344567" , "Professor" , "70.4567" , "12000" , "John" , "Smith Patterson" , "9835" , "3467"};

	for(int i=0;i<10;i++){
		vlac.setAttribute(i,schema,record4[i]);
	}

	ASSERT(vlac.getFloatAttribute(4,schema) == float(70.4567));
	ASSERT(vlac.getUnsignedAttribute(8,schema) == 9835);
	ASSERT(vlac.getTextAttribute(3,schema) == "Professor");
	ASSERT(vlac.getTimeAttribute(2,schema) == 22220344567);

	/////////////////////////////////////////////

	std::string record5[10] =
	{"David Simpson" , "3245" , "22220344567" , "Professor" , "70.4567" , "12000" , "John" , "Smith Patterson" , "9835" , "3467"};

	vlac.setAttribute(2,schema,record5[2]);

	ASSERT(vlac.getFloatAttribute(4,schema) == float(70.4567));
	ASSERT(vlac.getUnsignedAttribute(8,schema) == 9835);
	ASSERT(vlac.getTextAttribute(3,schema) == "Professor");
	ASSERT(vlac.getTimeAttribute(2,schema) == 22220344567);

	////////////////////////////////////////////

	std::string record6[10] =
	{"" , "0" , "22220344567" , "Professor" , "70.4567" , "12000" , "John" , "Smith Patterson" , "9835" , "3467"};

	for(int i=0;i<10;i++){
		vlac.setAttribute(i,schema,record6[i]);
	}

	std::vector<Score> results;
	std::vector<unsigned> attributes;
	attributes.push_back(2);
	attributes.push_back(3);
	attributes.push_back(5);
	attributes.push_back(7);
	attributes.push_back(8);

	vlac.getBatchOfAttributes(attributes , schema, &results);

	ASSERT(results.at(0).getTimeScore() == 22220344567);
	ASSERT(results.at(1).getTextScore().compare("Professor") == 0);
	ASSERT(results.at(2).getIntScore() == 12000);
	ASSERT(results.at(3).getTextScore().compare("Smith Patterson") == 0);
	ASSERT(results.at(4).getIntScore() == 9835);




}

int main(int argc, char *argv[]){


	test_1();
	test_2();

    cout << "Variable Length Attribute Container unit tests : Passed" << endl;

	return 0;
}
