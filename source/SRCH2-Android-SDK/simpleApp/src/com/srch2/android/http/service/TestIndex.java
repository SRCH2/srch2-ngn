package com.srch2.android.http.service;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

/**
 * Created by ashton on 7/29/2014.
 */
public class TestIndex extends TestableIndex {

    public static final String INDEX_NAME = "test";
    public static final String INDEX_FIELD_NAME_PRIMARY_KEY = "id";
    public static final String INDEX_FIELD_NAME_TITLE = "title";
    public static final String INDEX_FIELD_NAME_SCORE = "score";

    protected static final String ONE_RECORD_PRIMARY_KEY = "the chosen one";
    protected static final int BATCH_INSERT_NUM = 200;
    protected static final int BATCH_START_NUM = 0;

    HashSet<String> singleRecordQueryString;
    HashSet<String> multipleRecordQueryString;

    HashSet<Query> singleRecordQueryQuery;
    HashSet<Query> multipleRecordQueryQuery;

    /**
     * Returns a core named "test" with fields "id" as primary key and "title" as searchable text title.
     */
    @Override
    public IndexDescription getIndexDescription() {
        Field primaryKey = Field.createRefiningField(INDEX_FIELD_NAME_PRIMARY_KEY, Field.Type.TEXT);
        Field title = Field.createSearchableField(INDEX_FIELD_NAME_TITLE);
        Field score = Field.createRefiningField(INDEX_FIELD_NAME_SCORE, Field.Type.INTEGER);

        return new IndexDescription(INDEX_NAME, primaryKey, title, score);
    }

    /**
     * Returns a set of records with "id" field incremented by for loop and "title" set to "Title# + <loopIteration>".
     */
    public JSONArray getRecordsArray(int numberOfRecordsToInsert, int primaryKeyStartIndices) {
        JSONArray recordsArray = new JSONArray();
        for (int i = primaryKeyStartIndices; i < numberOfRecordsToInsert + primaryKeyStartIndices; ++i) {
            JSONObject recordObject = new JSONObject();
            try {
                recordObject.put(INDEX_FIELD_NAME_PRIMARY_KEY, String.valueOf(i));
                recordObject.put(INDEX_FIELD_NAME_TITLE, "Title ");
                recordObject.put(INDEX_FIELD_NAME_SCORE, i);
            } catch (JSONException ignore) {
            }
            recordsArray.put(recordObject);
        }
        return recordsArray;
    }

    @Override
    public JSONObject getSucceedToInsertRecord() {
        JSONObject testRecord = new JSONObject();
        try {
            testRecord.put(INDEX_FIELD_NAME_PRIMARY_KEY, ONE_RECORD_PRIMARY_KEY);

            testRecord.put(INDEX_FIELD_NAME_TITLE, ONE_RECORD_PRIMARY_KEY);
            testRecord.put(INDEX_FIELD_NAME_SCORE, "42");
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return testRecord;
    }

    @Override
    public JSONObject getFailToInsertRecord() {
        JSONObject testRecord = new JSONObject();
        try {
            // repeat insert the existing value should fail
            testRecord.put(INDEX_FIELD_NAME_PRIMARY_KEY, ONE_RECORD_PRIMARY_KEY);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return testRecord;
    }

    @Override
    public ArrayList<String> getSucceedToSearchString(JSONArray records) {

        // to make inteface simple, if the length is 1, we are supposed to verify the record from getSucceedToInsertRecord();
        if (records.length() == 1) {
            if (singleRecordQueryString == null) {
                singleRecordQueryString = new HashSet<String>();
                singleRecordQueryString.add("chosen");
                singleRecordQueryString.add("one");
                singleRecordQueryString.add("chos");
                singleRecordQueryString.add("chase");
                singleRecordQueryString.add("chasen on");
            }
            return new ArrayList<String>(singleRecordQueryString);
        } else { // else the queries should make up to operate the batch searching.
            if (multipleRecordQueryString == null) {
                multipleRecordQueryString = new HashSet<String>();
                multipleRecordQueryString.add("title");
                multipleRecordQueryString.add("titl");
                multipleRecordQueryString.add("totle");
            }
            return new ArrayList<String>(multipleRecordQueryString);
        }
    }

    @Override
    public List<String> getFailToSearchString(JSONArray records) {
        ArrayList<String> queries = new ArrayList<String>();
        queries.add("the"); // stopwords
        queries.add("nothing");
        queries.add("xxxxxxxxxxx");
        return queries;
    }

    @Override
    public List<Query> getSucceedToSearchQuery(JSONArray records) {
        if (records.length() == 1) {
            if (singleRecordQueryQuery == null) {
                singleRecordQueryQuery = new HashSet<Query>();
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosen").disableFuzzyMatching()).pagingSize(BATCH_INSERT_NUM));
                //TODO add all the advanced Query here
            }
            return new ArrayList<Query>(singleRecordQueryQuery);
        } else {
            if (multipleRecordQueryQuery == null) {
                multipleRecordQueryQuery = new HashSet<Query>();
                multipleRecordQueryQuery.add(new Query(new SearchableTerm("title").disableFuzzyMatching()).pagingSize(BATCH_INSERT_NUM));
                //TODO ditto
            }
            return new ArrayList<Query>(multipleRecordQueryQuery);
        }
    }

    @Override
    public List<Query> getFailToSearchQuery(JSONArray records) {
        ArrayList<Query> queries = new ArrayList<Query>();
        if (records.length() == 1) {
            //TODO add all the advanced Query here
            queries.add(new Query(new SearchableTerm("chason")));
        } else {
            queries.add(new Query(new SearchableTerm("titl").disableFuzzyMatching()));
            //TODO ditto
        }
        return queries;
    }


    @Override
    public JSONObject getFailToUpdateRecord() {

        try {
            return new JSONObject("{\"" + INDEX_FIELD_NAME_PRIMARY_KEY + "invalidschema\"" + ":\"invalid Schema\"}");
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return null;
    }

    @Override
    public String getPrimaryKeyFieldName() {
        return INDEX_FIELD_NAME_PRIMARY_KEY;
    }

    @Override
    public List<String> getFailToDeleteRecord() {
        ArrayList<String> tobeDelete = new ArrayList<String>();
        try {
            String id = getSucceedToInsertRecord().getString(INDEX_FIELD_NAME_PRIMARY_KEY) + "nullExistKey";
            tobeDelete.add(id);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return tobeDelete;
    }

    @Override
    public JSONArray getSucceedToInsertBatchRecords() {
        return getRecordsArray(BATCH_INSERT_NUM, BATCH_START_NUM);
    }

    @Override
    public JSONArray getFailToInsertBatchRecord() {
        return getRecordsArray(BATCH_INSERT_NUM, BATCH_START_NUM);
    }

    @Override
    public JSONArray getSucceedToUpdateBatchRecords() {
        return getRecordsArray(BATCH_INSERT_NUM, BATCH_START_NUM);
    }

    @Override
    public JSONArray getFailToUpdateBatchRecords() {
        JSONArray array = new JSONArray();
        try {
            for (int i = 0; i < 10; ++i) {
                array.put(new JSONObject("{\"" + INDEX_FIELD_NAME_PRIMARY_KEY + String.valueOf(i) + "invalidschema\"" + ":\"invalid Schema\""));
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return array;
    }

    boolean checkEveryRecords(ArrayList<JSONObject> jsonObjects, boolean getAll) {
        try {
            if (jsonObjects.size() == BATCH_INSERT_NUM || !getAll) {
                for (int i = 0; i < BATCH_INSERT_NUM && i < jsonObjects.size(); ++i) {
                    if (jsonObjects.get(i).getInt(INDEX_FIELD_NAME_PRIMARY_KEY) != i) {
                        return false;
                    }
                }
                return true;
            }
            return false;
        } catch (JSONException e) {
            return false;
        }
    }

    @Override
    public boolean verifyResult(String query, ArrayList<JSONObject> jsonObjects) {
        try {
            // simple way to detect if the index is the single record one or not
            if (singleRecordQueryString.contains(query)) {
                return jsonObjects.size() == 1 && jsonObjects.get(0).getString(INDEX_FIELD_NAME_PRIMARY_KEY).equals(ONE_RECORD_PRIMARY_KEY);
            } else if (multipleRecordQueryString.contains(query)) {
                return checkEveryRecords(jsonObjects, false);
            } else {
                throw new IllegalArgumentException("not supported yet");
            }
        } catch (JSONException e) {
            e.printStackTrace();
            return false;
        }
    }

    @Override
    public boolean verifyResult(Query query, ArrayList<JSONObject> jsonObjects) {
        try {
            if (singleRecordQueryQuery.contains(query)) {
                return jsonObjects.size() == 1 && jsonObjects.get(0).getString(INDEX_FIELD_NAME_PRIMARY_KEY).equals(ONE_RECORD_PRIMARY_KEY);
            } else if (multipleRecordQueryQuery.contains(query)) {
                return checkEveryRecords(jsonObjects, true);
            } else {
                throw new IllegalArgumentException("not supported yet");
            }
        } catch (JSONException e) {
            e.printStackTrace();
            return false;
        }
    }

    @Override
    public JSONObject getSucceedToUpdateExistRecord() {

        JSONObject record = null;
        try {
            record = new JSONObject(getSucceedToInsertRecord(), new String[]{INDEX_FIELD_NAME_PRIMARY_KEY});
            record.put(INDEX_FIELD_NAME_SCORE, 99);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return record;
    }

    @Override
    public JSONObject getSucceedToUpsertRecord() {
        JSONObject record = null;
        try {
            record = new JSONObject();
            record.put(INDEX_FIELD_NAME_PRIMARY_KEY, getSucceedToInsertRecord().getString(INDEX_FIELD_NAME_PRIMARY_KEY) + "differentKey");
            record.put(INDEX_FIELD_NAME_SCORE, 99);
        } catch (JSONException e) {
            e.printStackTrace();
        }
        return record;
    }
}
