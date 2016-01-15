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
 * mysqlEventHandler.h
 *
 *  Created on: Sep 17, 2014
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

/****************************MySQLTableIndex*******************************/
//Table index populates the table id and table name.
typedef std::map<uint64_t, mysql::Table_map_event *> Int2event_map;

class MySQLTableIndex: public mysql::Content_handler, public Int2event_map {
public:
    mysql::Binary_log_event *process_event(mysql::Table_map_event *tm);

    ~MySQLTableIndex();
};

/************************MySQLIncidentHandler******************************/
//This class handles all the incident events like LOST_EVENTS.
class MySQLIncidentHandler: public mysql::Content_handler {
public:
    MySQLIncidentHandler() :
            mysql::Content_handler() {
    }

    mysql::Binary_log_event * process_event(mysql::Incident_event * incident);
};

/*****************************Applier**********************************/
//This class handles insert, delete, update events.
class MySQLApplier: public mysql::Content_handler {
public:
    MySQLApplier(MySQLTableIndex * index, ServerInterface * serverInterface,
            std::vector<std::string> * schemaName, time_t & startTimestamp,
            std::string & pk);
    mysql::Binary_log_event * process_event(mysql::Row_event * rev);
private:
    MySQLTableIndex * tableIndex;
    std::vector<std::string> * schemaName;
    ServerInterface * serverInterface;
    time_t startTimestamp;
    std::string pk;

    void tableInsert(std::string & table_name, mysql::Row_of_fields &fields);
    void tableDelete(std::string & table_name, mysql::Row_of_fields &fields);
    void tableUpdate(std::string & table_name, mysql::Row_of_fields &old_fields,
            mysql::Row_of_fields &new_fields);
};

#endif /* __MYSQLEVENTHANDLER_H__ */
