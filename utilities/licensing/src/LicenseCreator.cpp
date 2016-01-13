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
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

class Base64Encoder {
    public:
        static std::string encode(unsigned char const* bytes_to_encode, unsigned int in_len)
        {
            std::string ret;
            int i = 0;
            int j = 0;
            unsigned char char_array_3[3];
            unsigned char char_array_4[4];

            while (in_len--) {
                char_array_3[i++] = *(bytes_to_encode++);
                if (i == 3) {
                    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                    char_array_4[3] = char_array_3[2] & 0x3f;

                    for(i = 0; (i <4) ; i++)
                        ret += base64_chars[char_array_4[i]];
                    i = 0;
                }
            }

            if (i)
            {
                for(j = i; j < 3; j++)
                    char_array_3[j] = '\0';

                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (j = 0; (j < i + 1); j++)
                    ret += base64_chars[char_array_4[j]];

                while((i++ < 3))
                    ret += '=';

            }

            return ret;
        }
};

int sign_data(const void *buf, size_t buf_len, void *pkey, size_t pkey_len, void **out_sig, size_t *out_sig_len) {
    int status = EXIT_SUCCESS;
    int rc = 1;

    SHA_CTX sha_ctx = { 0 };
    unsigned char digest[SHA_DIGEST_LENGTH];
    unsigned char *sig = NULL;
    unsigned int sig_len = 0;
    BIO *b = NULL;
    RSA *r = NULL;

    rc = SHA1_Init(&sha_ctx);
    if (1 != rc) { status = EXIT_FAILURE; goto end; }

    rc = SHA1_Update(&sha_ctx, buf, buf_len);
    if (1 != rc) { status = EXIT_FAILURE; goto end; }

    rc = SHA1_Final(digest, &sha_ctx);
    if (1 != rc) { status = EXIT_FAILURE; goto end; }

    std::cerr << "Digested..." << std::endl;

    b = BIO_new_mem_buf(pkey, pkey_len);
    std::cerr << "Allocated..." << b << std::endl;
    r = PEM_read_bio_RSAPrivateKey(b, NULL, NULL, NULL);
    std::cerr << "PKey read in..." << r << std::endl;

    sig = (unsigned char *)malloc(RSA_size(r));
    if (NULL == sig) { status = EXIT_FAILURE; goto end; }

    rc = RSA_sign(NID_sha1, digest, sizeof digest, sig, &sig_len, r);
    std::cerr << "Signed..." << std::endl;
    if (1 != rc) { status = EXIT_FAILURE; goto end; }
    *out_sig = sig;
    *out_sig_len = sig_len;
end:
    if (NULL != r) RSA_free(r);
    if (NULL != b) BIO_free(b);
    if (EXIT_SUCCESS != status) free(sig);
    if (1 != rc) fprintf(stderr, "OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));

    return status;
}

void readFile(char *fname, void **buffer, size_t *len)
{
    int fd = open(fname, O_RDONLY);
    *len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    std::cerr << *len << std::endl;
    *buffer = malloc(*len);
    size_t rem = *len;
    size_t p = 0;
    while(rem > 0) {
        ssize_t c = read(fd, (void *)(((uintptr_t)*buffer) + p), rem);
        p += c;
        rem -= c;
    }
    close(fd);
    std::cerr << "Done reading " << fname << std::endl;
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        std::cerr << "licensecreator <primary_key_file> <license_string>" << std::endl;
        return -1;
    }
    void *pkey;
    size_t pkeylen;
    readFile(argv[1], &pkey, &pkeylen);

    const void *data;
    size_t datalen;
    std::string lic(argv[2]);
    data = lic.c_str();
    datalen = lic.length();

    void *sign;
    size_t signlen;

    sign_data(data, datalen, pkey, pkeylen, &sign, &signlen);
    std::string sstr = Base64Encoder::encode((unsigned char *)sign, signlen);

    std::string signedData("Signature=");
    signedData += sstr;
    signedData += ',';
    signedData += argv[2];

    std::cerr << signedData << std::endl;

    free(pkey);
    free(sign);
}
