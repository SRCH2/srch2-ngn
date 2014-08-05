package com.srch2.android.http.service;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;

/**
 * Created by ashton on 7/29/2014.
 */
public class LifeCycleTestActivity extends Activity {
    public final static String TAG = "LifeCycleTestActivity";

    Context context;

    TestIndex testIndex;
    public TestControlResponseListener control;
    public TestSearchResultsListener search;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setTitle(TAG);
        context = this;
        Log.d(TAG, "onCreate");

        control = new TestControlResponseListener();
        search = new TestSearchResultsListener();

        testIndex = new TestIndex();
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

    public void initializeSRCH2Engine(boolean debugModeEnabled) {
        Log.d(TAG, "initializeSRCH2Engine");
        SRCH2Engine.initialize(testIndex);
        SRCH2Engine.setTestAndDebugMode(debugModeEnabled);
        SRCH2Engine.setControlResponseListener(control);
        SRCH2Engine.setSearchResultsListener(search);
    }

    public void startSRCH2Engine() {
        Log.d(TAG, "startSRCH2Engine");
        SRCH2Engine.onStart(context);
    }

    public void stopSRCH2Engine() {
        Log.d(TAG, "stopSRCH2Engine");
        SRCH2Engine.onStop(context);
    }

    public boolean getSRCH2EngineIsReady() {
        boolean isReady = SRCH2Engine.isReady();
        return isReady;
    }

    public void insertTenRecordsStartingAtPrimaryKeyEqualTo1() {
        Log.d(TAG, "insertingTenRecords");
        testIndex.insert(testIndex.getRecordsArray(10, 1));
    }

    public boolean verifyTenRecordsHaveBeenInserted() {
        return control.insertResponse != null ? control.insertResponse.getSuccessCount() == 10 : false;
    }

    public void deleteTenRecordsFromPrimaryKeyOneToTen() {
        for (int i = 1; i < 11; ++i) {
            testIndex.delete(String.valueOf(i));
        }

    }


    public boolean verifyZeroRecordsInInfoResponse() {
        return control.infoResponse != null ? control.infoResponse.getNumberOfDocumentsInTheIndex() == 0 : false;
    }

    public boolean verifyTenRecordsInInfoResponse() {
        return control.infoResponse != null ? control.infoResponse.getNumberOfDocumentsInTheIndex() == 10 : false;
    }


    public void doNullSearch() {
        try {
            testIndex.search("xxxxxxxxxxxxxxxxxxx");
        } catch (Exception e) { }
    }

    public void doValidSearch(String searchInput) {
        try {
            testIndex.search(searchInput);
        } catch (Exception e) { }
    }

    public int getPort() {
        return SRCH2Engine.conf.getPort();
    }

    public void doTestIndexInfo() {
        testIndex.info();
    }
}
