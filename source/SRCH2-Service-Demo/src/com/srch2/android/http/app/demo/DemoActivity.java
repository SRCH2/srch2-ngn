package com.srch2.android.http.app.demo;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.atomic.AtomicBoolean;

import org.json.JSONObject;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.srch2.android.http.app.demo.InstantSearchEditText.SearchInputEnteredObserver;
import com.srch2.android.http.app.demo.data.ViewableResult;
import com.srch2.android.http.app.demo.data.contacts.ContactViewableResult;
import com.srch2.android.http.app.demo.data.contacts.Contacts;
import com.srch2.android.http.app.demo.data.incremental.IncrementalDatabase;
import com.srch2.android.http.app.demo.data.incremental.IncrementalValuePair;
import com.srch2.android.http.service.ControlResponseListener;
import com.srch2.android.http.service.DeleteResponse;
import com.srch2.android.http.service.Field;
import com.srch2.android.http.service.GetRecordResponse;
import com.srch2.android.http.service.Index;
import com.srch2.android.http.service.IndexDescription;
import com.srch2.android.http.service.InfoResponse;
import com.srch2.android.http.service.InsertResponse;
import com.srch2.android.http.service.SRCH2Configuration;
import com.srch2.android.http.service.SRCH2Engine;
import com.srch2.android.http.service.SearchResultsListener;
import com.srch2.android.http.service.UpdateResponse;

public class DemoActivity extends Activity implements
		SearchInputEnteredObserver {
	public static final String TAG = "DemoActivity";

	private Context context;

	SearchResults searchResultsCallback;
	IndexControl indexControlCallback;

	Contacts contactsIndexWrapper;

	GooeyHandler uiHandler;
	ListView searchResultsListView;
	Adapter searchResultsListViewAdapter;

	AtomicBoolean onCreateCalled = new AtomicBoolean(true);

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Log.d("srch2:: " + TAG, "onCreate");

		context = this;

		ContextSingleton.link(this);
		DefaultUiResources.init(this);

		searchResultsCallback = new SearchResults(this);
		indexControlCallback = new IndexControl(this);
		uiHandler = new GooeyHandler(this);

		setContentView(R.layout.activity_demo);

		searchResultsListView = (ListView) findViewById(R.id.lv_search_results);
		searchResultsListViewAdapter = new Adapter(this);
		searchResultsListView.setAdapter(searchResultsListViewAdapter);

		setupSRCH2API();
	}

	public void setupSRCH2API() {
		Log.d("srch2:: " + TAG, "setupSRCH2API");

		SRCH2Configuration config = new SRCH2Configuration();

		IndexDescription contactsIndexDescription = new IndexDescription(
				"contacts", Field.getRefiningField("id", Field.Type.TEXT),
				Field.getSearchableField("name", 2),
				Field.getSearchableField("fulltext"), Field.getRefiningField(
						"score", Field.Type.FLOAT));

		Index i = config.createIndex(contactsIndexDescription);

		contactsIndexWrapper = new Contacts(i, this);

		SRCH2Engine.initialize(config, indexControlCallback,
				searchResultsCallback);
	}

	@Override
	protected void onResume() {
		super.onResume();
		Log.d("srch2:: " + TAG, "onResume");

		SRCH2Engine.onActivityResumed(context);

		contactsIndexWrapper.startObserveringIncrementalState(this);
		if (onCreateCalled.get()) {
			onCreateCalled.set(false);

		} else {
			Log.d("srch2:: " + TAG,
					"onResume - onCreate was false should update index now");

			contactsIndexWrapper.updateIndex(context);
		}

	}

	@Override
	protected void onPause() {
		super.onPause();

		Log.d("srch2:: " + TAG, "onPause");
		Thread t = new Thread(new Runnable() {

			@Override
			public void run() {
				contactsIndexWrapper.stopObserveringIncrementalState(context);
				IncrementalDatabase id = new IncrementalDatabase(context);
				id.serializeIncrementalState("contacts",
						contactsIndexWrapper.incrementalState
								.getSnapShotAsArrayList());
				SRCH2Engine.onActivityPaused(context);
			}
		});
		t.start();

	}

	class Adapter extends BaseAdapter {

		LayoutInflater linflator;
		ArrayList<ViewableResult> items;

		TouchController tc;

		public Adapter(Context c) {
			linflator = (LayoutInflater) c
					.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			items = new ArrayList<ViewableResult>();
			tc = new TouchController(c);
		}

		@Override
		public int getCount() {
			return items == null ? 0 : items.size();
		}

		@Override
		public ViewableResult getItem(int position) {
			return items == null ? null : items.get(position);
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {

			View v = linflator.inflate(R.layout.row_viewable_result, parent,
					false);
			v.setOnTouchListener(tc);

			ViewableResult vr = getItem(position);
			if (vr == null) {
				return v;
			}

			v.setTag(R.id.tag_viewable_result, vr);

			RelativeLayout rowRoot = (RelativeLayout) v
					.findViewById(R.id.rl_row_root);
			LinearLayout rowContent = (LinearLayout) v
					.findViewById(R.id.ll_row_content_container);
			View indicator = (View) v
					.findViewById(R.id.v_row_colorCodeIndicator);
			ImageView icon = (ImageView) v.findViewById(R.id.iv_row_icon);
			TextView title = (TextView) v.findViewById(R.id.tv_row_title);

			indicator.setBackgroundColor(vr.indicatorColor);
			icon.setImageBitmap(vr.icon);
			title.setText(vr.title);

			ImageView button1 = (ImageView) v
					.findViewById(R.id.iv_row_special_button_1);
			ImageView button2 = (ImageView) v
					.findViewById(R.id.iv_row_special_button_2);
			ImageView button3 = (ImageView) v
					.findViewById(R.id.iv_row_special_button_3);

			HashMap<Integer, ImageView> buttons = new HashMap<Integer, ImageView>();
			buttons.put(R.id.iv_row_special_button_1, button1);
			buttons.put(R.id.iv_row_special_button_2, button2);
			buttons.put(R.id.iv_row_special_button_3, button3);

			for (Integer id : buttons.keySet()) {
				if (vr.rowButtonClickMap.containsKey(id)) {

					ImageView iv = buttons.get(id);
					iv.setVisibility(View.VISIBLE);
					iv.setImageBitmap(vr.rowButtonClickMap.get(id));
					iv.setOnClickListener(tc);
				} else {
					buttons.get(id).setVisibility(View.GONE);
				}
			}
			rowRoot.setTag(R.id.tag_row_content, rowContent);
			return v;
		}

		synchronized public void updateSet(
				ArrayList<ViewableResult> newResultSet) {
			Log.d("srch2:: " + TAG, "Adapter - updateSet");
			items.clear();
			if (newResultSet != null && newResultSet.size() > 0) {
				items.addAll(newResultSet);
			}
			notifyDataSetChanged();
		}

		synchronized public void clearSet() {
			Log.d("srch2:: " + TAG, "Adapter - clearSet");
			items.clear();
			notifyDataSetChanged();
		}
	}

	static class GooeyHandler extends Handler {

		class Messages {
			public static final int MAKE_LOADED_TOAST = 100;
			public static final int MAKE_INSERTED_TOAST = 200;
			public static final int UPDATE_ADAPTER_RESULTS = 300;
			public static final int UPDATE_ADAPTER_RESULTS_NULL_SET = 400;
		}

		WeakReference<DemoActivity> dactivityReference;

		public GooeyHandler(DemoActivity dactivity) {
			dactivityReference = new WeakReference<DemoActivity>(dactivity);
		}

		@Override
		public void handleMessage(Message msg) {

			DemoActivity dactivity = dactivityReference.get();

			switch (msg.what) {

			case Messages.MAKE_LOADED_TOAST:
				dactivity.makeLoadedToast();
				break;
			case Messages.MAKE_INSERTED_TOAST:
				dactivity.makeInsertToast();
				break;
			case Messages.UPDATE_ADAPTER_RESULTS:
				ArrayList<ViewableResult> results = null;
				try {
					results = (ArrayList<ViewableResult>) msg.obj;
				} catch (ClassCastException ignore) {
				}

				dactivity.updateResults(results);
				break;
			case Messages.UPDATE_ADAPTER_RESULTS_NULL_SET:
				dactivity.updateResults(null);
				break;
			}
		}
	}

	void makeInsertToast() {
		Toast.makeText(context, "Index Created!", Toast.LENGTH_LONG).show();
	}

	void makeLoadedToast() {
		Toast.makeText(context, "SRCH2 Service is Ready for Business!",
				Toast.LENGTH_LONG).show();
	}

	void updateResults(ArrayList<ViewableResult> newResults) {
		if (newResults == null) {
			searchResultsListViewAdapter.clearSet();
		} else {
			searchResultsListViewAdapter.updateSet(newResults);
		}
	}

	@Override
	public void onNewSearchInput(String newSearchText) {
		Log.d("srch2:: " + TAG, "onNewSearchInput input is " + newSearchText);
		SRCH2Engine.searchAllIndexes(newSearchText);
	}

	@Override
	public void onNewSearchInputIsBlank() {
		Log.d("srch2:: " + TAG, "onNewSearchInput input is blank");
		searchResultsListViewAdapter.clearSet();
	}

	public static class SearchResults implements SearchResultsListener {

		public WeakReference<DemoActivity> demoActivityReference;

		public SearchResults(DemoActivity dactivity) {
			demoActivityReference = new WeakReference<DemoActivity>(dactivity);
		}

		@Override
		public void onNewSearchResults(int httpResponseCode,
				String jsonResultsLiteral,
				HashMap<String, ArrayList<JSONObject>> resultRecordMap) {

			final ArrayList<JSONObject> contacts = resultRecordMap
					.get("contacts");
			if (contacts != null) {

				final DemoActivity dactivity = demoActivityReference.get();

				if (contacts.size() > 0) {
					ArrayList<ViewableResult> cvrs = new ArrayList<ViewableResult>();

					for (JSONObject jo : contacts) {
						ViewableResult r = ContactViewableResult.factory
								.wrap(jo);
						cvrs.add(r);
					}

					if (dactivity != null) {
						dactivity.uiHandler.sendMessage(Message.obtain(
								dactivity.uiHandler,
								GooeyHandler.Messages.UPDATE_ADAPTER_RESULTS,
								cvrs));
					}

				} else {

					if (dactivity != null) {
						dactivity.uiHandler
								.sendMessage(Message
										.obtain(dactivity.uiHandler,
												GooeyHandler.Messages.UPDATE_ADAPTER_RESULTS_NULL_SET));
					}
				}
			}
		}
	}

	public static class IndexControl implements ControlResponseListener {

		public WeakReference<DemoActivity> demoActivityReference;

		public IndexControl(DemoActivity dactivity) {
			demoActivityReference = new WeakReference<DemoActivity>(dactivity);
		}

		@Override
		public void onInfoRequestComplete(String targetIndexName,
				InfoResponse theReturnedInfoResponse) {
			Log.d("srch2:: " + TAG, "onInfoRequestComplete responseCode "
					+ theReturnedInfoResponse.httpResponseCode + " "
					+ theReturnedInfoResponse.restfulResponseLiteral);

			Log.d("srch2:: " + TAG, "onInfoRequestComplete core "
					+ targetIndexName + " info response "
					+ theReturnedInfoResponse.toHumanReadableString());

			if (targetIndexName.equals("contacts")
					&& theReturnedInfoResponse.isValidInfoResponse) {

				DemoActivity dactivity = demoActivityReference.get();

			}

		}

		@Override
		public void onInsertRequestComplete(String targetIndexName,
				InsertResponse theReturnedInsertResponse) {
			Log.d("srch2:: " + TAG, "onInsertRequestComplete responseCode "
					+ theReturnedInsertResponse.httpResponseCode + " "
					+ theReturnedInsertResponse.restfulResponseLiteral);

			DemoActivity dactivity = demoActivityReference.get();
			if (dactivity != null) {
				dactivity.uiHandler.sendMessage(Message.obtain(
						dactivity.uiHandler,
						GooeyHandler.Messages.MAKE_INSERTED_TOAST));
			}
		}

		@Override
		public void onUpdateRequestComplete(String targetIndexName,
				UpdateResponse theReturnedUpdateResponse) {
			Log.d("srch2:: " + TAG, "onUpdateRequestComplete responseCode "
					+ theReturnedUpdateResponse.httpResponseCode + " "
					+ theReturnedUpdateResponse.restfulResponseLiteral);

		}

		@Override
		public void onDeleteRequestComplete(String targetIndexName,
				DeleteResponse theReturnedDeleteResponse) {
			Log.d("srch2:: " + TAG, "onDeleteRequestComplete responseCode "
					+ theReturnedDeleteResponse.httpResponseCode + " "
					+ theReturnedDeleteResponse.restfulResponseLiteral);

		}

		@Override
		public void onSRCH2ServiceReady(
				HashMap<String, InfoResponse> indexesToInfoResponseMap) {
			Log.d("srch2:: " + TAG, "onSRCH2ServiceReady");

			if (indexesToInfoResponseMap != null) {

				Log.d("srch2:: " + TAG,
						"onSRCH2ServiceReady size of infoResponseMap "
								+ indexesToInfoResponseMap.size());

				InfoResponse contactInfoResponse = indexesToInfoResponseMap
						.get("contacts");
				DemoActivity dactivity = demoActivityReference.get();

				if (dactivity != null) {
					dactivity.uiHandler.sendMessage(Message.obtain(
							dactivity.uiHandler,
							GooeyHandler.Messages.MAKE_LOADED_TOAST));
				}

				if (contactInfoResponse.isValidInfoResponse) {

					if (contactInfoResponse.numberOfDocumentsInTheIndex > 0) {
						Log.d("srch2:: " + TAG,
								"onSRCH2ServiceReady size of docs greater than 0");

						if (dactivity.contactsIndexWrapper.incrementalState
								.size() == 0) {

							ArrayList<IncrementalValuePair> savedSnapshot = null;
							IncrementalDatabase id = new IncrementalDatabase(
									dactivity.context);
							savedSnapshot = id
									.deserializeIncrementalState("contacts");
							if (savedSnapshot != null) {
								Log.d("srch2:: " + TAG,
										"savedSnapshot size is "
												+ savedSnapshot.size());

								dactivity.contactsIndexWrapper
										.setDeserializedSnapshot(savedSnapshot);

							}
						}

						dactivity.contactsIndexWrapper
								.updateIndex(dactivity.context);

					} else {

						dactivity.contactsIndexWrapper
								.buildIndex(dactivity.context);
					}

				}

			} else {
				Log.d("srch2:: " + TAG,
						"onSRCH2ServiceReady INDEX RESPONSE MAP was null!!!");
			}
		}

		@Override
		public void onGetRecordByIDComplete(String targetIndexName,
				GetRecordResponse theReturnedDocResponse) {
			Log.d("srch2:: " + TAG, "onGetRecordByIDComplete response code "
					+ theReturnedDocResponse.httpResponseCode + " "
					+ theReturnedDocResponse.restfulResponseLiteral);
		}

	}

	public void doSerialize(View v) {
		SRCH2Engine.doSerialize();
	}
}
