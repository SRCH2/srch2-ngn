package com.srch2.android.demo.helloworld;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ListView;
import android.widget.Toast;

import com.srch2.android.http.service.ControlResponseListener;
import com.srch2.android.http.service.DeleteResponse;
import com.srch2.android.http.service.GetRecordResponse;
import com.srch2.android.http.service.InfoResponse;
import com.srch2.android.http.service.InsertResponse;
import com.srch2.android.http.service.SRCH2Configuration;
import com.srch2.android.http.service.SRCH2Engine;
import com.srch2.android.http.service.UpdateResponse;

import java.lang.ref.WeakReference;
import java.util.HashMap;

public class MainActivity extends Activity implements InstantSearchEditText.SearchInputEnteredObserver {
	
	private static final String TAG = MainActivity.class.getSimpleName();
	
	private MusicIndex mMusicIndex;
	private MovieIndex mMovieIndex;
	
	private ListView mSearchResultsListView;
	private SearchResultsAdapter mSearchResultsAdapter;
	private IndexControlResponseListener mIndexControlResponseListener;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.activity_main);
		mSearchResultsListView = (ListView) findViewById(R.id.lv_search_results);
		mSearchResultsAdapter = new SearchResultsAdapter(this);
		mSearchResultsListView.setAdapter(mSearchResultsAdapter);


		mMusicIndex = new MusicIndex();
		mMovieIndex = new MovieIndex();
        SRCH2Configuration configuration = new SRCH2Configuration(mMusicIndex);

		mIndexControlResponseListener = new IndexControlResponseListener(this);

		SRCH2Engine.initialize(configuration, mIndexControlResponseListener, mSearchResultsAdapter.getSearchResultsListener(), true);
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




    public void doButtonClick(View v) {
        if (mMusicIndex != null) {
            Log.d("srch2:: MainActivity", "doign info on music!");
            mMusicIndex.getRecordbyID("one");

        }
    }





	private static class IndexControlResponseListener implements ControlResponseListener {

		private WeakReference<MainActivity> mMainActivityWeakReference;
		
		public IndexControlResponseListener(MainActivity mainActivity) {
			mMainActivityWeakReference = new WeakReference<MainActivity>(mainActivity);
		}
		
		@Override
		public void onDeleteRequestComplete(String arg0, DeleteResponse arg1) {
		}

		@Override
		public void onGetRecordByIDComplete(String arg0, GetRecordResponse arg1) {

            Log.d("srch2:: MainActivity", "RECORD BY ID " + arg1.toHumanReadableString());

		}

		@Override
		public void onInfoRequestComplete(String arg0, InfoResponse arg1) {
		}

		@Override
		public void onInsertRequestComplete(String arg0, InsertResponse arg1) {



		}

		@Override
		public void onSRCH2ServiceReady(HashMap<String, InfoResponse> arg0) {
			final MainActivity mainActivity = mMainActivityWeakReference.get();
			if (mainActivity != null) {
				
				mainActivity.runOnUiThread(new Runnable() {
					@Override
					public void run() {
						Toast.makeText(mainActivity, "SRCH2 Search Service is ready!", Toast.LENGTH_SHORT).show();
					}
				});
				
				InfoResponse musicIndexInfoResponse = arg0.get(MusicIndex.INDEX_NAME);
                Log.d("srch2:: MainActivity", musicIndexInfoResponse.toHumanReadableString());
				if (musicIndexInfoResponse.isValidInfoResponse) {
					if (musicIndexInfoResponse.numberOfDocumentsInTheIndex == 0) {
						mainActivity.mMusicIndex.insertRecords();
					} 
				}
				/*
				InfoResponse moviesIndexInfoResponse = arg0.get(MovieIndex.INDEX_NAME);
				if (moviesIndexInfoResponse.isValidInfoResponse) {
					if (moviesIndexInfoResponse.numberOfDocumentsInTheIndex == 0) {
						mainActivity.mMovieIndex.insertRecords();
					} 
				}*/
			}
		}

		@Override
		public void onUpdateRequestComplete(String arg0, UpdateResponse arg1) {
			// TODO Auto-generated method stub
			
		}
		
	}
	
}
