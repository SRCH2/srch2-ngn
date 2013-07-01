#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <exception>

using std::exception;

#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#include "license/LicenseVerifier.h"

namespace srch2
{
namespace instantsearch
{

static const std::string base64_chars = 
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

static inline bool is_base64(unsigned char c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

class Base64Decoder {
public:
	static std::string decode(const std::string& encoded_string)
	{
		int in_len = encoded_string.size();
		int i = 0;
		int j = 0;
		int in_ = 0;
		unsigned char char_array_4[4], char_array_3[3];
		std::string ret;

		while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
			char_array_4[i++] = encoded_string[in_]; in_++;
			if (i ==4) {
				for (i = 0; i <4; i++)
					char_array_4[i] = base64_chars.find(char_array_4[i]);

				char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
				char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
				char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

				for (i = 0; (i < 3); i++)
					ret += char_array_3[i];
				i = 0;
			}
		}

		if (i) {
			for (j = i; j <4; j++)
				char_array_4[j] = 0;

			for (j = 0; j <4; j++)
				char_array_4[j] = base64_chars.find(char_array_4[j]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
		}

		return ret;
	}
};

static const char *PUB_KEY =
		"-----BEGIN PUBLIC KEY-----\n"
		"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC4m+K1VswPvpeL/jCb0qc6TmF0\n"
		"aY+F0zuExIXDS1vWBYGCpPm8cttplienaBNfsGZ+ZL7MkEvB3519mGFWcPykV+vC\n"
		"gV53vp9uLDOLHDLZVFRrbXN+gOCVG3jaY3TkQH8RGLETLJRoTKRT+EeEqg4Kg81J\n"
		"f46avGyDoWwKKNUxCwIDAQAB\n"
		"-----END PUBLIC KEY-----";

class PublicKeyInitializer {
public:
	RSA *rsa;

	PublicKeyInitializer()
	{
		BIO *b = NULL;
		b = BIO_new_mem_buf(const_cast<char *>(PUB_KEY), strlen(PUB_KEY));
		EVP_PKEY *pkey = NULL;
		pkey = PEM_read_bio_PUBKEY(b, NULL, NULL, NULL);

		rsa = EVP_PKEY_get1_RSA(pkey);
	}
};

static RSA *getRSAPublicKey()
{
	static PublicKeyInitializer pki;
	return pki.rsa;
}

/// The input string must be strictly in format: YYYY-MM-DD
/// Example: 2012-04-27
bool isLicenseDateValid(std::string expiryDate)
{
	unsigned year = atoi(expiryDate.substr(0, 4).c_str());
	unsigned month = atoi(expiryDate.substr(5, 2).c_str());

	// fixed by CHENLI. Day is in 2 digits
	//unsigned day = atoi(expiryDate.substr(8, 3).c_str()); 
	unsigned day = atoi(expiryDate.substr(8, 2).c_str());

	// fixed by CHENLI. Need to "-1" for month
	//struct std::tm endDate = {0, 0, 0, day, month, year - 1900};
	struct std::tm endDate = {0, 0, 0, day, month - 1, year - 1900}; // CHENLI

	time_t timer = time(NULL);
	struct std::tm* currentDate = gmtime(&timer);

	//on error, -1 is returned by mktime
	std::time_t x = std::mktime(currentDate);
	std::time_t y = std::mktime(&endDate);

	if ( x != (std::time_t)(-1) && y != (std::time_t)(-1) )
	{
		double difference = std::difftime(y, x);
		if(difference < 0) {
			return false;
		}
	}
	return true;
}

bool LicenseVerifier::test(const std::string& license)
{
	std::string::size_type splitIdx = license.find(',');
	std::string signature(license.substr(0, splitIdx));
	std::string unsignedLicense(license.substr(splitIdx + 1));

	SHA_CTX sha_ctx = { 0 };
	unsigned char digest[SHA_DIGEST_LENGTH];
	int rc;

	if ((rc = SHA1_Init(&sha_ctx)) != 1) {
		return false;
	}

	if ((rc = SHA1_Update(&sha_ctx, unsignedLicense.c_str(), unsignedLicense.length())) != 1) {
		return false;
	}

	if ((rc = SHA1_Final(digest, &sha_ctx)) != 1) {
		return false;
	}

	std::string base64Sign(signature.substr(signature.find('=') + 1));
	std::string decoded(Base64Decoder::decode(base64Sign));

	if ((rc = RSA_verify(NID_sha1, digest, sizeof digest, (unsigned char *)decoded.c_str(), decoded.length(), getRSAPublicKey())) != 1) {
		return false;
	}

	std::string::size_type splitDate = unsignedLicense.find("Expiry-Date=");
	std::string expiryDate(unsignedLicense.substr(splitDate + 12, splitDate + 10));

	if ( isLicenseDateValid(expiryDate) == false) {
		return false;
	}
	return true;
}

bool LicenseVerifier::testFile(const std::string& filenameWithPath)
{
	std::string license_dir = "";
	std::string license_file = "";
	std::string line;
	std::ifstream infile;
	if (filenameWithPath.compare("") == 0)
	{
		return testWithEnvironmentVariable();
	}
	else
	{
		try
		{
			//license_dir = std::string(getenv("srch2_license_dir")); /// getenv could return NULL and app will give seg fault if assigned to string
			//license_file = license_dir + "/srch2_license_key.txt";
			license_file = filenameWithPath;
			infile.open (license_file.c_str());
		}
		catch (exception& e)
		{
			std::cerr << "Cannot read the license key file. Check \"license-file\" in the configuration file, which defines the location of the license key file.\n";
			abort();
		}

		if (infile.good())
		{
			getline(infile,line); // Saves the line in STRING.
			if(line.length()>0 && line[line.length()-1] == '\r') line.resize(line.length()-1);
		}
		else
		{
			std::cerr << "Cannot read the license key file. Check \"license-file\" in the configuration file, which defines the location of the license key file.\n";
			abort();
		}

		infile.close();

		if (! test(line))
		{
			std::cerr << "License key file invalid. Please provide a valid license key file. Feel free to contact contact@srch2.com\n";
			abort();
		}
		return true;
	}
}

bool LicenseVerifier::testWithEnvironmentVariable()
{
	std::string license_dir = "";
	std::string license_file = "";
	std::string line;
	std::ifstream infile;
	try
	{
		license_dir = std::string(getenv("srch2_license_dir")); /// getenv could return NULL and app will give seg fault if assigned to string
		license_file = license_dir + "/srch2_license_key.txt";
		infile.open (license_file.c_str());
	}
	catch (exception& e)
	{
		std::cerr << "Cannot read the license key file. Check the environment variable \"srch2_license_dir\", which defines the folder that includes the license key file.\n";
		abort();
	}

	if (infile.good())
	{
		getline(infile,line); // Saves the line in STRING.
	}
	else
	{
		std::cerr << "Cannot read the license key file. Check the environment variable \"srch2_license_dir\", which defines the folder that includes the license key file.\n";
		abort();
	}

	infile.close();

	if (! test(line))
	{
		std::cerr << "License key file invalid. Please provide a valid license key file. Feel free to contact contact@srch2.com\n";
		abort();
	}

	return true;
}


}}


