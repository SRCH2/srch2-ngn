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
	 *  4 bytes or 8 bytes depending on the system configuration. If we read a index file saved by
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

