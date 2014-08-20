package com.srch2.android.sdk.sandbox;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import com.srch2.android.sdk.*;
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

    SRCH2Results results;
    Index index;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_my);

        results = new SRCH2Results();
        index = new Index();

        SRCH2Engine.initialize(index);
        SRCH2Engine.setSearchResultsListener(results);
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

        @Override
        public void onIndexReady() {
            super.onIndexReady();
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
}
