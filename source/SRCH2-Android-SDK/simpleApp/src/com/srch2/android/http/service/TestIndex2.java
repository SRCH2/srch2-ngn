package com.srch2.android.http.service;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * Created by jianfeng on 8/4/14.
 */
public class TestIndex2 extends TestIndex {
    public static final String INDEX_NAME = "test2";

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

}
