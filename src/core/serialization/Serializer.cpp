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
/*
 * Serializer.cpp
 *
 *  Created on: Oct 9, 2013
 *      Author: Surendra
 */

#include "Serializer.h"
namespace srch2 {
namespace instantsearch {

/*
 *  Initialize static variable currentVersion
 */
IndexVersion IndexVersion::currentVersion(INDEX_VERSION, BOOST_VERSION);

IndexVersion::IndexVersion(uint8_t internalVersion, unsigned boostVersion){
	this->sequentialId = internalVersion;
	this->boostVersion = boostVersion;
	/*
	 *  endianness will be 0 for big endian or 1 for small endian
	 */
	unsigned endianess = 0x01;
	this->endianness = ((uint8_t *)&endianess)[0]; // store first byte of starting address
	/*
	 *  We should only care about the size of pointer here. 64 bit systems may have pointer size
	 *  4 bytes or 8 bytes depending on the system configuration. If we read an index file saved by
	 *  the engine on any system whose pointer size matches with the current running engine's
	 *  pointer size then we are fine with it.
	 */
	this->bitness = sizeof(void *);
}
Serializer::Serializer() {
}
Serializer::~Serializer() {
}

}
}

