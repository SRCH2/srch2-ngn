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
#include "instantsearch/TypedValue.h"

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




	Byte * vlac = NULL;
	unsigned vlacSize;
	std::string record1[10] =
	{"John Smith" , "23" , "2344567" , "Doctor" , "70.4567" , "12000" , "John" , "Smith" , "12345" , "34567"};
	vector<vector<string> > nonSearchableAttributeValues;
	for(int i=0;i<10;i++){
		vector<string> temp;
		temp.push_back(record1[i]);
		nonSearchableAttributeValues.push_back(temp);
	}
	VariableLengthAttributeContainer::fill(schema, nonSearchableAttributeValues , vlac , vlacSize);
	for(int i=0;i<10;i++){
	    if(record1[i] != VariableLengthAttributeContainer::getAttribute(i , schema, vlac)){
	        ASSERT(false);
	    }
	}
	//////////////////////////////

	std::string record2[10] =
	{"John Black Patterson" , "0" , "20344567" , "Professor" , "70.4567" , "12000" , "John" , "" , "1235" , "3467"};
	nonSearchableAttributeValues.clear();
	VariableLengthAttributeContainer::clear(vlac , vlacSize );
	for(int i=0;i<10;i++){
		vector<string> temp;
		temp.push_back(record2[i]);
		nonSearchableAttributeValues.push_back(temp);
	}
	VariableLengthAttributeContainer::fill(schema,nonSearchableAttributeValues, vlac , vlacSize);
	for(int i=0;i<10;i++){
		ASSERT( record2[i] == VariableLengthAttributeContainer::getAttribute(i , schema, vlac) );
	}
	/////////////////////////////////

	std::string record3[10] =
	{"" , "0" , "20344567" , "Professor" , "-70.4567" , "12000" , "John" , "Smith Patterson" , "9835" , "3467"};
    nonSearchableAttributeValues.clear();
    VariableLengthAttributeContainer::clear(vlac, vlacSize);
	for(int i=0;i<10;i++){
		vector<string> temp;
		temp.push_back(record3[i]);
		nonSearchableAttributeValues.push_back(temp);
	}
    VariableLengthAttributeContainer::fill(schema,nonSearchableAttributeValues , vlac, vlacSize);
	for(int i=0;i<10;i++){
		ASSERT( record3[i] == VariableLengthAttributeContainer::getAttribute(i , schema,vlac) );
	}
	//////////////////////////////////////////////

	std::string record4[10] =
	{"" , "0" , "20344567" , "Professor" , "70.4567" , "12000" , "John" , "Smith Patterson" , "9835" , "3467"};
    nonSearchableAttributeValues.clear();
    VariableLengthAttributeContainer::clear(vlac , vlacSize);
	for(int i=0;i<10;i++){
		vector<string> temp;
		temp.push_back(record4[i]);
		nonSearchableAttributeValues.push_back(temp);
	}
    VariableLengthAttributeContainer::fill(schema,nonSearchableAttributeValues, vlac, vlacSize);
	ASSERT(VariableLengthAttributeContainer::getFloatAttribute(4,schema,vlac) == float(70.4567));
	ASSERT(VariableLengthAttributeContainer::getUnsignedAttribute(8,schema,vlac) == 9835);
	ASSERT(VariableLengthAttributeContainer::getTextAttribute(3,schema,vlac) == "Professor");
	ASSERT(VariableLengthAttributeContainer::getTimeAttribute(2,schema,vlac) == 20344567);
	/////////////////////////////////////////////

	std::string record5[10] =
	{"David Simpson" , "3245" , "20344567" , "Professor" , "70.4567" , "12000" , "John" , "Smith Patterson" , "9835" , "3467"};
    nonSearchableAttributeValues.clear();
    VariableLengthAttributeContainer::clear(vlac, vlacSize);
	for(int i=0;i<10;i++){
		vector<string> temp;
		temp.push_back(record5[i]);
		nonSearchableAttributeValues.push_back(temp);
	}
    VariableLengthAttributeContainer::fill(schema,nonSearchableAttributeValues, vlac, vlacSize);
	ASSERT(VariableLengthAttributeContainer::getFloatAttribute(4,schema,vlac) == float(70.4567));
	ASSERT(VariableLengthAttributeContainer::getUnsignedAttribute(8,schema,vlac) == 9835);
	ASSERT(VariableLengthAttributeContainer::getTextAttribute(3,schema,vlac) == "Professor");
	ASSERT(VariableLengthAttributeContainer::getTimeAttribute(2,schema,vlac) == 20344567);
	////////////////////////////////////////////

	std::string record6[10] =
	{"" , "0" , "20344567" , "Professor" , "70.4567" , "12000" , "John" , "Smith Patterson" , "9835" , "3467"};
    nonSearchableAttributeValues.clear();
    VariableLengthAttributeContainer::clear(vlac,vlacSize);
	for(int i=0;i<10;i++){
		vector<string> temp;
		temp.push_back(record6[i]);
		nonSearchableAttributeValues.push_back(temp);
	}
    VariableLengthAttributeContainer::fill(schema,nonSearchableAttributeValues,vlac,vlacSize);
	std::vector<TypedValue> results;
	std::vector<unsigned> attributes;
	attributes.push_back(2);
	attributes.push_back(3);
	attributes.push_back(5);
	attributes.push_back(7);
	attributes.push_back(8);
	VariableLengthAttributeContainer::getBatchOfAttributes(attributes , schema, vlac, &results);
	ASSERT(results.at(0).getTimeTypedValue() == 20344567);
	ASSERT(results.at(1).getTextTypedValue().compare("Professor") == 0);
	ASSERT(results.at(2).getIntTypedValue() == 12000);
	ASSERT(results.at(3).getTextTypedValue().compare("Smith Patterson") == 0);
	ASSERT(results.at(4).getIntTypedValue() == 9835);
}

// test getter and setters
// schema : TEXT,	UNSIGNED,	TIME,	TEXT,	FLOAT,	UNSIGNED,	TEXT,	TEXT,	UNSIGNED,	UNSIGNED
// semantic:NAME,	AGE,		bdate,	title,	weight,	salary,		fname,	lname,	ID,			EID
void test_2(){

    ///Create Schema
    Schema *schema = Schema::create(srch2::instantsearch::DefaultIndex);

    schema->setNonSearchableAttribute("text1_name" , srch2::instantsearch::ATTRIBUTE_TYPE_TEXT, "text1");
    schema->setNonSearchableAttribute("int1_unsigned", srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED, "1" );
    schema->setNonSearchableAttribute("time1_bdate", srch2::instantsearch::ATTRIBUTE_TYPE_TIME, "0" );
    schema->setNonSearchableAttribute("text2_title", srch2::instantsearch::ATTRIBUTE_TYPE_TEXT, "text2" , true );
    schema->setNonSearchableAttribute("float1_weight", srch2::instantsearch::ATTRIBUTE_TYPE_FLOAT, "0.1" );
    schema->setNonSearchableAttribute("int2_salary", srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED, "2" );
    schema->setNonSearchableAttribute("text3_fname" , srch2::instantsearch::ATTRIBUTE_TYPE_TEXT, "text3");
    schema->setNonSearchableAttribute("text4_lname" , srch2::instantsearch::ATTRIBUTE_TYPE_TEXT, "text4");
    schema->setNonSearchableAttribute("int3_id", srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED, "3" );
    schema->setNonSearchableAttribute("int4_eid", srch2::instantsearch::ATTRIBUTE_TYPE_UNSIGNED, "4" , true);
    schema->setSearchableAttribute("searchable_name" , 2);
    schema->setSearchableAttribute("text1_name" , 2);



	Byte * vlac = NULL;
	unsigned vlacSize;
	std::string record1[10] =
	{"John Smith" , "23" , "2344567" , "Doctor"/*and also "Pilot" "Teacher"*/ , "70.4567" , "12000" , "John" , "Smith" , "12345" , "34567" /*and also "34568" "34569"*/};
	vector<vector<string> > nonSearchableAttributeValues;
	for(int i=0;i<10;i++){
		vector<string> temp;
		temp.push_back(record1[i]);
		if(i == schema->getNonSearchableAttributeId("text2_title")){ // /*and also "Pilot" "Teacher"*/
			temp.push_back("Pilot");
			temp.push_back("Teacher");
		}
		if(i == schema->getNonSearchableAttributeId("int4_eid")){ // /*and also "34568" "34569"*/
			temp.push_back("34568");
			temp.push_back("34569");
		}
		nonSearchableAttributeValues.push_back(temp);
	}
	VariableLengthAttributeContainer::fill(schema, nonSearchableAttributeValues , vlac , vlacSize);
	for(int i=0;i<10;i++){
		if(i == schema->getNonSearchableAttributeId("text2_title")){
			vector<string> jobs = VariableLengthAttributeContainer::getMultiTextAttribute(i , schema , vlac);
			ASSERT(jobs.size() == 3);
			ASSERT(jobs.at(0).compare("Doctor") == 0);
			ASSERT(jobs.at(1).compare("Pilot") == 0);
			ASSERT(jobs.at(2).compare("Teacher") == 0);
			continue;
		}
		if(i == schema->getNonSearchableAttributeId("int4_eid")){
			vector<unsigned> numbers = VariableLengthAttributeContainer::getMultiUnsignedAttribute(i , schema , vlac);
			ASSERT(numbers.size() == 3);
			ASSERT(numbers.at(0) == 34567);
			ASSERT(numbers.at(1) == 34568);
			ASSERT(numbers.at(2) == 34569);
			continue;
		}
	    if(record1[i] != VariableLengthAttributeContainer::getAttribute(i , schema, vlac)){
	        ASSERT(false);
	    }
	}
	//////////////////////////////

	std::string record2[10] =
	{"John Black Patterson" , "0" , "20344567" , "Professor" , "70.4567" , "12000" , "John" , "" , "1235" , "3467"};
	nonSearchableAttributeValues.clear();
	VariableLengthAttributeContainer::clear(vlac , vlacSize );
	for(int i=0;i<10;i++){

		vector<string> temp;
		temp.push_back(record2[i]);
		nonSearchableAttributeValues.push_back(temp);
	}
	VariableLengthAttributeContainer::fill(schema,nonSearchableAttributeValues, vlac , vlacSize);
	for(int i=0;i<10;i++){
		if(i == schema->getNonSearchableAttributeId("text2_title")){
			vector<string> jobs = VariableLengthAttributeContainer::getMultiTextAttribute(i , schema , vlac);
			ASSERT(jobs.size() == 1);
			ASSERT(jobs.at(0).compare("Professor") == 0);
			continue;
		}
		if(i == schema->getNonSearchableAttributeId("int4_eid")){
			vector<unsigned> numbers = VariableLengthAttributeContainer::getMultiUnsignedAttribute(i , schema , vlac);
			ASSERT(numbers.size() == 1);
			ASSERT(numbers.at(0) == 3467);
			continue;
		}
		ASSERT( record2[i] == VariableLengthAttributeContainer::getAttribute(i , schema, vlac) );
	}
	/////////////////////////////////

	std::string record3[10] =
	{"" , "0" , "20344567" , "Professor" , "-70.4567" , "12000" , "John" , "Smith Patterson" , "9835" , "3467"};
    nonSearchableAttributeValues.clear();
    VariableLengthAttributeContainer::clear(vlac, vlacSize);
	for(int i=0;i<10;i++){
		vector<string> temp;
		if(i == schema->getNonSearchableAttributeId("text2_title")){ // /*and also "Pilot" "Teacher"*/
			temp.push_back(record3[i]);
			temp.push_back("Pilot");
			temp.push_back("Teacher");
		}else if(i != schema->getNonSearchableAttributeId("int4_eid")){ //
			temp.push_back(record3[i]);
		}
		nonSearchableAttributeValues.push_back(temp);

	}
    VariableLengthAttributeContainer::fill(schema,nonSearchableAttributeValues , vlac, vlacSize);
	for(int i=0;i<10;i++){
		if(i == schema->getNonSearchableAttributeId("text2_title")){
			vector<string> jobs = VariableLengthAttributeContainer::getMultiTextAttribute(i , schema , vlac);
			ASSERT(jobs.size() == 3);
			ASSERT(jobs.at(0).compare("Professor") == 0);
			ASSERT(jobs.at(1).compare("Pilot") == 0);
			ASSERT(jobs.at(2).compare("Teacher") == 0);
			continue;
		}
		if(i == schema->getNonSearchableAttributeId("int4_eid")){
			vector<unsigned> numbers = VariableLengthAttributeContainer::getMultiUnsignedAttribute(i , schema , vlac);
			ASSERT(numbers.size() == 0);
			continue;
		}
		ASSERT( record3[i] == VariableLengthAttributeContainer::getAttribute(i , schema,vlac) );
	}
	//////////////////////////////////////////////

	std::string record4[10] =
	{"" , "0" , "20344567" , "Professor" , "70.4567" , "12000" , "John" , "Smith Patterson" , "9835" , "3467"};
    nonSearchableAttributeValues.clear();
    VariableLengthAttributeContainer::clear(vlac , vlacSize);
	for(int i=0;i<10;i++){
		vector<string> temp;
		if(i != schema->getNonSearchableAttributeId("int4_eid") && i != schema->getNonSearchableAttributeId("text2_title")){ //
			temp.push_back(record4[i]);
		}
		nonSearchableAttributeValues.push_back(temp);
	}
    VariableLengthAttributeContainer::fill(schema,nonSearchableAttributeValues, vlac, vlacSize);
	vector<string> jobs = VariableLengthAttributeContainer::getMultiTextAttribute(3 , schema , vlac);
	ASSERT(jobs.size() == 0);
	vector<unsigned> numbers = VariableLengthAttributeContainer::getMultiUnsignedAttribute(9 , schema , vlac);
	ASSERT(numbers.size() == 0);

	ASSERT(VariableLengthAttributeContainer::getFloatAttribute(4,schema,vlac) == float(70.4567));
	ASSERT(VariableLengthAttributeContainer::getUnsignedAttribute(8,schema,vlac) == 9835);
	ASSERT(VariableLengthAttributeContainer::getTimeAttribute(2,schema,vlac) == 20344567);
	/////////////////////////////////////////////

}


int main(int argc, char *argv[]){

	test_1();

	test_2();

    cout << "Variable Length Attribute Container unit tests : Passed" << endl;

	return 0;
}
