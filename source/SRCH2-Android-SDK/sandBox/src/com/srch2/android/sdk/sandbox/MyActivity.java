package com.srch2.android.sdk.sandbox;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
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
        geoIndex = new GeoIdx();

        SRCH2Engine.initialize(geoIndex);

        sr = new SearchResults();
        SRCH2Engine.setSearchResultsListener(sr, true);
        SRCH2Engine.setAutomatedTestingMode(true);
    }

    class SearchResults implements SearchResultsListener {
        @Override
        public void onNewSearchResults(int HTTPResponseCode, String JSONResponse, HashMap<String, ArrayList<JSONObject>> resultMap) {
            Toast.makeText(context, "hello everybody", Toast.LENGTH_LONG).show();
        }
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
