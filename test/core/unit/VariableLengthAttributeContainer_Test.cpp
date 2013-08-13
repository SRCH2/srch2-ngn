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


// test getter and setters
// schema : TEXT,	UNSIGNED,	TIME,	TEXT,	FLOAT,	UNSIGNED,	TEXT,	TEXT,	UNSIGNED,	UNSIGNED
// semantic:NAME,	AGE,		bdate,	title,	weight,	salary,		fname,	lname,	ID,			EID
void test_1(){

    ///Create Schema
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);

    schema->setNonSearchableAttribute("text1_name" , srch2::instantsearch::ATTRIBUTE_TYPE_TEXT, "text1");
    schema->setNonSearchableAttribute("int1_unsigned", srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED, "1" );
    schema->setNonSearchableAttribute("time1_bdate", srch2::instantsearch::ATTRIBUTE_TYPE_TIME, "0" );
    schema->setNonSearchableAttribute("text2_title", srch2::instantsearch::ATTRIBUTE_TYPE_TEXT, "text2" );
    schema->setNonSearchableAttribute("float1_weight", srch2::instantsearch::ATTRIBUTE_TYPE_FLOAT, "0.1" );
    schema->setNonSearchableAttribute("int2_salary", srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED, "2" );
    schema->setNonSearchableAttribute("text3_fname" , srch2::instantsearch::ATTRIBUTE_TYPE_TEXT, "text3");
    schema->setNonSearchableAttribute("text4_lname" , srch2::instantsearch::ATTRIBUTE_TYPE_TEXT, "text4");
    schema->setNonSearchableAttribute("int3_id", srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED, "3" );
    schema->setNonSearchableAttribute("int4_eid", srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED, "4" );
    schema->setSearchableAttribute("searchable_name" , 2);
    schema->setSearchableAttribute("text1_name" , 2);




	VariableLengthAttributeContainer vlac;
	std::string record1[10] =
	{"John Smith" , "23" , "2000344567" , "Doctor" , "70.4567" , "12000" , "John" , "Smith" , "12345" , "34567"};
	vector<string> nonSearchableAttributeValues;
	nonSearchableAttributeValues.insert(nonSearchableAttributeValues.begin() , record1,record1 + 10);
	vlac.fill(schema, nonSearchableAttributeValues);
	for(int i=0;i<10;i++){
	    if(record1[i] != vlac.getAttribute(i , schema)){
	        ASSERT(false);
	    }
	}
	//////////////////////////////

	std::string record2[10] =
	{"John Black Patterson" , "0" , "22220344567" , "Professor" , "70.4567" , "12000" , "John" , "" , "1235" , "3467"};
	nonSearchableAttributeValues.clear();
	vlac.clear();
	nonSearchableAttributeValues.insert(nonSearchableAttributeValues.begin() , record2 , record2+10);
	vlac.fill(schema,nonSearchableAttributeValues);
	for(int i=0;i<10;i++){
		ASSERT( record2[i] == vlac.getAttribute(i , schema) );
	}
	/////////////////////////////////

	std::string record3[10] =
	{"" , "0" , "22220344567" , "Professor" , "-70.4567" , "12000" , "John" , "Smith Patterson" , "9835" , "3467"};
    nonSearchableAttributeValues.clear();
    vlac.clear();
    nonSearchableAttributeValues.insert(nonSearchableAttributeValues.begin() , record3 , record3+10);
    vlac.fill(schema,nonSearchableAttributeValues);
	for(int i=0;i<10;i++){
		ASSERT( record3[i] == vlac.getAttribute(i , schema) );
	}
	//////////////////////////////////////////////

	std::string record4[10] =
	{"" , "0" , "22220344567" , "Professor" , "70.4567" , "12000" , "John" , "Smith Patterson" , "9835" , "3467"};
    nonSearchableAttributeValues.clear();
    vlac.clear();
    nonSearchableAttributeValues.insert(nonSearchableAttributeValues.begin() , record4 , record4+10);
    vlac.fill(schema,nonSearchableAttributeValues);
	ASSERT(vlac.getFloatAttribute(4,schema) == float(70.4567));
	ASSERT(vlac.getUnsignedAttribute(8,schema) == 9835);
	ASSERT(vlac.getTextAttribute(3,schema) == "Professor");
	ASSERT(vlac.getTimeAttribute(2,schema) == 22220344567);
	/////////////////////////////////////////////

	std::string record5[10] =
	{"David Simpson" , "3245" , "22220344567" , "Professor" , "70.4567" , "12000" , "John" , "Smith Patterson" , "9835" , "3467"};
    nonSearchableAttributeValues.clear();
    vlac.clear();
    nonSearchableAttributeValues.insert(nonSearchableAttributeValues.begin() , record5 , record5+10);
    vlac.fill(schema,nonSearchableAttributeValues);
	ASSERT(vlac.getFloatAttribute(4,schema) == float(70.4567));
	ASSERT(vlac.getUnsignedAttribute(8,schema) == 9835);
	ASSERT(vlac.getTextAttribute(3,schema) == "Professor");
	ASSERT(vlac.getTimeAttribute(2,schema) == 22220344567);
	////////////////////////////////////////////

	std::string record6[10] =
	{"" , "0" , "22220344567" , "Professor" , "70.4567" , "12000" , "John" , "Smith Patterson" , "9835" , "3467"};
    nonSearchableAttributeValues.clear();
    vlac.clear();
    nonSearchableAttributeValues.insert(nonSearchableAttributeValues.begin() , record6 , record6+10);
    vlac.fill(schema,nonSearchableAttributeValues);
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


//	test_1();
	test_1();

    cout << "Variable Length Attribute Container unit tests : Passed" << endl;

	return 0;
}
