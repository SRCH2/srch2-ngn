/*
 * mysqlEventHandler.h
 *
 *  Created on: Sep 17, 2014
 *      Author: Chen Liu liu@srch2.com
 */

#ifndef __MYSQLEVENTHANDLER_H__
#define __MYSQLEVENTHANDLER_H__

#include <stdlib.h>
#include <map>
#include "DataConnector.h"
#include "binlog_api.h"
#include "binlog_event.h"
#include "basic_content_handler.h"

using mysql::Binary_log;
using mysql::system::create_transport;

/****************************Table_index*******************************/
//Table index populates the table id and table name.
typedef std::map<uint64_t, mysql::Table_map_event *> Int2event_map;

class Table_index: public mysql::Content_handler, public Int2event_map {
public:
    mysql::Binary_log_event *process_event(mysql::Table_map_event *tm);

    ~Table_index();

    int get_table_name(int table_id, std::string out);

};

/************************Incident_handler******************************/
//This class handle all the incident events like LOST_EVENTS.
class Incident_handler: public mysql::Content_handler {
public:
    Incident_handler() :
            mysql::Content_handler() {
    }

    mysql::Binary_log_event * process_event(mysql::Incident_event * incident);
};

/*****************************Applier**********************************/
class Applier: public mysql::Content_handler {
public:
    Applier(Table_index * index, ServerInterface * serverHandle,
            std::vector<std::string> * schemaName, time_t & startTimestamp,
            std::string & pk);
    mysql::Binary_log_event * process_event(mysql::Row_event * rev);
private:
    Table_index * m_table_index;
    std::vector<std::string> * schemaName;
    ServerInterface * serverHandle;
    time_t startTimestamp;
    std::string pk;

    void table_insert(std::string table_name, mysql::Row_of_fields &fields);
    void table_delete(std::string table_name, mysql::Row_of_fields &fields);
    void table_update(std::string table_name, mysql::Row_of_fields &old_fields,
            mysql::Row_of_fields &new_fields);
};

#endif /* __MYSQLEVENTHANDLER_H__ */
