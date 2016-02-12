package com.srch2.android.sdk;

import android.os.Bundle;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import static junit.framework.Assert.*;

public class LifeCycleTestActivity extends TestableActivity {

    DumbIndex index;
    SearchResultsCallback searchResultsCallback;

    @Override
    public List<String> getTestMethodNameListWithOrder() {
        return Arrays.asList(new String[]{
                "testLifeCycle",
        });
    }

    @Override
    public void beforeAll() {
        SRCH2Service.clearServerLogEntriesForTest(getApplicationContext());
        deleteSrch2Files();
        index = new DumbIndex();
        searchResultsCallback = new SearchResultsCallback();

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

    private void initializeOnResume() {
        SRCH2Engine.setIndexables(index);;
        SRCH2Engine.setSearchResultsListener(searchResultsCallback);
    }

    public void testLifeCycle() {
        assertFalse(SRCH2Engine.isReady());
        initializeOnResume();
        onStartAndWaitForIsReady(this, 60000);
        assertTrue(SRCH2Engine.isReady());
        String serverUrl = SRCH2Engine.getConfig().getUrlString();
        onStopAndWaitForNotIsReady(this, 15000);
        assertFalse(SRCH2Engine.isReady());
        initializeOnResume();
        onStartAndWaitForIsReady(this, 60000);
        assertTrue(SRCH2Engine.isReady());
        assertEquals(serverUrl, SRCH2Engine.getConfig().getUrlString());
        onStopAndWaitForNotIsReady(this, 60000);
        assertFalse(SRCH2Engine.isReady());
        sleep(30000);
        initializeOnResume();
        onStartAndWaitForIsReady(this, 60000);
        assertTrue(SRCH2Engine.isReady());
        assertNotSame(serverUrl, SRCH2Engine.getConfig().getUrlString());
        onStopAndWaitForNotIsReady(this, 60000);
        assertFalse(SRCH2Engine.isReady());
    }

    static class SearchResultsCallback implements SearchResultsListener {
        @Override
        public void onNewSearchResults(int HTTPResponseCode, String JSONResponse, HashMap<String, ArrayList<JSONObject>> resultMap) {

        }
    }

}
