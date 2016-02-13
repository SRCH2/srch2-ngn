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
package com.srch2.android.sdk.sandbox;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ListView;
import com.srch2.android.sdk.SRCH2Engine;
import com.srch2.android.sdk.SearchResultsListener;
import org.json.JSONObject;
import java.util.ArrayList;
import java.util.HashMap;

public class MyActivity extends Activity implements InstantSearchEditText.SearchInputEnteredObserver {


    public static final String S2_TAG = "s2sb:: ";

    public ListView searchResultsListView;
    public SearchResultsAdapter resultsAdapter;
    public Idx index;
    public IdxTwo indexTwo;
    public GeoIdx geoIndex;
    public Context context;
    DatabaseHelper dbHelper;
    SearchResults sr;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getActionBar().hide();
        Log.d("s2sdk::Sandbox", "onCreate------------------------------------------!!!");
        context = this;
        setContentView(R.layout.activity_my);
        searchResultsListView = (ListView) findViewById(R.id.lv);
        resultsAdapter = new SearchResultsAdapter(this);
        searchResultsListView.setAdapter(resultsAdapter);

        index = new Idx();
       // indexTwo = new IdxTwo();


     //   dbHelper = new DatabaseHelper(this);



        sr = new SearchResults();
        SRCH2Engine.setAutomatedTestingMode(true);

    }



    class SearchResults implements SearchResultsListener {
        @Override
        public void onNewSearchResults(int HTTPResponseCode, String JSONResponse, HashMap<String, ArrayList<JSONObject>> resultMap) {

            Log.d(S2_TAG.concat("SEARCHRESULTS"), "response is [" + JSONResponse + "]");


            ArrayList<SearchResultsAdapter.SearchResultItem> items = new ArrayList<SearchResultsAdapter.SearchResultItem>();

            ArrayList<JSONObject> idxResults = resultMap.get(Idx.INDEX_NAME);
            if (idxResults != null && index != null) {
                items.addAll(index.wrap(idxResults));
            }

            ArrayList<JSONObject> idx2Results = resultMap.get(IdxTwo.INDEX_NAME);
            if (idx2Results != null && indexTwo != null) {
                items.addAll(indexTwo.wrap(idx2Results));
            }


            ArrayList<JSONObject> dbResults = resultMap.get(DatabaseHelper.SQLiteSchema.TABLE_NAME);
            if (dbResults != null && dbHelper != null) {
                items.addAll(dbHelper.wrap(dbResults));
            }


            resultsAdapter.updateDisplayedSearchResults(items);

        }
    }

    public void foo(View v) {


    }


    public void bar(View v) {

        Thread t = new Thread( new Runnable() {
            @Override
            public void run() {
                if (dbHelper != null) {
                    dbHelper.insertRecords();
                }
                if (index != null) {
                    index.insert(index.getRecords());
                }
                if (indexTwo != null) {
                    indexTwo.insert(indexTwo.getRecords());
                }
            }
        });
        t.start();
    }


    @Override
    protected void onResume() {
        super.onResume();
        Log.d("s2sdk::Sandbox", "onResume------------------------------------------!!!");

        SRCH2Engine.setIndexables(index);
        SRCH2Engine.onResume(this, sr, true);
        InstantSearchEditText.checkIfSearchInputShouldOpenSoftKeyboard(this, (InstantSearchEditText) findViewById(R.id.et_instant_search_input));
    }

    @Override
    protected void onPause() {
        super.onPause();

        Log.d("s2sdk::Sandbox", "onPause------------------------------------------!!!");
        SRCH2Engine.onPause(this);
    }



    @Override
    public void onNewSearchInput(String newSearchText) {

        SRCH2Engine.searchAllIndexes(newSearchText);
    }

    @Override
    public void onNewSearchInputIsBlank() {
        resultsAdapter.clearDisplayedSearchResults();
    }

}
