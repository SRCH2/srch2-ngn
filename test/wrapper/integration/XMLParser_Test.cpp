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
#include "server/util/xmlParser/pugixml.hpp"
#include <iostream>
#include <string>
#include <stdlib.h>
#include "util/Assert.h"
#include <assert.h>

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


int main() {
    test1(getenv("XmlFilePath"));
}
