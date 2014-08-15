package com.srch2.android.sdk;

import java.net.URL;

class GetRecordTask extends SearchTask {


    GetRecordTask(URL url, String nameOfTheSingleCoreToQuery,
                  StateResponseListener controlResultsListener) {
        super(url, nameOfTheSingleCoreToQuery, null);
        super.controlResponseObserver = controlResultsListener;
    }

    @Override
    protected void onTaskComplete(int returnedResponseCode,
                                  String returnedResponseLiteral) {
        if (super.controlResponseObserver != null) {
            GetRecordResponse getRecordResponse;
            if (returnedResponseLiteral == null || returnedResponseLiteral.equals(RestfulResponse.IRRECOVERABLE_NETWORK_ERROR_MESSAGE)) {
                getRecordResponse = new GetRecordResponse(returnedResponseCode, RestfulResponse.IRRECOVERABLE_NETWORK_ERROR_MESSAGE, super.targetCoreName);
            } else {
                getRecordResponse = new GetRecordResponse(returnedResponseCode, returnedResponseLiteral, super.targetCoreName);
            }
            this.controlResponseObserver.onGetRecordByIDComplete(super.targetCoreName, getRecordResponse);
        }
    }
}
