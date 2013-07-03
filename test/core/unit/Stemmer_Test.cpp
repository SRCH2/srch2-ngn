//$Id$

#include "analyzer/StemmerInternal.h"

#include <string.h>
#include <stdio.h>
#include <iostream>
#include "util/Assert.h"


using namespace std;
using namespace bimaple::instantsearch;


/*
 * test1() is testing the case where the 'tokens' are not present in the
 * map(headwords file).  Example of such words are agreed,aaron, abaissiez, abandon, abandoned, abase,
 * befall, befallen, befalls, cambric, cambrics, cambridge, cambyses
 */

void test1()

{

	Stemmer *stemmer_handler = new Stemmer();
	//std::map<std::string,int> headwords;
	//stemmer_handler->createHeadWordsMap(headwords);

	string test = "pencils";
	string result = stemmer_handler->stemToken(test);
	ASSERT(result=="pencil");
	cout<<result<<endl;
	test = "agreed";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="agre");
	cout<<result<<endl;
	test = "aaron";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="aaron");
	cout<<result<<endl;
	test = "abandon";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="abandon");
	cout<<result<<endl;
	test = "befall";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="befall");
	cout<<result<<endl;
	test = "befallen";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="befallen");
	cout<<result<<endl;
	test = "cambrics";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="cambric");
	cout<<result<<endl;
	test = "cambric";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="cambric");
	cout<<result<<endl;
	test = "cambyses";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="cambyse");
	cout<<result<<endl;



}



/*
 * test2() is testing the case where the 'tokens' are only present in the
 * map(headwords file).  Example of such words are achilles, acropolis, adams, adonis,neal
   netherlands, olympus, orleans, orly
 */


void test2()

{
	Stemmer *stemmer_handler = new Stemmer();

	string test = "achilles";
	string result = stemmer_handler->stemToken(test);
	ASSERT(result=="achilles");
	cout<<result<<endl;
	test = "acropolis";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="acropolis");
	cout<<result<<endl;
	test = "adams";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="adams");
	cout<<result<<endl;
	test = "adonis";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="adonis");
	cout<<result<<endl;
	test = "neal";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="neal");
	cout<<result<<endl;
	test = "netherlands";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="netherlands");
	cout<<result<<endl;
	test = "olympus";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="olympus");
	cout<<result<<endl;
	test = "orleans";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="orleans");
	cout<<result<<endl;
	test = "orly";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="orly");
	cout<<result<<endl;



}

/*
 * test3() is testing the case where some 'tokens' are present in the
 * map(headwords file) and some are not.  Example of such words are pershing, peter, peters, philippines,
 * accident,accidental,accidentally, accidents
 */

void test3()

{
	Stemmer *stemmer_handler = new Stemmer();

	string test = "pershing";
	string result = stemmer_handler->stemToken(test);
	ASSERT(result=="pershing");
	cout<<result<<endl;
	test = "peter";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="peter");
	cout<<result<<endl;
	test = "peters";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="peters");
	cout<<result<<endl;
	test = "phillippine";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="phillippine");
	cout<<result<<endl;
	test = "accident";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="accident");
	cout<<result<<endl;
	test = "accidental";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="accidental");
	cout<<result<<endl;
	test = "acidentally";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="acident");
	cout<<result<<endl;
	test = "orleans";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="orleans");
	cout<<result<<endl;
	test = "accidents";
	result = stemmer_handler->stemToken(test);
	ASSERT(result=="accid");
	cout<<result<<endl;
}


/*
 * test4() is testing the case where 'token' is empty!
 */

void test4()

{
	Stemmer *stemmer_handler = new Stemmer();

	string test = "";
	string result = stemmer_handler->stemToken(test);
	cout << result;
	ASSERT(result=="");
}

int main(int argc, char *argv[])
{

	test1();

	test2();

	test3();

	test4();

	cout<<"\n\nStemmer unit tests passed!!";

	return 0;

}

