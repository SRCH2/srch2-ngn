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
#include <iostream>
#include "license/LicenseVerifier.h"
#include <util/Assert.h>
using namespace srch2::instantsearch;

int main(int argc, char **argv)
{
	/// The input string must be strictly in format Expiry-Date=YYYY-MM-DD
	/// System crashes if the ExpiryDate in the Signature that is generated and sent to Alice, is not in this format.

	//Correct Signature, ExpiryDate has Passed
    bool resultFail1 = LicenseVerifier::test("Signature=L3M4GvCg6tiK6dszXfljijCT10+BObjQRy8I5BGBxHt4Fe6x79JXg4ln+p8TeUXuYFQlZ5xkfOIAf/w2x375tfK8zLSAgyjvrvK8dibUIUvXAJKtfhBEMZo8ZqrPoJ0VU+iQin1eLBmGNXO0XDjYEBgJkOLIRVTaAXnFOVUG5Rw=,Expiry-Date=2010-09-12");
    ASSERT(resultFail1 == 0);
    std::cerr << "License test: " << resultFail1 << std::endl;

    //Wrong key
    bool resultFail2 = LicenseVerifier::test("Signature=L3M4GvCg6tiK6dszXfljijCT10+BObjQRy8I5BGBxHt4Fe6x79JXg4ln+p8TeUXuYFQlZ5xkfOIAf/w2x375tfK8zLSAgyjvrvK8dibUIUvXAJKtfhBEMZo8ZqrPoJ0VU+iQin1eLBmGNXO0XDjYEBgJkOLIRVTaAXnFOVUG5Rw=,Expiry-Date=2010-09-13");
    ASSERT(resultFail2 == 0);
    std::cerr << "License test: " << resultFail2 << std::endl;

    //Correct Signature and ExpiryDate changed
    bool resultFail3 = LicenseVerifier::test("Signature=eTwLb3n1cYBYsR6reo/uSlFLwu80cxqjO+t3LrtcqqvgpkCyIEryd2ajMqCneqHN8vBpfI4jcoKSXtQLJGIvCurZxkuHdBwT7eCOCuauoXCOK+0MKeLI4PNggEQAQSxDXZci9QEN2d7NSeEKG+VUl3MZQTUz5CP45H3htzRlozU=,name=srch2_license,Expiry-Date=2020-08-02");
    ASSERT(resultFail3 == 0);
    std::cerr << "License test: " << resultFail3 << std::endl;

    //CorrectKey and ExpiryDate valid
    bool resultPass = LicenseVerifier::test("Signature=eTwLb3n1cYBYsR6reo/uSlFLwu80cxqjO+t3LrtcqqvgpkCyIEryd2ajMqCneqHN8vBpfI4jcoKSXtQLJGIvCurZxkuHdBwT7eCOCuauoXCOK+0MKeLI4PNggEQAQSxDXZci9QEN2d7NSeEKG+VUl3MZQTUz5CP45H3htzRlozU=,name=srch2_license,Expiry-Date=2020-08-01");
    ASSERT(resultPass == 1);
    std::cerr << "License test: " << resultPass << std::endl;

}
