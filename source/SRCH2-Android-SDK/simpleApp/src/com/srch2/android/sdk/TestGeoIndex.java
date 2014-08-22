package com.srch2.android.sdk;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

public class TestGeoIndex extends TestIndex{
    public static final int BATCH_INSERT_NUM = 200;


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
        PrimaryKeyField primaryKey = Field.createDefaultPrimaryKeyField(INDEX_FIELD_NAME_PRIMARY_KEY);
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
                recordObject.put(INDEX_FIELD_NAME_LATITUDE, (double)i);
                recordObject.put(INDEX_FIELD_NAME_LONGITUDE, (double)i);
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
            if (singleRecordQueryQuery == null) {
                super.getSucceedToSearchQuery(records);
                singleRecordQueryQuery.add(new Query(40, 40, 50, 50));
                singleRecordQueryQuery.add(new Query(40, 40, 10));
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosen")).insideBoxRegion(40, 40, 50, 50));
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosen")).insideCircleRegion(40, 40, 10));
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
            queries.add( new Query(new SearchableTerm("chosen")).insideBoxRegion(45,45,60,60));
            queries.add( new Query(new SearchableTerm("chosen")).insideCircleRegion(45,45,1));
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

}
