package com.srch2.android.http.service;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.*;

/**
 * Created by ashton on 7/29/2014.
 */
public class TestIndex extends TestableIndex {

    public static final String INDEX_NAME = "test";
    public static final String INDEX_FIELD_NAME_PRIMARY_KEY = "id";
    public static final String INDEX_FIELD_NAME_TITLE = "title";
    public static final String INDEX_FIELD_NAME_SCORE = "score";

    protected static final String ONE_RECORD_PRIMARY_KEY = "the chosen # one";
    protected static final int BATCH_INSERT_NUM = 200;
    protected static final int BATCH_START_NUM = 0;

    HashSet<String> singleRecordQueryString;
    HashSet<String> multipleRecordQueryString;

    HashSet<Query> singleRecordQueryQuery;
    HashSet<Query> multipleRecordQueryQuery;
    private Query queryMultipleSortAllAsc;
    private Query queryMultipleSortAllDcs;
    private Query queryMultipleFilter;

    /**
     * Returns a core named "test" with fields "id" as primary key and "title" as searchable text title.
     */
    @Override
    public IndexDescription getIndexDescription() {
        Field primaryKey = Field.createSearchableField(INDEX_FIELD_NAME_PRIMARY_KEY);
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

            testRecord.put(INDEX_FIELD_NAME_TITLE, ONE_RECORD_PRIMARY_KEY + " extra ");
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
                // search term
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosen").disableFuzzyMatching()).pagingSize(BATCH_INSERT_NUM));
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosen").AND(new SearchableTerm("one"))).pagingSize(BATCH_INSERT_NUM));
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosen").OR(new SearchableTerm("two"))).pagingSize(BATCH_INSERT_NUM));

                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosen").AND_NOT(new SearchableTerm("two"))).pagingSize(BATCH_INSERT_NUM));

                singleRecordQueryQuery.add(new Query(new SearchableTerm("chose").setIsPrefixMatching(true)).pagingSize(BATCH_INSERT_NUM));
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosen").searchSpecificField(INDEX_FIELD_NAME_TITLE)).pagingSize(BATCH_INSERT_NUM));
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosen").searchSpecificField(INDEX_FIELD_NAME_TITLE)
                            .AND(new SearchableTerm("extra").searchSpecificField(INDEX_FIELD_NAME_TITLE))
                        ).pagingSize(BATCH_INSERT_NUM));
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosem").enableFuzzyMatching(0.7f)));
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosem").enableFuzzyMatching(0.7f).setBoostValue(2)));
                // queries
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosem").enableFuzzyMatching(0.7f)).filterByFieldEndsTo(INDEX_FIELD_NAME_SCORE,"42"));
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosem").enableFuzzyMatching(0.7f)).filterByFieldStartsFrom(INDEX_FIELD_NAME_SCORE, "42"));
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosem").enableFuzzyMatching(0.7f)).filterByFieldEqualsTo(INDEX_FIELD_NAME_SCORE, "42"));
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosem").enableFuzzyMatching(0.7f)).filterByFieldInRange(INDEX_FIELD_NAME_SCORE, "42", "42"));


                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosem").enableFuzzyMatching(0.7f))
                        .filterByFieldEndsTo(INDEX_FIELD_NAME_SCORE, "42")
                        .filterByFieldEqualsTo(INDEX_FIELD_NAME_SCORE, "42")
                        .setFilterRelationAND());
                singleRecordQueryQuery.add(new Query(new SearchableTerm("chosem").enableFuzzyMatching(0.7f))
                        .filterByFieldEndsTo(INDEX_FIELD_NAME_SCORE, "42")
                        .filterByFieldEqualsTo(INDEX_FIELD_NAME_SCORE, "43")
                        .setFilterRelationOR());
            }
            return new ArrayList<Query>(singleRecordQueryQuery);
        } else {
            if (multipleRecordQueryQuery == null) {
                multipleRecordQueryQuery = new HashSet<Query>();
                queryMultipleSortAllAsc= new Query(new SearchableTerm("title").disableFuzzyMatching())
                        .sortOnFields(INDEX_FIELD_NAME_SCORE).orderByAscending().pagingSize(BATCH_INSERT_NUM);
                multipleRecordQueryQuery.add(queryMultipleSortAllAsc);
                // sorting and filtering queries
                queryMultipleSortAllDcs = new Query(new SearchableTerm("title").disableFuzzyMatching())
                        .sortOnFields(INDEX_FIELD_NAME_SCORE).orderByDescending().pagingSize(BATCH_INSERT_NUM);
                multipleRecordQueryQuery.add(queryMultipleSortAllDcs);

                queryMultipleFilter = new Query(new SearchableTerm("title").disableFuzzyMatching())
                        .filterByFieldInRange(INDEX_FIELD_NAME_SCORE, "13", "20").sortOnFields(INDEX_FIELD_NAME_SCORE).orderByAscending();

                multipleRecordQueryQuery.add(queryMultipleFilter);
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
                            // search term
            queries.add(new Query(new SearchableTerm("chosenn").disableFuzzyMatching()).pagingSize(BATCH_INSERT_NUM));
            queries.add(new Query(new SearchableTerm("chosen").AND(new SearchableTerm("two"))).pagingSize(BATCH_INSERT_NUM));
            queries.add(new Query(new SearchableTerm("chosenn").OR(new SearchableTerm("oone"))).pagingSize(BATCH_INSERT_NUM));
            queries.add(new Query(new SearchableTerm("chosen").AND_NOT(new SearchableTerm("one"))).pagingSize(BATCH_INSERT_NUM));
            queries.add(new Query(new SearchableTerm("chose").setIsPrefixMatching(false)).pagingSize(BATCH_INSERT_NUM));
            queries.add(new Query(new SearchableTerm("extra").searchSpecificField(INDEX_FIELD_NAME_PRIMARY_KEY)).pagingSize(BATCH_INSERT_NUM));
            queries.add(new Query(new SearchableTerm("chosem").enableFuzzyMatching(1f)));
            queries.add(new Query(new SearchableTerm("chosem").enableFuzzyMatching(1f).setBoostValue(2)));
            // queries
            queries.add(new Query(new SearchableTerm("chosem").enableFuzzyMatching(0.7f)).filterByFieldEndsTo(INDEX_FIELD_NAME_SCORE,"41"));
            queries.add(new Query(new SearchableTerm("chosem").enableFuzzyMatching(0.7f)).filterByFieldStartsFrom(INDEX_FIELD_NAME_SCORE, "43"));
            queries.add(new Query(new SearchableTerm("chosem").enableFuzzyMatching(0.7f)).filterByFieldEqualsTo(INDEX_FIELD_NAME_SCORE, "45"));
            queries.add(new Query(new SearchableTerm("chosem").enableFuzzyMatching(0.7f)).filterByFieldInRange(INDEX_FIELD_NAME_SCORE, "0", "2"));

            queries.add(new Query(new SearchableTerm("chosem").enableFuzzyMatching(0.7f))
                    .filterByFieldEndsTo(INDEX_FIELD_NAME_SCORE, "42")
                    .filterByFieldEqualsTo(INDEX_FIELD_NAME_SCORE, "41")
                    .filterByFieldStartsFrom(INDEX_FIELD_NAME_SCORE, "41")
                    .setFilterRelationAND());
            queries.add(new Query(new SearchableTerm("chosem").enableFuzzyMatching(0.7f))
                    .filterByFieldEndsTo(INDEX_FIELD_NAME_SCORE, "41")
                    .filterByFieldEqualsTo(INDEX_FIELD_NAME_SCORE, "43")
                    .filterByFieldStartsFrom(INDEX_FIELD_NAME_SCORE, "43")
                    .setFilterRelationOR());

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

    boolean checkAllSortedRecord(ArrayList<JSONObject> jsonObjects, boolean getAll, int start, int end) {
        try {
            if (jsonObjects.size() == end-start || !getAll) {
                for (int i = start; i < end && i < jsonObjects.size(); ++i) {
                    if (jsonObjects.get(i-start).getInt(INDEX_FIELD_NAME_PRIMARY_KEY) != i) {
                        return false;
                    }
                }
                return true;
            }
            return false;
        } catch (JSONException e) {
            e.printStackTrace();
            return false;
        }
    }

    boolean checkPartialResult(Query query, ArrayList<JSONObject> jsonObjects) {
        try{
            if (query == queryMultipleSortAllAsc){
                return checkAllSortedRecord(jsonObjects, true, 0, BATCH_INSERT_NUM);
            }else if (query == queryMultipleSortAllDcs){
                Collections.reverse(jsonObjects);
                return checkAllSortedRecord(jsonObjects, true, 0, BATCH_INSERT_NUM);
            } else if (query == queryMultipleFilter){
                // depend on how queryMultipleFilter get constructed
                return checkAllSortedRecord(jsonObjects, true, 13, 20+1);
            }
            return true;
        } catch (Exception e){
            e.printStackTrace();
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
                // String search don't have so much control, just check the size
                return jsonObjects.size() == 10; // default row number
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
                return checkPartialResult(query, jsonObjects);
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
