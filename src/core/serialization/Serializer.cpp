/*
 * Serializer.cpp
 *
 *  Created on: Oct 9, 2013
 *      Author: srch2
 */

#include "Serializer.h"
namespace srch2 {
namespace instantsearch {

/*
 *  Initialize static variable currentVersion
 */
IndexVersion IndexVersion::currentVersion(CURRENT_INDEX_VERSION, BOOST_VERSION);

IndexVersion::IndexVersion(uint8_t internalVersion, unsigned boostVersion){
	this->internalVersion = internalVersion;
	this->boostVersion = boostVersion;
	/*
	 *  endianness will be 0 for big endian or 1 for small endian
	 */
	unsigned endianess = 0x01;
	this->endianness = ((uint8_t *)&endianess)[0]; // store first byte of starting address
	/*
	 *  We should care about here is size of pointer. Some 64 bit system may be have pointer size
	 *  4 byte and some may have 8 byte. If we read a index file saved by the engine on any system
	 *  whose pointer size matches with the current running engine's system then we should be fine.
	 */
	this->bitness = sizeof(void *);
}
Serializer::Serializer() {
}
Serializer::~Serializer() {
}

}
}

