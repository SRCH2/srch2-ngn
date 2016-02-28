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
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;

final class MultiSaveTask extends HttpTask {

    private static final String TAG = "MultiSaveTask";


    ArrayList<Indexable> indexesToSave;

    MultiSaveTask(ArrayList<Indexable> dirtyIndexes) {
        indexesToSave = dirtyIndexes;
        if (indexesToSave == null) {
            indexesToSave = new ArrayList<Indexable>(0);
        }
    }

    @Override
    public void run() {
        Cat.d(TAG, "starting to save");
        if (indexesToSave != null && indexesToSave.size() > 0) {
            for (Indexable idx : indexesToSave) {
                Cat.d(TAG, "doing save for index " + idx.getIndexName());
                boolean success = doSave(UrlBuilder.getSaveUrl(SRCH2Engine.getConfig(), idx.indexInternal.indexDescription));
                if (success) {
                    Cat.d(TAG, "save was successful for index " + idx.getIndexName() + " reseting isDirty");
                    idx.indexInternal.isDirty.set(false);
                }
            }
        }
    }

    private boolean doSave(URL saveUrl) {
        URL url = saveUrl;
        HttpURLConnection connection = null; // can probably reuse by pulling out before for loop in doInBackground
        boolean success = true;
        String response = "";
        try {
            Cat.d(TAG, "doing save task for " + saveUrl);

            connection = (HttpURLConnection) url.openConnection();
            connection.setDoOutput(true);
            connection.setConnectTimeout(3000);
            connection.setRequestMethod("PUT");
            connection.setUseCaches(false);
            connection.connect();

            int responseCode = connection.getResponseCode();
            success = (int) (responseCode / 100) == 2;

            Cat.d(TAG, "response code was " + responseCode + " and success was " + success);

            response = handleStreams(connection, TAG);

        } catch (IOException e) {
            success = false;
            response = handleIOExceptionMessagePassing(e, response, TAG);
        } finally {
            if (connection != null) { connection.disconnect(); }
        }

        Cat.d(TAG, "save task complete response was " + response);
        return success;
    }
}
