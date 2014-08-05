package com.srch2.android.http.service;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Created by ashton on 7/29/2014.
 */
public class TestIndex extends Indexable {

    public static final String INDEX_NAME = "test";
    public static final String INDEX_FIELD_NAME_PRIMARY_KEY = "id";
    public static final String INDEX_FIELD_NAME_TITLE = "title";
    public static final String INDEX_FIELD_NAME_SCORE = "score";


    /** Returns a core named "test" with fields "id" as primary key and "title" as searchable text title. */
    @Override
    public IndexDescription getIndexDescription() {
        Field primaryKey = Field.getRefiningField(INDEX_FIELD_NAME_PRIMARY_KEY, Field.Type.TEXT);
        Field title  = Field.getSearchableField(INDEX_FIELD_NAME_TITLE);
        Field score  = Field.getRefiningField(INDEX_FIELD_NAME_SCORE, Field.Type.INTEGER);

        return new IndexDescription(INDEX_NAME, primaryKey, title, score);
    }

    /** Returns a set of records with "id" field incremented by for loop and "title" set to "Title# + <loopIteration>". */
    public JSONArray getRecordsArray(int numberOfRecordsToInsert, int primaryKeyStartIndice) {
        JSONArray recordsArray = new JSONArray();
        for (int i = primaryKeyStartIndice; i < numberOfRecordsToInsert + primaryKeyStartIndice; ++i) {
            JSONObject recordObject = new JSONObject();
            try {
                recordObject.put(INDEX_FIELD_NAME_PRIMARY_KEY, String.valueOf(i));
                recordObject.put(INDEX_FIELD_NAME_TITLE, "Title ");
                recordObject.put(INDEX_FIELD_NAME_SCORE, i);
            } catch (JSONException ignore) { }
            recordsArray.put(recordObject);
        }
        return recordsArray;
    }
}
