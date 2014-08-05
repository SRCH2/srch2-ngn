package com.srch2.android.http.service;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.HashMap;

public class MyActivity extends Activity {
    public static final String TAG = "srch2:: MyActivity";

    public SRCH2ResultsListener mResultListener = new SRCH2ResultsListener();
    public SRCH2ControlListener mControlListener = new SRCH2ControlListener();

    public MusicIndex mMusicIndex = new MusicIndex();
    public MovieIndex mMovieIndex = new MovieIndex();
    public GeoIndex mGeoIndex = new GeoIndex();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setTitle(TAG);

        Log.d(TAG, "onCreate");
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume");
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG, "onPause");
    }

    @Override
    protected void onStart() {
        super.onStart();
        Log.d(TAG, "onStart");
    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.d(TAG, "onStop");
    }

    @Override
    protected void onRestart() {
        super.onRestart();
        Log.d(TAG, "onRestart");
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy");
    }

    public void initializeSRCH2EngineAndCallStart() {
        initializeSRCH2Engine();
        try {
            Thread.currentThread().sleep(300);
        } catch (InterruptedException e) {

        }
        callSRCH2EngineStart();
    }

    public void initializeSRCH2Engine() {
        SRCH2Engine.initialize(mMusicIndex, mMovieIndex, mGeoIndex);
        SRCH2Engine.setSearchResultsListener(mResultListener);
        SRCH2Engine.setStateResponseListener(mControlListener);
        SRCH2Engine.setTestAndDebugMode(true);
    }

    public void callSRCH2EngineStart() {
        SRCH2Engine.onStart(this);
    }

    public void callSRCH2EngineStop() {
        SRCH2Engine.onStop(this);
    }

    public boolean doInfoTest() {
        if (mMusicIndex != null) {
            mMusicIndex.info();
            return true;
        }
        return false;
    }

    private void reset() {
        mControlListener.reset();
        mResultListener.reset();
    }

    public InfoResponse getInfoResponse(){
        Util.waitForResponse(this, InfoResponse.class);
        return mControlListener.infoResponse;
    }

    public InsertResponse getInsertResponse() {
        Util.waitForResponse(this, InsertResponse.class);
        return mControlListener.insertResponse;
    }

    public DeleteResponse getDeleteResponse() {
        Util.waitForResponse(this, DeleteResponse.class);
        return mControlListener.deleteResponse;
    }

    public UpdateResponse getUpdateResponse() {
        Util.waitForResponse(this, UpdateResponse.class);
        return mControlListener.updateResponse;
    }

    public GetRecordResponse getRecordResponse() {
        Util.waitForResponse(this, GetRecordResponse.class);
        return mControlListener.recordResponse;
    }

    public SRCH2ResultsListener getSearchResult() {
        Util.waitForResultResponse(this);
        return mResultListener;
    }

    public void waitForEngineReady() {
        Util.waitSRCH2EngineIsReady();
    }


    public static class MusicIndex extends Indexable {
        public static final String INDEX_NAME = "music";
        public static final String INDEX_KEY_PRIMARY_KEY = "idx";
        public static final String INDEX_KEY_SONG_TITLE = "song";
        public static final String INDEX_KEY_SONG_YEAR = "year";
        public static final String INDEX_KEY_ARTIST = "artist";
        public static final String INDEX_KEY_GENRE = "genre";

        @Override
        public IndexDescription getIndexDescription() {


            Field primaryKey = Field.getRefiningField(INDEX_KEY_PRIMARY_KEY, Field.Type.TEXT);
            Field songTitle = Field.getSearchableField(INDEX_KEY_SONG_TITLE, 5);
//            Field songYear = Field.getRefiningField(INDEX_KEY_SONG_YEAR, Field.Type.INTEGER);
            //Field artist = Field.getSearchableField(INDEX_KEY_ARTIST, 4);
            Field genre = Field.getSearchableField(INDEX_KEY_GENRE, 3);
            return new IndexDescription(INDEX_NAME, primaryKey, songTitle, genre);//songYear, artist, genre);
        }


    }

    public static class MovieIndex extends Indexable {
        public static final String INDEX_NAME = "movies";

        public static final String INDEX_KEY_PRIMARY_KEY = "id";
        public static final String INDEX_KEY_TITLE = "title";
        public static final String INDEX_KEY_YEAR = "year";
        public static final String INDEX_KEY_GENRE = "genre";

        @Override
        public IndexDescription getIndexDescription() {
            Field primaryKey = Field.getRefiningField(INDEX_KEY_PRIMARY_KEY, Field.Type.INTEGER);
            Field title = Field.getSearchableField(INDEX_KEY_TITLE);

            Field genre = Field.getSearchableField(INDEX_KEY_GENRE);
            Field year = Field.getRefiningField(INDEX_KEY_YEAR, Field.Type.INTEGER);
            return new IndexDescription(INDEX_NAME, primaryKey, title,  genre, year);//, year, genre);
        }


    }

    public static class GeoIndex extends Indexable {
        public static final String INDEX_NAME = "geo";
        public static final String INDEX_KEY_PRIMARY_KEY = "title";
        public static final String LAT = "lat";
        public static final String LON = "long";

        public static final Field fieldID = Field.getSearchableField(INDEX_KEY_PRIMARY_KEY);

        @Override
        public IndexDescription getIndexDescription() {
            return new IndexDescription(INDEX_NAME,
                    fieldID, LAT, LON);
        }


    }

    public class SRCH2ResultsListener implements SearchResultsListener {

        public int httpResponseCode;
        public String jsonResultsLiteral;
        public HashMap<String, ArrayList<JSONObject>> resultRecordMap;

        public void reset() {
            httpResponseCode = 0;
            jsonResultsLiteral = null;
            resultRecordMap = null;
        }

        @Override
        public void onNewSearchResults(int httpResponseCode,
                                       String jsonResultsLiteral,
                                       HashMap<String, ArrayList<JSONObject>> resultRecordMap) {
            this.httpResponseCode = httpResponseCode;
            this.jsonResultsLiteral = jsonResultsLiteral;
            this.resultRecordMap = resultRecordMap;
        }

    }

    public class SRCH2ControlListener implements StateResponseListener {

        public InfoResponse infoResponse;
        public InsertResponse insertResponse;
        public UpdateResponse updateResponse;
        public HashMap<String, InfoResponse> indexesInfoResponseMap;
        public DeleteResponse deleteResponse;
        public GetRecordResponse recordResponse;

        public void reset() {
            infoResponse = null;
            insertResponse = null;
            updateResponse = null;
            indexesInfoResponseMap = null;
            deleteResponse = null;
            recordResponse = null;
        }

        @Override
        public void onInfoRequestComplete(String targetIndexName,
                                          InfoResponse theReturnedInfoResponse) {
            infoResponse = theReturnedInfoResponse;

        }

        @Override
        public void onInsertRequestComplete(String targetIndexName,
                                            InsertResponse theReturnedInsertResponse) {
            Log.d(TAG, "APP get insertResponse:" + theReturnedInsertResponse.toString());
            Log.d(TAG, ((Object) this).toString());
            insertResponse = theReturnedInsertResponse;
        }

        @Override
        public void onUpdateRequestComplete(String targetIndexName,
                                            UpdateResponse theReturnedUpdateResponse) {
            Log.d(TAG, "APP get updateResponse:" + theReturnedUpdateResponse.toString());
            updateResponse = theReturnedUpdateResponse;
        }

        @Override
        public void onSRCH2ServiceReady(
                HashMap<String, InfoResponse> indexesToInfoResponseMap) {
            Log.d(TAG, "APP get ServiceReady:" + indexesToInfoResponseMap.values());
            indexesInfoResponseMap = indexesToInfoResponseMap;
        }

        @Override
        public void onDeleteRequestComplete(String targetIndexName,
                                            DeleteResponse theReturnedDeleteResponse) {
            Log.d(TAG, "APP get deleteResponse:" + theReturnedDeleteResponse.toString());
            deleteResponse = theReturnedDeleteResponse;
        }

        @Override
        public void onGetRecordByIDComplete(String targetIndexName,
                                            GetRecordResponse theReturnedDocResponse) {
            Log.d(TAG, "APP get GetRecordByID:" + theReturnedDocResponse.toString());
            recordResponse = theReturnedDocResponse;
        }

    }
}
