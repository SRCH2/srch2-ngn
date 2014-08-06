package com.srch2.android.http.service;

import android.util.Log;
import com.srch2.android.http.service.*;

import java.util.HashMap;

/**
 * Created by ashton on 7/29/2014.
 */
public class TestControlResponseListener  implements StateResponseListener {

    public static final String TAG = "TestControlResponseListener";

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