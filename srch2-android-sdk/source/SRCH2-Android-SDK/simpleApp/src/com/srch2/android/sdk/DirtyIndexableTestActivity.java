/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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


    private void initializeOnResumeCall() {
        SRCH2Engine.setIndexables(index);
        SRCH2Engine.setSearchResultsListener(searchResultsCallback);
    }

    public void testSaveIfDirtyIndexable() {
        assertFalse(SRCH2Engine.isReady());
        initializeOnResumeCall();
        onStartAndWaitForIsReady(this, 30000);
        assertTrue(SRCH2Engine.isReady());
        assertFalse(SRCH2Engine.conf.indexableMap.get(DumbIndex.INDEX_NAME).indexInternal.isDirty.get());
        onStopAndWaitForNotIsReady(this, 30000);
        assertFalse(SRCH2Engine.isReady());
        sleep(1000);
        initializeOnResumeCall();
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


        // NOTE there is a serious problem: how to test? since onPause no clears (stopping the SRCH2Engine)
        // the indexable map list, this will always work. HOWEVER, key is to test whether the save task did
        // actually execute at 98. for now assume, is it was dirty at line 97 multitask save task in
        // SRCH2Engine.onPause does start.

    }

    static class SearchResultsCallback implements SearchResultsListener {
        @Override
        public void onNewSearchResults(int HTTPResponseCode, String JSONResponse, HashMap<String, ArrayList<JSONObject>> resultMap) {

        }
    }

}
