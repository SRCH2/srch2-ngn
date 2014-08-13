package com.srch2.android.demo.helloworld;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.widget.ListView;
import android.widget.Toast;

import com.srch2.android.http.service.DeleteResponse;
import com.srch2.android.http.service.GetRecordResponse;
import com.srch2.android.http.service.InfoResponse;
import com.srch2.android.http.service.InsertResponse;
import com.srch2.android.http.service.SRCH2Engine;
import com.srch2.android.http.service.StateResponseListener;
import com.srch2.android.http.service.UpdateResponse;

import java.util.HashMap;


public class SearchActivity extends Activity implements InstantSearchEditText.SearchInputEnteredObserver {

    private static final String TAG = SearchActivity.class.getSimpleName();

    private MovieIndex mMovieIndex;

    private ListView mSearchResultsListView;
    private SearchResultsAdapter mSearchResultsAdapter;
    private SRCH2StateResponseListener mSRCH2StateResponseListener;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_search);

        setContentView(R.layout.activity_search);
        mSearchResultsListView = (ListView) findViewById(R.id.lv_search_results);
        mSearchResultsAdapter = new SearchResultsAdapter(this);
        mSearchResultsListView.setAdapter(mSearchResultsAdapter);

        setupSRCH2Engine();
    }

    private void setupSRCH2Engine() {
        // The following calls only need to be made when an instance of the
        // activity is created, so this method is called from onCreate().

        mMovieIndex = new MovieIndex();
        SRCH2Engine.initialize(mMovieIndex);

        // This callback can be reset and re-registered at any point.
        SRCH2Engine.setSearchResultsListener(mSearchResultsAdapter
                .getSearchResultsListener());

        // This callback can be reset and re-registered at any point as well.
        mSRCH2StateResponseListener = new SRCH2StateResponseListener(this);
        SRCH2Engine.setStateResponseListener(mSRCH2StateResponseListener);
    }

    @Override
    protected void onResume() {
        super.onResume();
        // This method is called in onResume because the SRCH2 search server should always be
        // available when this activity is in the foreground and is visible to the user, since
        // they may want to do searches. It is not called from onCreate() because the SRCH2
        // search server should only be active while this activity has the current focus.
        SRCH2Engine.onStart(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        // Since this activity is no longer going to be visible to and interacted with by the
        // user, the SRCH2 search server should stop being active. It is not immediately brought
        // to a halt but will persist for a short interval in case the user is simply switching to
        // another activity to reply to text or some other short-lived task and will return to this
        // activity. In this event, the stopping of the SRCH2 search server will be cancelled when
        // this activity's onResume() method is called, in turn calling SRCH2Engine.onStart(...).
        SRCH2Engine.onStop(this);
    }

    @Override
    public void onNewSearchInput(String newSearchText) {
        // Pass the text of the InstantSearchEditText input field to the SRCH2Engine for doing
        // searches. A search could also be performed here specifically on mMovieIndex by calling
        // either mMovieIndex.search(newSearchText) or SRCH2Engine.searchIndex(MovieIndex.INDEX_NAME).
        SRCH2Engine.searchAllIndexes(newSearchText);
    }

    @Override
    public void onNewSearchInputIsBlank() {
        // Since the input field of the InstantSearchEditText is now empty, do not perform a search
        // and simply clear the results of the list view by clearing the dataset of its backing
        // adapter.
        mSearchResultsAdapter.clearDisplayedSearchResults();
    }

    private static class SRCH2StateResponseListener implements
            StateResponseListener {

        private SearchActivity mActivity;

        public SRCH2StateResponseListener(SearchActivity myActivity) {
            mActivity = myActivity;
        }

        // The following callback methods are executed off the UI thread, so it is necessary to
        // pass any information they forward to the Ui thread if they are to be displayed to the user.

        @Override
        public void onInsertRequestComplete(final String indexName,
                                            final InsertResponse response) {
            Log.d(TAG, "Insert for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
        }

        @Override
        public void onUpdateRequestComplete(final String indexName,
                                            final UpdateResponse response) {
            Log.d(TAG, "Update for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
        }

        @Override
        public void onDeleteRequestComplete(final String indexName,
                                            final DeleteResponse response) {
            Log.d(TAG, "Delete for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
        }

        @Override
        public void onGetRecordByIDComplete(final String indexName,
                                            final GetRecordResponse response) {
            Log.d(TAG, "Got record for index: " + indexName
                    + " complete. Printing info:\n" + response.toString());
        }

        @Override
        public void onInfoRequestComplete(final String indexName,
                                          final InfoResponse response) {
            Log.d(TAG, "Info for index: " + indexName + ". Printing info:\n"
                    + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(),
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }

        @Override
        public void onSRCH2ServiceReady(
                final HashMap<String, InfoResponse> indexesToInfoResponseMap) {
            Log.d(TAG, "SRCH2 Search Service is ready for action!");
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity,
                                "SRCH2 Search Service is ready for action!",
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }

            // The very first time this callback method is executed, there will be
            // no records in the index: thus the number of records in the InfoResponse
            // for the MovieIndex is checked, and if it is zero the initial set of records
            // are inserted. The next time this method is triggered, after the application
            // is quit from and started again, the number of records will not be zero and
            // inserting an initial skip of records is no longer required.
            InfoResponse movieIndexInfoResponse = indexesToInfoResponseMap
                    .get(MovieIndex.INDEX_NAME);
            if (movieIndexInfoResponse != null
                    && movieIndexInfoResponse.isValidInfoResponse()) {
                if (movieIndexInfoResponse.getNumberOfDocumentsInTheIndex() == 0) {
                    SRCH2Engine.insertIntoIndex(MovieIndex.INDEX_NAME,
                            MovieIndex.getAFewRecordsToInsert());
                }
            }
        }
    }
}
