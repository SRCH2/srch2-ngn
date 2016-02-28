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
package com.srch2.android.sdk;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class TestGeoIndex extends TestIndex{
    public static final int BATCH_INSERT_NUM = 200;
    public static final int BATCH_START_NUM= 0;


    public static final String INDEX_NAME = "testGeo";
    public static final String INDEX_FIELD_NAME_PRIMARY_KEY = "id";
    public static final String INDEX_FIELD_NAME_TITLE = "title";
    public static final String INDEX_FIELD_NAME_SCORE = "score";
    public static final String INDEX_FIELD_NAME_LATITUDE = "lat";
    public static final String INDEX_FIELD_NAME_LONGITUDE = "lon";




    @Override
    public String getIndexName() {
        return INDEX_NAME;
    }

    @Override
    public Schema getSchema() {
        PrimaryKeyField primaryKey = Field.createSearchablePrimaryKeyField(INDEX_FIELD_NAME_PRIMARY_KEY);
        Field title = Field.createSearchableField(INDEX_FIELD_NAME_TITLE);
        Field score = Field.createRefiningField(INDEX_FIELD_NAME_SCORE, Field.Type.INTEGER);
        return new Schema(primaryKey, INDEX_FIELD_NAME_LATITUDE, INDEX_FIELD_NAME_LONGITUDE, title, score);
    }


    /**
     * Returns a set of records with "id" field incremented by for loop and "title" set to "Title# + <loopIteration>".
     */
    public JSONArray getRecordsArray(int numberOfRecordsToInsert, int primaryKeyStartIndice) {
        JSONArray recordsArray = new JSONArray();
        for (int i = primaryKeyStartIndice; i < numberOfRecordsToInsert + primaryKeyStartIndice; ++i) {
            JSONObject recordObject = new JSONObject();
            try {
                recordObject.put(INDEX_FIELD_NAME_PRIMARY_KEY, String.valueOf(i));
                recordObject.put(INDEX_FIELD_NAME_TITLE, "Title ");
                recordObject.put(INDEX_FIELD_NAME_SCORE, i);
                recordObject.put(INDEX_FIELD_NAME_LATITUDE, i);
                recordObject.put(INDEX_FIELD_NAME_LONGITUDE, i);
            } catch (JSONException ignore) {
            }
            recordsArray.put(recordObject);
        }
        return recordsArray;
    }

    @Override
    public JSONObject getSucceedToInsertRecord()  {
        JSONObject obj = super.getSucceedToInsertRecord();
        try {
            obj.put(INDEX_FIELD_NAME_LATITUDE, 42.42);
            obj.put(INDEX_FIELD_NAME_LONGITUDE, 42.42);
            return obj;
        } catch (JSONException e) {
            e.printStackTrace();
            return null;
        }
    }

    @Override
    public JSONObject getFailToInsertRecord() {
       return getSucceedToInsertRecord();
    }


    @Override
    public List<Query> getSucceedToSearchQuery(JSONArray records) {
        if (records.length() == 1){
            if (singleRecordQueryQuery.isEmpty()) {
                super.getSucceedToSearchQuery(records);
                singleRecordQueryQuery.add(new Query(40, 40, 50, 50));
                singleRecordQueryQuery.add(new Query(42, 42, 10));
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosen")).insideRectangle(40, 40, 50, 50));
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosen")).insideCircle(40, 40, 10));
            }
            return new ArrayList<Query>(singleRecordQueryQuery);
        } else {
            return super.getSucceedToSearchQuery(records);
        }
    }

    @Override
    public List<Query> getFailToSearchQuery(JSONArray records) {
        if (records.length() == 1){
            List<Query> queries = super.getFailToSearchQuery(records);
            queries.add( new Query(45,45,60,60));
            queries.add( new Query(45,45,1));
            queries.add( new Query(new SearchableTerm("chosen")).insideRectangle(45, 45, 60, 60));
            queries.add( new Query(new SearchableTerm("chosen")).insideCircle(45, 45, 1));
            return queries;
        }
        return super.getFailToSearchQuery(records);
    }


    @Override
    public String getPrimaryKeyFieldName() {
        return this.INDEX_FIELD_NAME_PRIMARY_KEY;
    }

    @Override
    public JSONArray getSucceedToInsertBatchRecords() {
        return this.getRecordsArray(BATCH_INSERT_NUM, BATCH_START_NUM);
    }


    @Override
    public JSONObject getSucceedToUpdateExistRecord() {

        JSONObject record = super.getSucceedToUpdateExistRecord();
        try {
            record.put(INDEX_FIELD_NAME_LATITUDE, 42);
            record.put(INDEX_FIELD_NAME_LONGITUDE, 42);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return record;
    }

    @Override
    public JSONObject getSucceedToUpsertRecord() {
        JSONObject record = super.getSucceedToUpsertRecord();
         try {
            record.put(INDEX_FIELD_NAME_LATITUDE, 42);
            record.put(INDEX_FIELD_NAME_LONGITUDE, 42);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return record;
    }

    @Override
    public JSONArray getFailToInsertBatchRecord() {
        return this.getRecordsArray(BATCH_INSERT_NUM, BATCH_START_NUM);
    }

    @Override
    public JSONArray getSucceedToUpdateBatchRecords() {
        return this.getRecordsArray(BATCH_INSERT_NUM, BATCH_START_NUM);
    }
}
