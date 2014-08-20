package com.srch2.android.sdk;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;

class InternalInfoTask {

    private static final String TAG = "InteralInfoTask";

    /**
     * Has the <b>constant</b> value of 1000 milliseconds, or 1 second.
     */
    private static final int DEFAULT_CONNECTION_TIMEOUT_MS = 1000;
    private final int connectionTimeOutInMilliseconds;

    private URL targetUrl;

    boolean ignoreIOException = false;

    InternalInfoTask(URL theTargetUrl) {
        targetUrl = theTargetUrl;
        connectionTimeOutInMilliseconds = DEFAULT_CONNECTION_TIMEOUT_MS;
    }

    InternalInfoTask(URL theTargetUrl, int theConnectionTimeOutInMilliseconds) {
        targetUrl = theTargetUrl;
        connectionTimeOutInMilliseconds = (theConnectionTimeOutInMilliseconds < 1) ? DEFAULT_CONNECTION_TIMEOUT_MS
                : theConnectionTimeOutInMilliseconds;
    }


    InternalInfoTask(URL theTargetUrl, int theConnectionTimeOutInMilliseconds, boolean isCheckCoresLoadedTask) {
        ignoreIOException = isCheckCoresLoadedTask;
        targetUrl = theTargetUrl;
        connectionTimeOutInMilliseconds = (theConnectionTimeOutInMilliseconds < 1) ? DEFAULT_CONNECTION_TIMEOUT_MS
                : theConnectionTimeOutInMilliseconds;
    }

    public InternalInfoResponse getInfo() {
        InputStream is = null;
        HttpURLConnection connection = null;

        int responseCode = HttpTask.RESTfulResponseTags.FAILED_TO_CONNECT_RESPONSE_CODE;
        String response = null;
        try {
            connection = (HttpURLConnection) targetUrl.openConnection();
            connection.setReadTimeout(connectionTimeOutInMilliseconds);
            connection.setConnectTimeout(connectionTimeOutInMilliseconds);
            connection.setDoInput(true);
            connection.connect();

            responseCode = connection.getResponseCode();
            response = HttpTask.handleStreams(connection, TAG);
        } catch (IOException networkError) {
            if (!ignoreIOException) {
                response = HttpTask.handleIOExceptionMessagePassing(networkError, response, TAG);
            }
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
            response = Indexable.IRRECOVERABLE_NETWORK_ERROR_MESSAGE;
        }

        return new InternalInfoResponse(responseCode, response);
    }
}
