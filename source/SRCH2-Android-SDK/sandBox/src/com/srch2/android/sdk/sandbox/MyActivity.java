package com.srch2.android.sdk.sandbox;

import android.app.Activity;
import android.os.Bundle;
import android.widget.ListView;
import com.srch2.android.sdk.SRCH2Engine;

public class MyActivity extends Activity implements InstantSearchEditText.SearchInputEnteredObserver {

    public ListView searchResultsListView;
    public SearchResultsAdapter resultsAdapter;
    public Idx index;
    public GeoIdx geoIndex;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_my);
        searchResultsListView = (ListView) findViewById(R.id.lv);
        resultsAdapter = new SearchResultsAdapter(this);
        searchResultsListView.setAdapter(resultsAdapter);
        index = new Idx();
        geoIndex = new GeoIdx();
        SRCH2Engine.initialize(geoIndex);
        SRCH2Engine.setSearchResultsListener(resultsAdapter.getSearchResultsListener());
        SRCH2Engine.setAutomatedTestingMode(true);

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
