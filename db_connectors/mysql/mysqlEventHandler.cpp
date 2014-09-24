/*
 * mysqlEventHandler.cpp
 *
 *  Created on: Sep 17, 2014
 *      Author: Chen Liu liu@srch2.com
 */

#include "mysqlEventHandler.h"
#include "json/json.h"
#include "Logger.h"

using srch2::util::Logger;

/****************************Table_index*******************************/
//Table index populates the table id and table name.
mysql::Binary_log_event *Table_index::process_event(
        mysql::Table_map_event *tm) {
    if (find(tm->table_id) == end())
        insert(std::pair<uint64_t, mysql::Table_map_event *>(tm->table_id, tm));

    /* Consume this event so it won't be deallocated beneith our feet */
    return 0;
}

Table_index::~Table_index() {
    Int2event_map::iterator it = begin();
    do {
        delete it->second;
    } while (++it != end());
}

int Table_index::get_table_name(int table_id, std::string out) {
    iterator it;
    if ((it = find(table_id)) == end()) {
        std::stringstream os;
        os << "unknown_table_" << table_id;
        out.append(os.str());
        return 1;
    }

    out.append(it->second->table_name);
    return 0;
}

/************************Incident_handler******************************/
//This class handles all the incident events like LOST_EVENTS.
mysql::Binary_log_event * Incident_handler::process_event(
        mysql::Incident_event * incident) {
    Logger::debug("MYSQLCONNECTOR: Incident event type: %s\n length: %d,"
            " next pos: %d\n type= %u, message= %s",
            mysql::system::get_event_type_str(incident->get_event_type()),
            incident->header()->event_length, incident->header()->next_position,
            (unsigned) incident->type, incident->message.c_str());
    /* Consume the event */
    delete incident;
    return 0;
}

/*****************************Applier**********************************/
//This class handles insert, delete, update events.
Applier::Applier(Table_index * index, ServerInterface * serverHandle,
        std::vector<std::string> * schemaName, time_t & startTimestamp,
        std::string & pk) {
    m_table_index = index;
    this->serverHandle = serverHandle;
    this->schemaName = schemaName;
    this->startTimestamp = startTimestamp;
    this->pk = pk;
}

mysql::Binary_log_event * Applier::process_event(mysql::Row_event * rev) {
    //Ignore the old event
    time_t ts = rev->header()->timestamp;

    if (ts < startTimestamp) {
        return rev;
    }

    startTimestamp = ts;    //Update the time stamp

    //Get the table id
    uint64_t table_id = rev->table_id;
    Int2event_map::iterator ti_it = m_table_index->find(table_id);
    if (ti_it == m_table_index->end()) {
        Logger::error("MYSQLCONNECTOR: Table id %d was not registered"
                " by any preceding table map event.");
        return rev;
    }
    /*
     * Each row event contains multiple rows and fields. The Row_iterator
     * allows us to iterate one row at a time.
     */
    mysql::Row_event_set rows(rev, ti_it->second);

    //Create a fully qualified table name
    std::ostringstream os;
    os << ti_it->second->db_name << '.' << ti_it->second->table_name;

    try {
        mysql::Row_event_set::iterator it = rows.begin();
        do {
            mysql::Row_of_fields fields = *it;
            if (rev->get_event_type() == mysql::WRITE_ROWS_EVENT
                    || rev->get_event_type() == mysql::WRITE_ROWS_EVENT_V1) {
                table_insert(os.str(), fields);
            }

            if (rev->get_event_type() == mysql::UPDATE_ROWS_EVENT
                    || rev->get_event_type() == mysql::UPDATE_ROWS_EVENT_V1) {
                ++it;
                mysql::Row_of_fields fields2 = *it;
                table_update(os.str(), fields, fields2);
            }
            if (rev->get_event_type() == mysql::DELETE_ROWS_EVENT
                    || rev->get_event_type() == mysql::DELETE_ROWS_EVENT_V1) {
                table_delete(os.str(), fields);
            }

        } while (++it != rows.end());
    } catch (const std::logic_error& le) {
        Logger::error("MYSQLCONNECTOR: MySQL data type error : %s",le.what());
    }

    return rev;
}

void Applier::table_insert(std::string table_name,
        mysql::Row_of_fields &fields) {
    mysql::Row_of_fields::iterator field_it = fields.begin();
    std::vector<std::string>::iterator schema_it = schemaName->begin();
    mysql::Converter converter;

    Json::Value record;
    Json::FastWriter writer;

    do {
        /*
         Each row contains a vector of Value objects. The converter
         allows us to transform the value into another
         representation.
         */
        std::string str;
        converter.to(str, *field_it);

        record[*schema_it] = str;

        field_it++;
        schema_it++;
    } while (field_it != fields.end() && schema_it != schemaName->end());
    std::string jsonString = writer.write(record);
    Logger::info("MYSQLCONNECTOR: Inserting %s ", jsonString.c_str());

    serverHandle->insertRecord(jsonString);
}

void Applier::table_delete(std::string table_name,
        mysql::Row_of_fields &fields) {
    mysql::Row_of_fields::iterator field_it = fields.begin();
    std::vector<std::string>::iterator schema_it = schemaName->begin();
    mysql::Converter converter;
    do {
        /*
         Each row contains a vector of Value objects. The converter
         allows us to transform the value into another
         representation.
         */

        std::string str;
        converter.to(str, *field_it);
        if ((*schema_it).compare(this->pk.c_str()) == 0) {
            Logger::info("MYSQLCONNECTOR: Deleting primary key %s = %s ",
                    this->pk.c_str(), str.c_str());
            serverHandle->deleteRecord(str);
            break;
        }
        field_it++;
        schema_it++;
    } while (field_it != fields.end() && schema_it != schemaName->end());
    if (field_it == fields.end() || schema_it == schemaName->end()) {
        Logger::error("MYSQLCONNECTOR: Schema doesn't contain primary key.");
    }
}

void Applier::table_update(std::string table_name,
        mysql::Row_of_fields &old_fields, mysql::Row_of_fields &new_fields) {
    /*
     Find previous entry and delete it.
     */
    table_delete(table_name, old_fields);

    /*
     Insert new entry.
     */
    table_insert(table_name, new_fields);

}

