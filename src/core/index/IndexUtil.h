/*
 */

#pragma once
#ifndef __INDEX_H__
#define __INDEX_H__

namespace srch2
{
namespace instantsearch
{

typedef enum
{
    INDEX_LOAD = 0, // load an existing index
    INDEX_BUILD = 1, // build an index from a file
    INDEX_UPDATE = 2 // update an existing index
} IndexOpenMode;

class IndexConfig
{
public:
    static const char* const trieFileName;
    static const char* const invertedIndexFileName;
    static const char* const forwardIndexFileName;
    static const char* const recordIdConverterFileName;
    static const char* const permissionMapFileName;
    static const char* const schemaFileName;
    static const char* const analyzerFileName;
    static const char* const indexCountsFileName;
    static const char* const AccessControlFile;

    static const char* const quadTreeFileName;

    static const char* const normalizerFileName;
    static const char* const stemmerFileName;

    static const char* const queryTrieFileName;
    static const char* const queryFeedbackFileName;
};
}}

#endif //__INDEX_H__
