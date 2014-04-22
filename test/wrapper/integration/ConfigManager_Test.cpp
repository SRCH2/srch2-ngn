#include "util/xmlParser/pugixml.hpp"
#include <iostream>
#include <string>
#include <stdlib.h>
#include "util/Assert.h"
#include <assert.h>

#include "ConfigManager.h"
using srch2http::ConfigManager;

using namespace std;
using namespace srch2::instantsearch;

void test1(char* xmlFile) {
    cout << endl << "#######################################################" << endl;
    cout << "XML Parser Test1: START" << endl << endl;

    pugi::xml_document doc;
    if (!doc.load_file(xmlFile)) {
        cout << "file " << xmlFile << " parsed with errors." << endl;
        cout << "Test1 is incomplete" << endl;
        ASSERT(false);
        return;
    }
    pugi::xml_node allSearchEngines = doc.child("xml");

   for (pugi::xml_node searchEngine = allSearchEngines.first_child(); searchEngine;
           searchEngine = searchEngine.next_sibling()) {
       cout << "searchEngine: ";

       for (pugi::xml_attribute attr = searchEngine.first_attribute(); attr; attr = attr.next_attribute()) {
           cout << " " << attr.name() << "=" << attr.value();
       }
       cout << endl;

       for (pugi::xml_node property  = searchEngine.first_child(); property;
               property = property.next_sibling()) {

                for (pugi::xml_attribute attr = property.first_attribute(); attr; attr = attr.next_attribute()) {
                    cout << "    " << property.name() << "=" << attr.value() << endl;
                }
                if (property.text()){
                    cout << "    " << property.name() << "=" << property.child_value() << endl;
                }
       }

       cout << endl;
   }
   cout << "XML Parser Test1: FINISHED" << endl;
   cout << "#######################################################" << endl << endl;
}


void test2(char* configFile)
{
	ConfigManager *configManager = new ConfigManager(configFile);

	configManager->loadConfigFile();

	ASSERT(configManager->getSrch2Home() == "./multicore//");
}

int main() {
    //test1(getenv("ConfigManagerFilePath"));
    test2(getenv("ConfigManagerFilePath"));
}
