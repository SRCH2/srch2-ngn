package com.srch2.android.sdk;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;

class DeleteTask extends HttpTask.ControlHttpTask {
    private static final String TAG = "DeleteTask";

    DeleteTask(URL insertUrl, String theTargetCoreName, StateResponseListener theControlResponseListener) {
        super(insertUrl, theTargetCoreName, theControlResponseListener);
    }

    @Override
    public void run() {
        doDelete();
    }

    private void doDelete() {
        HttpURLConnection connection = null;

        int responseCode = -1;
        String response = null;

        Cat.d(TAG, "targeturl for DELETION is " + targetUrl);

        try {
            connection = (HttpURLConnection) targetUrl.openConnection();
            connection.setRequestMethod("DELETE");
            connection.setUseCaches(false);
            connection.connect();

            responseCode = connection.getResponseCode();
            response = handleStreams(connection, TAG);
        } catch (IOException e) {
            response = handleIOExceptionMessagePassing(e, response, TAG);
        } finally {
            if (connection != null) {
                connection.disconnect();
            }
        }

        if (response == null) {
            response = RestfulResponse.IRRECOVERABLE_NETWORK_ERROR_MESSAGE;
        }

        onTaskComplete(responseCode, response);
    }


    @Override
    protected void onTaskComplete(int returnedResponseCode, String returnedResponseLiteral) {

        if (controlResponseObserver != null) {
            DeleteResponse deleteResponse;
            if (returnedResponseLiteral == null || returnedResponseLiteral.equals(RestfulResponse.IRRECOVERABLE_NETWORK_ERROR_MESSAGE)) {
                deleteResponse = new DeleteResponse(returnedResponseCode, RestfulResponse.IRRECOVERABLE_NETWORK_ERROR_MESSAGE);
            } else {
                deleteResponse = new DeleteResponse(returnedResponseCode, returnedResponseLiteral);
            }
            controlResponseObserver.onDeleteRequestComplete(targetCoreName, deleteResponse);
        }
        super.onTaskComplete(returnedResponseCode, returnedResponseLiteral);
    }
}