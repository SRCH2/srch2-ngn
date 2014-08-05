package com.srch2.android.demo.helloworld;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;

import com.srch2.android.http.service.SearchResultsListener;

import org.json.JSONException;
import org.json.JSONObject;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;

public class SearchResultsAdapter extends BaseAdapter {

	private static class ViewHolder {
		public TextView mTitleTextView;
		public TextView mSubtitleTextView;
		
		public ViewHolder(TextView titleTextView, TextView subtitleTextView) {
			mTitleTextView = titleTextView;
			mSubtitleTextView = subtitleTextView;
		}
	}
	
	private ArrayList<SearchResult> mSearchResults;
	private LayoutInflater mLayoutInflater;
	private SearchResultsUiHandler mSearchResultsUiHandler;
	
	public SearchResultsListener getSearchResultsListener() { return mSearchResultsUiHandler; }
	
	public SearchResultsAdapter(Context context) {
		mLayoutInflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		mSearchResults = new ArrayList<SearchResult>();
		mSearchResultsUiHandler = new SearchResultsUiHandler(this);
	}
	
	public void clearDisplayedSearchResults() {
		mSearchResults.clear();
		notifyDataSetChanged();
	}
	
	public void updateDisplayedSearchResults(ArrayList<SearchResult> newSearchResults) {
		mSearchResults.clear();
		mSearchResults.addAll(newSearchResults);
		notifyDataSetChanged();
	}
	
	@Override
	public int getCount() {
		return mSearchResults == null ? 0 : mSearchResults.size();
	}

	@Override
	public SearchResult getItem(int position) {
		return mSearchResults == null ? null : mSearchResults.get(position);
	}

	@Override
	public long getItemId(int position) {
		return position;
	}

	@Override
	public View getView(int position, View convertView, ViewGroup parent) {
		SearchResult searchResult = mSearchResults.get(position);
		if (searchResult == null) { // probably unnecessary
			View view = new View(parent.getContext());
			view.setVisibility(View.GONE);
			return view;
		} else {
			ViewHolder viewHolder;
			if (convertView == null) {
				convertView = mLayoutInflater.inflate(R.layout.listview_search_result_row, parent, false);
				TextView titleTextView = (TextView) convertView.findViewById(R.id.tv_title_search_result_row);
				TextView subtitleTextView = (TextView) convertView.findViewById(R.id.tv_subtitle_search_result_row);	
				viewHolder = new ViewHolder(titleTextView, subtitleTextView);
				convertView.setTag(viewHolder);
			} else {
				viewHolder = (ViewHolder) convertView.getTag();
			}
			viewHolder.mTitleTextView.setText(searchResult.mTitle);
			viewHolder.mSubtitleTextView.setText(searchResult.mSubtitle);
			return convertView;
		}
	}
	
	private static class SearchResultsUiHandler extends Handler implements SearchResultsListener {

		private static final int MESSAGE_PUBLISH_NEW_RESULTS = 001;
		private static final int MESSAGE_PUBLISH_NO_NEW_RESULTS = 002;
		
		private WeakReference<SearchResultsAdapter> mSearchResultsAdapterWeakReference;

		public SearchResultsUiHandler(SearchResultsAdapter searchResultsAdapter) {
			mSearchResultsAdapterWeakReference = new WeakReference<SearchResultsAdapter>(searchResultsAdapter);
		}
		
		@Override
		public void handleMessage(Message msg) {
			Log.d("handler", "handle message msg what is " + msg.what);
			SearchResultsAdapter searchResultAdapter = mSearchResultsAdapterWeakReference.get();
			
			if (searchResultAdapter != null) {
				switch (msg.what){
					case MESSAGE_PUBLISH_NEW_RESULTS:
						ArrayList<SearchResult> newResults = null;
						try {
							newResults = (ArrayList<SearchResult>) msg.obj;
						} catch (ClassCastException oops) {
						}
						
						if (newResults != null) {
							searchResultAdapter.updateDisplayedSearchResults(newResults);
						}
						return;
					case MESSAGE_PUBLISH_NO_NEW_RESULTS:
						searchResultAdapter.clearDisplayedSearchResults();
						return;
				}
			}
		}

		@Override
		public void onNewSearchResults(int httpResponseCode, String jsonResponse, HashMap<String, ArrayList<JSONObject>> resultRecordMap) {
            Log.d("handler", "onNewSearchResults hrc:" + httpResponseCode + " jsonresponse:" + jsonResponse);
			if (httpResponseCode == 200) {
				
				ArrayList<SearchResult> newResults = new ArrayList<SearchResult>();
				
				ArrayList<JSONObject> musicResults = resultRecordMap.get(MusicIndex.INDEX_NAME);
				if (musicResults != null) {
					for (JSONObject jsonObject : musicResults) {
						SearchResult searchResult = null;
						try {
							searchResult = new SearchResult(jsonObject.getString(MusicIndex.INDEX_KEY_SONG_TITLE), jsonObject.getString(MusicIndex.INDEX_KEY_GENRE));
						} catch (JSONException oops) {
						}
						
						if (searchResult != null) {
							newResults.add(searchResult);
						}
					}
				}
				
				ArrayList<JSONObject> movieResults = resultRecordMap.get(MovieIndex.INDEX_NAME);
				if (movieResults != null) {
					for (JSONObject jsonObject : movieResults) {
						SearchResult searchResult = null;
						try {
							searchResult = new SearchResult(jsonObject.getString(MovieIndex.INDEX_KEY_TITLE), jsonObject.getString(MovieIndex.INDEX_KEY_GENRE));
						} catch (JSONException oops) {
						}
						
						if (searchResult != null) {
							newResults.add(searchResult);
						}
					}
				}
				sendMessage(Message.obtain(this, newResults.size() > 0 ? MESSAGE_PUBLISH_NEW_RESULTS : MESSAGE_PUBLISH_NO_NEW_RESULTS, newResults));	
			}
		}

	}
}