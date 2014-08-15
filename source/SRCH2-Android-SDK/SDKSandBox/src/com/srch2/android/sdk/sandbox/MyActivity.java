package com.srch2.android.sdk.sandbox;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import com.srch2.android.http.service.*;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;

public class MyActivity extends Activity implements InstantSearchEditText.SearchInputEnteredObserver {



    @Override
    public void onNewSearchInput(String newSearchText) {
        SRCH2Engine.searchAllIndexes(newSearchText);
    }

    @Override
    public void onNewSearchInputIsBlank() {

    }

    SRCH2State state;
    SRCH2Results results;
    Index index;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_my);

        state = new SRCH2State();
        results = new SRCH2Results();
        index = new Index();

        SRCH2Engine.initialize(index);
        SRCH2Engine.setSearchResultsListener(results);
        SRCH2Engine.setStateResponseListener(state);
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

    class Index extends Indexable {
        public static final String INDEX_NAME = "name";
        public static final String INDEX_FIELD_NAME_PRIMARY_KEY = "id";
        public static final String INDEX_FIELD_NAME_TITLE = "title";

        @Override
        public String getIndexName() {
            return INDEX_NAME;
        }

        @Override
        public Schema getSchema() {
            PrimaryKeyField pk = Field.createDefaultPrimaryKeyField(INDEX_FIELD_NAME_PRIMARY_KEY);
            Field title = Field.createSearchableField(INDEX_FIELD_NAME_TITLE);
            return Schema.createSchema(pk, title);
        }

        final public JSONArray getRecords() {

            JSONArray records = new JSONArray();
            try {

                JSONObject o1 = new JSONObject();
                o1.put(INDEX_FIELD_NAME_PRIMARY_KEY, "1");
                o1.put(INDEX_FIELD_NAME_TITLE, "titleOne 1");
                records.put(o1);
            } catch (JSONException ee) {

            }
            return records;
        }
    }


    class SRCH2Results implements SearchResultsListener {
        private static final String TAG = "s2test:: SRCH2Results";


        @Override
        public void onNewSearchResults(int i, String s, HashMap<String, ArrayList<JSONObject>> stringArrayListHashMap) {
            Log.d(TAG, "onNewSearchResults");
            Log.d(TAG, "onNewSearchResults " + s);
        }
    }

    class SRCH2State implements StateResponseListener {

        private static final String TAG = "s2test:: SRCH2State";

        @Override
        public void onInfoRequestComplete(String s, InfoResponse infoResponse) {
            Log.d(TAG, "onInfo");

        }

        @Override
        public void onInsertRequestComplete(String s, InsertResponse insertResponse) {
            Log.d(TAG, "onInsertRequestComplete");
            Log.d(TAG, "onInsertRequestComplete - " + insertResponse.toString());
        }

        @Override
        public void onUpdateRequestComplete(String s, UpdateResponse updateResponse) {
            Log.d(TAG, "onDeleteRequestComplete");

        }

        @Override
        public void onSRCH2ServiceReady() {
            Log.d(TAG, "onSRCH2ServiceReady");
            Indexable i = SRCH2Engine.getIndex(Index.INDEX_NAME);
            Log.d(TAG, "onSRCH2ServiceReady - " + Index.INDEX_NAME + " recordCount: " + i.getRecordCount());

            if (i.getRecordCount() == 0) {
                Log.d(TAG, "onSRCH2ServiceReady - inserting a record");
                i.insert(index.getRecords());
            }
        }

        @Override
        public void onDeleteRequestComplete(String s, DeleteResponse deleteResponse) {
            Log.d(TAG, "onDeleteRequestComplete");

        }

        @Override
        public void onGetRecordByIDComplete(String s, GetRecordResponse getRecordResponse) {
            Log.d(TAG, "onDeleteRequestComplete");

        }
    }




}
