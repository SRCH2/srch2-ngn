package com.srch2.android.sdk;

import android.os.Bundle;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import static junit.framework.Assert.*;

public class HeartBeatTestActivity extends TestableActivity {

    DumbIndex index;
    SearchResultsCallback searchResultsCallback;

    @Override
    public List<String> getTestMethodNameListWithOrder() {
        return Arrays.asList(new String[]{
            "testHeartBeat",
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



    public void testHeartBeat() {
        assertTrue(HeartBeatPing.instance == null);
        assertFalse(SRCH2Engine.isReady());
        onStartAndWaitForIsReady(this, 60000);
        assertTrue(SRCH2Engine.isReady());
        assertTrue(HeartBeatPing.instance != null);
        JSONArray longTimeInsertionRecordArray = new JSONArray();
        for (int  i = 1; i < 5000; ++i) {
            longTimeInsertionRecordArray.put(DumbIndex.getRecord(String.valueOf(i), String.valueOf(i)));
        }
        index.insert(longTimeInsertionRecordArray);
        assertNull(HeartBeatPing.instance.timer);
        sleep(60000);
        assertNotNull(HeartBeatPing.instance.timer);
        onStopAndWaitForNotIsReady(this, 60000);
        assertTrue(HeartBeatPing.instance == null);
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
