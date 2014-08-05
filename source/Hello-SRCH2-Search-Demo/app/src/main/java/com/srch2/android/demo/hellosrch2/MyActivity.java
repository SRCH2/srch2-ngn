package com.srch2.android.demo.hellosrch2;

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

public class MyActivity extends Activity implements InstantSearchEditText.SearchInputEnteredObserver {

    private static final String TAG = MyActivity.class.getSimpleName();

    private MovieIndex mMovieIndex;

    private ListView mSearchResultsListView;
    private SearchResultsAdapter mSearchResultsAdapter;
    private SRCH2StateResponseListener mSRCH2StateResponseListener;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_main);
        mSearchResultsListView = (ListView) findViewById(R.id.lv_search_results);
        mSearchResultsAdapter = new SearchResultsAdapter(this);
        mSearchResultsListView.setAdapter(mSearchResultsAdapter);

        setupSRCH2Engine();
    }

    private void setupSRCH2Engine() {
        SRCH2Engine.setSearchResultsListener(mSearchResultsAdapter.getSearchResultsListener());

        mSRCH2StateResponseListener = new SRCH2StateResponseListener(this);
        SRCH2Engine.setStateResponseListener(mSRCH2StateResponseListener);

        mMovieIndex = new MovieIndex();
        SRCH2Engine.initialize(mMovieIndex);
    }

    @Override
    protected void onResume() {
        super.onResume();
        SRCH2Engine.onStart(this);
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
        mSearchResultsAdapter.clearDisplayedSearchResults();
    }

    private static class SRCH2StateResponseListener implements StateResponseListener {

        private MyActivity mActivity;

        public SRCH2StateResponseListener(MyActivity myActivity) {
            mActivity = myActivity;
        }

        @Override
        public void onInfoRequestComplete(final String indexName, final InfoResponse response) {
            Log.d(TAG, "Info for index: " + indexName + ". Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(), Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }

        @Override
        public void onInsertRequestComplete(final String indexName, final InsertResponse response) {
            Log.d(TAG, "Insert for index: " + indexName + " complete. Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(), Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }

        @Override
        public void onUpdateRequestComplete(final String indexName, final UpdateResponse response) {
            Log.d(TAG, "Update for index: " + indexName + " complete. Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(), Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }

        @Override
        public void onSRCH2ServiceReady(final HashMap<String, InfoResponse> indexesToInfoResponseMap) {
            Log.d(TAG, "SRCH2 Search Service is ready for action!");
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, "SRCH2 Search Service is ready for action!", Toast.LENGTH_SHORT).show();
                    }
                });
            }
            InfoResponse movieIndexInfoResponse = indexesToInfoResponseMap.get(MovieIndex.INDEX_NAME);
            if (movieIndexInfoResponse != null && movieIndexInfoResponse.isValidInfoResponse()) {
                if (movieIndexInfoResponse.getNumberOfDocumentsInTheIndex() == 0) {
                    SRCH2Engine.insertIntoIndex(MovieIndex.INDEX_NAME, MovieIndex.getAFewRecordsToInsert());
                }
            }
        }

        @Override
        public void onDeleteRequestComplete(final String indexName, final DeleteResponse response) {
            Log.d(TAG, "Delete for index: " + indexName + " complete. Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(), Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }

        @Override
        public void onGetRecordByIDComplete(final String indexName, final GetRecordResponse response) {
            Log.d(TAG, "Got record for index: " + indexName + " complete. Printing info:\n" + response.toString());
            if (mActivity != null) {
                mActivity.runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mActivity, response.toToastString(), Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }
    }
}




