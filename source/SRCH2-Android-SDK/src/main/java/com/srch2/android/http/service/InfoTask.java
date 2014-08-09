package com.srch2.android.http.service;

import android.util.Log;
import com.srch2.android.http.service.HttpTask.ControlHttpTask;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;

/**
 * An <code>InfoTask</code> performs the control restful request of doing an
 * /info on a core. The core is specified by the caller in passing the fully
 * formed URL to the constructor for this class.
 */
class InfoTask extends ControlHttpTask {

    private static final String TAG = "srch2:: InfoTask";

    /**
     * Has the <b>constant</b> value of 1000 milliseconds, or 1 second.
     */
    private static final int DEFAULT_CONNECTION_TIMEOUT_MS = 1000;

    private final int connectionTimeOutInMilliseconds;

    /**
     * Initializes an InfoTask for returning an <code>InfoRestfulRespone</code>
     * object. Callers must provide a non-null URL for <code>theTargetUrl</code>
     * or a null pointer exception will be thrown. This constructor will set the
     * connection time out to the value of
     * <code>InfoTask.DEFAULT_CONNECTION_TIMEOUT_MS</code> which is 1000 ms or 1
     * second.
     *
     * @param theTargetUrl                        the fully formed URL specifying the core to perform a restful
     *                                            /info on
     * @param theControlResponseListener          the control response listener implemented by the user
     */
    InfoTask(URL theTargetUrl, String theTargetCoreName, StateResponseListener theControlResponseListener) {
        super(theTargetUrl, theTargetCoreName, theControlResponseListener);
        connectionTimeOutInMilliseconds = DEFAULT_CONNECTION_TIMEOUT_MS;
    }

    /**
     * Initializes an InfoTask for returning an <code>InfoRestfulRespone</code>
     * object. Callers must provide a non-null URL for <code>theTargetUrl</code>
     * or a null pointer exception will be thrown. This constructor will set the
     * connection time out to the value of
     * <code>theConnectionTimeOutInMilliseconds</code>; if this value is 0 or
     * negative, the connection time out value will be set to
     * <code>InfoTask.DEFAULT_CONNECTION_TIMEOUT_MS</code>.
     *
     * @param theTargetUrl                        the fully formed URL specifying the core to perform a restful
     *                                            /info on
     * @param theControlResponseListener the control response listener implemented by the user
     * @param theConnectionTimeOutInMilliseconds  the time in milliseconds to time out in
     */
    InfoTask(URL theTargetUrl, String theTargetCoreName, StateResponseListener theControlResponseListener, int theConnectionTimeOutInMilliseconds) {
        super(theTargetUrl, theTargetCoreName, theControlResponseListener);
        connectionTimeOutInMilliseconds = (theConnectionTimeOutInMilliseconds < 1) ? DEFAULT_CONNECTION_TIMEOUT_MS
                : theConnectionTimeOutInMilliseconds;
    }

    @Override
    public void run() {
        InputStream is = null;
        HttpURLConnection connection = null;

        int responseCode = RestfulResponse.FAILED_TO_CONNECT_RESPONSE_CODE;
        String response = null;
        try {
            connection = (HttpURLConnection) targetUrl.openConnection();
            connection.setReadTimeout(connectionTimeOutInMilliseconds);
            connection.setConnectTimeout(connectionTimeOutInMilliseconds);
            connection.setDoInput(true);
            connection.connect();

            responseCode = connection.getResponseCode();
            response = handleStreams(connection, TAG);
        } catch (IOException networkError) {
            handleIOExceptionMessagePassing(networkError, response, TAG);
        } finally {
            if (connection != null) {
                connection.disconnect();
            }
            if (is != null) {
                try {
                    is.close();
                } catch (IOException ignore) {
                }
            }
        }

        if (response == null) {
            response = RestfulResponse.IRRECOVERABLE_NETWORK_ERROR_MESSAGE;
        }
        onTaskComplete(responseCode, response);
    }

    @Override
    protected void onTaskComplete(int returnedResponseCode,
                                  String returnedResponseLiteral) {
        if (controlResponseObserver != null) {
            InfoResponse infoResponse;
            if (returnedResponseLiteral == null || returnedResponseLiteral.equals(RestfulResponse.IRRECOVERABLE_NETWORK_ERROR_MESSAGE)) {
                Log.d(TAG, "failed to do the thing it was supposed to do!");

                infoResponse = new InfoResponse(
                        returnedResponseCode,
                        RestfulResponse.IRRECOVERABLE_NETWORK_ERROR_MESSAGE);
            } else {
                Log.d(TAG, "doing the thing it was supposed to do!");
                infoResponse = new InfoResponse(returnedResponseCode,
                        returnedResponseLiteral);
            }
            controlResponseObserver.onInfoRequestComplete(targetCoreName, infoResponse);
        }
    }
}
