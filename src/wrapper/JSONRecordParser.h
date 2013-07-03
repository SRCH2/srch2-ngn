//$Id: JSONRecordParser.h 3429 2013-06-10 09:13:54Z jiaying $

#ifndef _DAEMONDATASOURCE_H_
#define _DAEMONDATASOURCE_H_

#include "json/json.h"
#include <instantsearch/Schema.h>
#include <instantsearch/Analyzer.h>
#include <instantsearch/Record.h>
#include <instantsearch/Indexer.h>

#include "BimapleServerConf.h"
#include "BimapleServerLogger.h"

namespace bmis = bimaple::instantsearch;

namespace bimaple
{
namespace httpwrapper
{


class JSONRecordParser
{
 public:
  static bool populateRecordFromJSON( const std::string &inputLine, const BimapleServerConf *indexDataContainerConf,
				      bmis::Record *record, std::stringstream &error);
  static bool _JSONValueObjectToRecord(bmis::Record *record, const std::string &inputLine, const Json::Value &root,
				       const BimapleServerConf *indexDataContainerConf, std::stringstream &error);
  static bmis::Schema* createAndPopulateSchema( const BimapleServerConf *indexDataContainerConf);

 private:
  static void getJsonValueString(const Json::Value &jsonValue, const std::string &key, std::string &stringValue, const string &configName);
  static void getJsonValueDouble(const Json::Value &jsonValue, const std::string &key, double &doubleValue, const string& configName);
};

class DaemonDataSource
{
	public:
		static void createNewIndexFromFile(bmis::Indexer *indexer, const BimapleServerConf *indexDataContainerConf, const BimapleServerLogger *bimapleServerLogger);
};


}}

#endif // _DAEMONDATASOURCE_H_
