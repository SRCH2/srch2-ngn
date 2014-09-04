package com.srch2.android.sdk.sandbox;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ListView;
import android.widget.Toast;
import com.srch2.android.sdk.SRCH2Engine;
import com.srch2.android.sdk.SearchResultsListener;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;

public class MyActivity extends Activity implements InstantSearchEditText.SearchInputEnteredObserver {

    public ListView searchResultsListView;
    public SearchResultsAdapter resultsAdapter;
    public Idx index;
    public IdxTwo indexTwo;
    public GeoIdx geoIndex;
    public Context context;

    SearchResults sr;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        context = this;
        setContentView(R.layout.activity_my);
        searchResultsListView = (ListView) findViewById(R.id.lv);
        resultsAdapter = new SearchResultsAdapter(this);
        searchResultsListView.setAdapter(resultsAdapter);
        index = new Idx();
        indexTwo = new IdxTwo();
        geoIndex = new GeoIdx();

        SRCH2Engine.initialize(index, indexTwo);

        sr = new SearchResults();
        SRCH2Engine.setSearchResultsListener(sr, true);
        SRCH2Engine.setAutomatedTestingMode(true);
    }

    class SearchResults implements SearchResultsListener {
        @Override
        public void onNewSearchResults(int HTTPResponseCode, String JSONResponse, HashMap<String, ArrayList<JSONObject>> resultMap) {

            Log.d("SEARCHRESULTS", "response is [" + JSONResponse + "]");

            int count = 0;


            for (ArrayList<JSONObject> results : resultMap.values()) {
                for (JSONObject jo : results) {
                    ++count;
                }
            }
            Log.d("SandboxMyActivity", "onnewsearchresults " + count);
            Toast.makeText(context, "result count: " + count, Toast.LENGTH_LONG).show();
        }
    }

    Object o;
    public void foo(View v) {


        o.hashCode();
    }


    @Override
    protected void onResume() {
        super.onResume();
        SRCH2Engine.onStart(this);
        InstantSearchEditText.checkIfSearchInputShouldOpenSoftKeyboard(this, (InstantSearchEditText) findViewById(R.id.et_instant_search_input));
    }

    @Override
    protected void onPause() {
        super.onPause();
        SRCH2Engine.onStop(this);
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
