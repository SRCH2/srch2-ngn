package com.srch2.android.http.service;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by jianfeng on 8/4/14.
 */
public class TestGeoIndex extends TestableIndex {
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
        return new Schema(primaryKey, title, score);
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
//                recordObject.put(INDEX_FIELD_NAME_LATITUDE, TestCaseUtil.generateRandomGeo());
//                recordObject.put(INDEX_FIELD_NAME_LONGITUDE, TestCaseUtil.generateRandomGeo());
            } catch (JSONException ignore) {
            }
            recordsArray.put(recordObject);
        }
        return recordsArray;
    }

    @Override
    public JSONObject getSucceedToInsertRecord() {
        return null;
    }

    @Override
    public JSONObject getFailToInsertRecord() {
        return null;
    }

    @Override
    public List<String> getSucceedToSearchString(JSONArray records) {
        return null;
    }

    @Override
    public List<String> getFailToSearchString(JSONArray records) {
        return null;
    }

    @Override
    public List<Query> getSucceedToSearchQuery(JSONArray records) {
        return null;
    }

    @Override
    public List<Query> getFailToSearchQuery(JSONArray records) {
        return null;
    }


    @Override
    public JSONObject getFailToUpdateRecord() {
        return null;
    }

    @Override
    public String getPrimaryKeyFieldName() {
        return null;
    }

    @Override
    public List<String> getFailToDeleteRecord() {
        return null;
    }

    @Override
    public JSONArray getSucceedToInsertBatchRecords() {
        return null;
    }

    @Override
    public JSONArray getFailToInsertBatchRecord() {
        return null;
    }

    @Override
    public JSONArray getSucceedToUpdateBatchRecords() {
        return null;
    }

    @Override
    public JSONArray getFailToUpdateBatchRecords() {
        return null;
    }

    @Override
    public boolean verifyResult(String query, ArrayList<JSONObject> jsonObjects) {
        return false;
    }

    @Override
    public boolean verifyResult(Query query, ArrayList<JSONObject> jsonObjects) {
        return false;
    }

    @Override
    public JSONObject getSucceedToUpdateExistRecord() {
        return null;
    }

    @Override
    public JSONObject getSucceedToUpsertRecord() {
        return null;
    }


}
