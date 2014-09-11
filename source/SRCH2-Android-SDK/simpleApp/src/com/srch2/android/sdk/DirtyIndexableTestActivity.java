package com.srch2.android.sdk;

import android.os.Bundle;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import static junit.framework.Assert.assertFalse;
import static junit.framework.Assert.assertTrue;

public class DirtyIndexableTestActivity extends TestableActivity {

    DumbIndex index;
    SearchResultsCallback searchResultsCallback;

    @Override
    public List<String> getTestMethodNameListWithOrder() {
        return Arrays.asList(new String[]{
                "testSaveIfDirtyIndexable",
        });
    }

    @Override
    public void beforeAll() {
        deleteSrch2Files();
        index = new DumbIndex();
        searchResultsCallback = new SearchResultsCallback();
        SRCH2Engine.setIndexables(index);
        SRCH2Engine.onStart();
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


    public void testSaveIfDirtyIndexable() {
        assertFalse(SRCH2Engine.isReady());
        onStartAndWaitForIsReady(this, 30000);
        assertTrue(SRCH2Engine.isReady());
        assertFalse(SRCH2Engine.conf.indexableMap.get(DumbIndex.INDEX_NAME).indexInternal.isDirty.get());
        onStopAndWaitForNotIsReady(this, 30000);
        assertFalse(SRCH2Engine.isReady());
        sleep(1000);
        onStartAndWaitForIsReady(this, 30000);
        assertTrue(SRCH2Engine.isReady());
        assertFalse(SRCH2Engine.conf.indexableMap.get(DumbIndex.INDEX_NAME).indexInternal.isDirty.get());
        JSONArray records = new JSONArray();
        StringBuilder sb = new StringBuilder();
        for (int i = 1; i < 100; ++i) {
            sb.setLength(0);
            for (int j = 1; j < i + 1; ++j) {
                sb.append(String.valueOf(i));
                sb.append(" title ");
                sb.append(String.valueOf(j));
            }
            records.put(DumbIndex.getRecord(String.valueOf(i), sb.toString()));
        }
        index.insert(records);
        sleep(15000);
        assertTrue(SRCH2Engine.conf.indexableMap.get(DumbIndex.INDEX_NAME).indexInternal.isDirty.get());
        onStopAndWaitForNotIsReady(this, 30000);
        assertFalse(SRCH2Engine.isReady());
        sleep(30000);
        assertFalse(SRCH2Engine.conf.indexableMap.get(DumbIndex.INDEX_NAME).indexInternal.isDirty.get());


    }

    static class SearchResultsCallback implements SearchResultsListener {
        @Override
        public void onNewSearchResults(int HTTPResponseCode, String JSONResponse, HashMap<String, ArrayList<JSONObject>> resultMap) {

        }
    }

}
