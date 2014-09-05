/*
 * Serializer.h
 *
 *  Created on: Oct 9, 2013
 *      Author: srch2
 */

#ifndef __CORE_SERIALIZATION_SERIALIZER_H__
#define __CORE_SERIALIZATION_SERIALIZER_H__

#include <boost/version.hpp>
#include <string>
#include <stdint.h>
#include <fstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "util/Version.h"
#include "util/Logger.h"
using namespace std;
using namespace srch2::util;

namespace srch2 {
namespace instantsearch {

class ForwardIndex;
class InvertedIndex;
class Trie;
class SchemaInternal;
class QuadTree;

/*
 *   Index version layout
 *   ------------------------------------------------------------------------------------------
 *   | 16 bits counter | 32bits Boost version | 8 bits endianess (0|1) | 8 bits size of pointer|
 *   ------------------------------------------------------------------------------------------
 *
 *   We do not store engine's version because index version may not change in each new release.
 */
class IndexVersion {
public:
    IndexVersion(uint8_t internalVersion, unsigned boostVersion);
    IndexVersion() {
        this->sequentialId = 0;
        this->boostVersion = 0;
        this->endianness = 0;
        this->bitness = 0;
    }
    bool operator ==(const IndexVersion& in) {
        return (this->sequentialId == in.sequentialId
                && this->boostVersion == in.boostVersion
                && this->endianness == in.endianness
                && this->bitness == in.bitness);
    }
    template<class Archive>
    void serialize(Archive &ar, const unsigned int file_version) {
        ar & sequentialId;
        ar & boostVersion;
        ar & endianness;
        ar & bitness;
    }
    /*
     * As of now all indexes share same version because we do not support partially rebuilding of
     * indexes. If we support it in future then try creating separate version info for each indexes.
     */
    static IndexVersion currentVersion;
private:
    short sequentialId;
    unsigned boostVersion;
    uint8_t endianness;  // 0 for big endian or 1 for small endian
    uint8_t bitness;     // this value holds size of pointer i.e 4/8 bytes.
};

/*
 *  Common serialization class for all the "serializable" object in the engine, It checks the
 *  serialized version at the beginning of each index files  and compare with current engine's
 *  Index version. if the file's version is incompatible with the current version then throw an
 *  exception.
 *  Note: Use template specialization to write your specific implementation if required.
 */
class Serializer {
    bool isCompatibleVersion(IndexVersion& storedIndexVersion) {
        return IndexVersion::currentVersion == storedIndexVersion;
    }
public:
    template<class T>
    void load(T& dataObject, const string& serializedFileName) {
        std::ifstream ifs(serializedFileName.c_str(), std::ios::binary);
        boost::archive::binary_iarchive ia(ifs);
        IndexVersion storedIndexVersion;
        ia >> storedIndexVersion;
        try {
            if (isCompatibleVersion(storedIndexVersion)) {
                ia >> dataObject;
            } else {
                // throw invalid index file exception
                throw exception();
            }
        } catch (exception& ex) {
            ifs.close();
            Logger::error(
                    "Invalid index file. Either index files are built with a previous version"
                            " of the engine or copied from a different machine/architecture.");
            throw ex;
        }
        ifs.close();
    }
    template<class T>
    void save(const T& dataObject, const string& serializedFileName) {
        std::ofstream ofs(serializedFileName.c_str(), std::ios::binary);
        if (!ofs.good())
            throw std::runtime_error("Error opening " + serializedFileName);
        boost::archive::binary_oarchive oa(ofs);
        oa << IndexVersion::currentVersion;
        oa << dataObject;
        ofs.close();
    }
    Serializer();
    ~Serializer();
};

}
}
#endif /* __CORE__SERIALIZATION__SERIALIZER_H__ */
