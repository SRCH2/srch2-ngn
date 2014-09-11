package com.srch2.android.sdk.sandbox;

import android.content.Context;
import android.text.Html;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

import java.util.ArrayList;

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


    public SearchResultsAdapter(Context context) {
        mLayoutInflater = (LayoutInflater) context
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mSearchResults = new ArrayList<SearchResultItem>();
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
            viewHolder.mTitleTextView.setText(Html.fromHtml(searchResult.textOne));
            viewHolder.mGenreTextView.setText(searchResult.textTwo);
            viewHolder.mYearTextView.setText(searchResult.textThree);
            return convertView;
        }
    }


}