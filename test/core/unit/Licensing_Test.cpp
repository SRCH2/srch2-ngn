#include <iostream>
#include "license/LicenseVerifier.h"
#include <util/Assert.h>
using namespace bimaple::instantsearch;

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
    bool resultFail3 = LicenseVerifier::test("Signature=Nqh+DH22KBxY9t1unwmbWxI1DdZTYvJyqF4YPoT4f9a218BcMJRcNgBeU1ZizZG0Jl+qyTfeBtoR+onnZmYq4ZoksMQe5EuP7OVNK4IEsg9wpBxW48i1oFRnTH16LhsBbPtEJ1I6GWHgUw6lHInPXJBHadYFOK2xzuzccldTcxY=,name=bimaple_license,Expiry-Date=2020-08-02");
    ASSERT(resultFail3 == 0);
    std::cerr << "License test: " << resultFail3 << std::endl;

    //CorrectKey and ExpiryDate valid
    bool resultPass = LicenseVerifier::test("Signature=Nqh+DH22KBxY9t1unwmbWxI1DdZTYvJyqF4YPoT4f9a218BcMJRcNgBeU1ZizZG0Jl+qyTfeBtoR+onnZmYq4ZoksMQe5EuP7OVNK4IEsg9wpBxW48i1oFRnTH16LhsBbPtEJ1I6GWHgUw6lHInPXJBHadYFOK2xzuzccldTcxY=,name=bimaple_license,Expiry-Date=2020-08-01");
    ASSERT(resultPass == 1);
    std::cerr << "License test: " << resultPass << std::endl;

}
