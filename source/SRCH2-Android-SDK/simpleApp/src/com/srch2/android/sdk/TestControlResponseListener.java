package com.srch2.android.sdk;

import android.util.Log;
import com.srch2.android.sdk.*;

/**
 * Created by ashton on 7/29/2014.
 */
public class TestControlResponseListener implements StateResponseListener {

    public static final String TAG = "TestControlResponseListener";

    public InsertResponse insertResponse;
    public UpdateResponse updateResponse;
    public DeleteResponse deleteResponse;
    public GetRecordResponse recordResponse;

    public void reset() {
        insertResponse = null;
        updateResponse = null;
        deleteResponse = null;
        recordResponse = null;
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
    public void onSRCH2ServiceReady() {
        Log.d(TAG, "APP get ServiceReady");
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