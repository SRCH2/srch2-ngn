package com.srch2.android.sdk.sandbox;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;
import com.srch2.android.sdk.SearchResultsListener;
import org.json.JSONObject;

import java.lang.ref.WeakReference;
import java.net.HttpURLConnection;
import java.util.ArrayList;
import java.util.HashMap;

public class SearchResultsAdapter extends BaseAdapter {

    private static class ViewHolder {
        public TextView mTitleTextView;
        public TextView mGenreTextView;
        public TextView mYearTextView;

        public ViewHolder(TextView titleTextView, TextView genreTextView,
                          TextView yearTextView) {
            mTitleTextView = titleTextView;
            mGenreTextView = genreTextView;
            mYearTextView = yearTextView;
        }
    }

    public static class SearchResultItem {
        public final String textOne;
        public final String textTwo;
        public final String textThree;

        public SearchResultItem(String theTextOne, String theTextTwo, String theTextThree) {
            textOne = theTextOne;
            textTwo = theTextTwo;
            textThree = theTextThree;
        }
    }

    private ArrayList<SearchResultItem> mSearchResults;
    private LayoutInflater mLayoutInflater;
    private SearchResultsUiHandler mSearchResultsUiHandler;

    public SearchResultsListener getSearchResultsListener() {
        return mSearchResultsUiHandler;
    }

    public SearchResultsAdapter(Context context) {
        mLayoutInflater = (LayoutInflater) context
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mSearchResults = new ArrayList<SearchResultItem>();
        mSearchResultsUiHandler = new SearchResultsUiHandler(this);
    }

    public void clearDisplayedSearchResults() {
        mSearchResults.clear();
        notifyDataSetChanged();
    }

    public void updateDisplayedSearchResults(
            ArrayList<SearchResultItem> newSearchResults) {
        // Swap out the data set of this adapter with the new set of search results
        // and invalidate the list view this adapter is backing with these new
        // search results.
        mSearchResults.clear();
        mSearchResults.addAll(newSearchResults);
        notifyDataSetChanged();
    }

    @Override
    public int getCount() {
        return mSearchResults == null ? 0 : mSearchResults.size();
    }

    @Override
    public SearchResultItem getItem(int position) {
        return mSearchResults == null ? null : mSearchResults.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        SearchResultItem searchResult = mSearchResults.get(position);
        if (searchResult == null) {
            View view = new View(parent.getContext());
            view.setVisibility(View.GONE);
            return view;
        } else {
            ViewHolder viewHolder;
            if (convertView == null) {
                convertView = mLayoutInflater.inflate(
                        R.layout.row, parent, false);
                TextView titleTextView = (TextView) convertView
                        .findViewById(R.id.tv1);
                TextView genreTextView = (TextView) convertView
                        .findViewById(R.id.tv2);
                TextView yearTextView = (TextView) convertView
                        .findViewById(R.id.tv3);
                viewHolder = new ViewHolder(titleTextView, genreTextView,
                        yearTextView);
                convertView.setTag(viewHolder);
            } else {
                viewHolder = (ViewHolder) convertView.getTag();
            }
            viewHolder.mTitleTextView.setText(searchResult.textOne);
            viewHolder.mGenreTextView.setText(searchResult.textTwo);
            viewHolder.mYearTextView.setText(searchResult.textThree);
            return convertView;
        }
    }

    private static class SearchResultsUiHandler extends Handler implements
            SearchResultsListener {

        // Since the SearchResultsListener callback method onNewSearchResults(...)
        // is executed off the Ui thread, it is implemented in a handler that will
        // pass the search results of this callback to the Ui thread: specifically,
        // to the SearchResultsAdapter so they can be displayed to the user.

        private static final int MESSAGE_WHAT_PUBLISH_NEW_RESULTS = 001;
        private static final int MESSAGE_WHAT_PUBLISH_NO_NEW_RESULTS = 002;

        private WeakReference<SearchResultsAdapter> mSearchResultsAdapterWeakReference;

        public SearchResultsUiHandler(SearchResultsAdapter searchResultsAdapter) {
            mSearchResultsAdapterWeakReference = new WeakReference<SearchResultsAdapter>(
                    searchResultsAdapter);
        }

        @Override
        public void handleMessage(Message msg) {

            SearchResultsAdapter searchResultAdapter = mSearchResultsAdapterWeakReference
                    .get();

            if (searchResultAdapter != null) {
                switch (msg.what) {
                    case MESSAGE_WHAT_PUBLISH_NEW_RESULTS:
                        ArrayList<SearchResultItem> newResults = null;
                        try {
                            newResults = (ArrayList<SearchResultItem>) msg.obj;
                        } catch (ClassCastException oops) {
                        }

                        if (newResults != null) {
                            searchResultAdapter
                                    .updateDisplayedSearchResults(newResults);
                        }
                        return;
                    case MESSAGE_WHAT_PUBLISH_NO_NEW_RESULTS:
                        searchResultAdapter.clearDisplayedSearchResults();
                        return;
                }
            }
        }

        @Override
        public void onNewSearchResults(int httpResponseCode,
                                       String jsonResponse,
                                       HashMap<String, ArrayList<JSONObject>> resultRecordMap) {
            if (httpResponseCode == HttpURLConnection.HTTP_OK) {
                ArrayList<SearchResultItem> newResults = new ArrayList<SearchResultItem>();

                ArrayList<JSONObject> movieResults = resultRecordMap
                        .get(Idx.INDEX_NAME);
                if (movieResults != null && movieResults.size() > 0) {
                    ArrayList<SearchResultItem> results = Idx.wrap(movieResults);
                    if (results != null) {
                        newResults.addAll(results);
                    }
                }
                sendMessage(Message
                        .obtain(this,
                                newResults.size() > 0 ? MESSAGE_WHAT_PUBLISH_NEW_RESULTS
                                        : MESSAGE_WHAT_PUBLISH_NO_NEW_RESULTS,
                                newResults));
            }
        }
    }
}