/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
package com.srch2.android.sdk;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;

class InternalInfoTask {

    private static final String TAG = "InteralInfoTask";

    static final int SHORT_CONNECTION_TIMEOUT_MS = 250;

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
            response = HttpTask.IRRECOVERABLE_NETWORK_ERROR_MESSAGE;
        }

        return new InternalInfoResponse(responseCode, response);
    }
}
