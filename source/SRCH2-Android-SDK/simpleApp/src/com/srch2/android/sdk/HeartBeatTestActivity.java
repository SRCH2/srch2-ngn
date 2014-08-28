package com.srch2.android.sdk;

import android.os.Bundle;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

public class HeartBeatTestActivity extends TestableActivity {

    DumbIndex index;
    SearchResultsCallback searchResultsCallback;

    @Override
    public List<String> getTestMethodNameListWithOrder() {
        return Arrays.asList(new String[]{

        });
    }

    @Override
    public void beforeAll() {
        deleteSrch2Files();
        index = new DumbIndex();
        searchResultsCallback = new SearchResultsCallback();
        SRCH2Engine.initialize(index);
        SRCH2Engine.setSearchResultsListener(searchResultsCallback);
    }

    @Override
    public void afterAll() {

    }

    @Override
    public void beforeEach() {

    }

    @Override
    public void afterEach() {

    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    

    static class DumbIndex extends Indexable {

        public static final String INDEX_NAME = "dumb-index";
        public static final String INDEX_FIELD_NAME_PRIMARY_KEY = "id";
        public static final String INDEX_FIELD_NAME_TITLE = "title";

        @Override
        public String getIndexName() {
            return INDEX_NAME;
        }

        @Override
        public Schema getSchema() {
            return Schema.createSchema(
                    Field.createDefaultPrimaryKeyField(INDEX_FIELD_NAME_PRIMARY_KEY),
                    Field.createSearchableField(INDEX_FIELD_NAME_TITLE));
        }

        @Override
        public void onInsertComplete(int success, int failed, String JSONResponse) {
            super.onInsertComplete(success, failed, JSONResponse);
        }

        @Override
        public void onUpdateComplete(int success, int upserts, int failed, String JSONResponse) {
            super.onUpdateComplete(success, upserts, failed, JSONResponse);
        }

        @Override
        public void onDeleteComplete(int success, int failed, String JSONResponse) {
            super.onDeleteComplete(success, failed, JSONResponse);
        }

        @Override
        public void onGetRecordComplete(boolean success, JSONObject record, String JSONResponse) {
            super.onGetRecordComplete(success, record, JSONResponse);
        }

        @Override
        public void onIndexReady() {
            super.onIndexReady();
        }
    }

    static class SearchResultsCallback implements SearchResultsListener {
        @Override
        public void onNewSearchResults(int HTTPResponseCode, String JSONResponse, HashMap<String, ArrayList<JSONObject>> resultMap) {

        }
    }

    void sleep(int sleepTime) {
        try {
            Thread.currentThread().sleep(sleepTime);
        } catch (InterruptedException e) {
        }
    }
}
