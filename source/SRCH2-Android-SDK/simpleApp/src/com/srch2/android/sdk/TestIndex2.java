package com.srch2.android.sdk;

public class TestIndex2 extends TestIndex {
    public static final String INDEX_NAME = "test2";


    @Override
    public String getIndexName() {
        return INDEX_NAME;
    }

    @Override
    public Schema getSchema() {
        PrimaryKeyField primaryKey = Field.createSearchablePrimaryKeyField(INDEX_FIELD_NAME_PRIMARY_KEY);
        Field title = Field.createSearchableField(INDEX_FIELD_NAME_TITLE);
        Field score = Field.createRefiningField(INDEX_FIELD_NAME_SCORE, Field.Type.INTEGER);
        return new Schema(primaryKey, title, score);
    }
}
