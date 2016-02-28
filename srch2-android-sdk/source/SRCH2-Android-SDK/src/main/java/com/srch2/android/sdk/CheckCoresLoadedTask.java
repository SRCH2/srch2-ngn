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

import android.util.Log;

import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;


final class CheckCoresLoadedTask extends HttpTask {

    private static final String TAG = "CheckCoresLoadedTask";


    private static final int PING_RECONNECTION_TIME_MS = 250;

    private final HashMap<String, URL> targetCoreUrlsMap;

    private boolean noNetworkConnection = false;

    private boolean isCheckingCoresAfterCrash = false;

    CheckCoresLoadedTask(HashMap<String, URL> theTargetCoreUrls, boolean isCheckingAfterCrash) {
        isCheckingCoresAfterCrash = isCheckingAfterCrash;
        targetCoreUrlsMap = theTargetCoreUrls;
    }


    public void doCheckCoresLoadedTask() {

        Cat.d(TAG, "run start");

        int coreCount = targetCoreUrlsMap.size();
        Cat.d(TAG, "run coreCount " + coreCount);
        ArrayList<String> indexNames = new ArrayList<String>();
        for (String s : targetCoreUrlsMap.keySet()) {
            indexNames.add(s);
        }
        ArrayList<String> validIndexes = new ArrayList<String>();
        int pingCountSuccess = 0;
        int i = 0;
        int superCount = 0;

        int failsInARow = 0;

        while (true) {
            if (failsInARow > 10) {
                try {
                    Thread.currentThread().sleep(100);
                } catch (InterruptedException e) {
                    noNetworkConnection = true;
                    break;
                }
                failsInARow = 0;
            }

            if (superCount > 100) {
                noNetworkConnection = true;
                break;
            }

            String indexName = indexNames.get(i);

            if (validIndexes.contains(indexName)) {
                continue;
            }

            URL targetUrl = targetCoreUrlsMap.get(indexName);

            Cat.d(TAG, "run - core check loop @ iteration " + i + " targetingUrl " + targetUrl);

            InternalInfoTask iit = new InternalInfoTask(targetUrl, PING_RECONNECTION_TIME_MS, true);
            InternalInfoResponse iir = iit.getInfo();

            if (iir.isValidInfoResponse) {
                Cat.d(TAG, "@ iteration " + i + " was valid info response ");
                IndexableCore idx = SRCH2Engine.conf.indexableMap.get(indexName);
                validIndexes.add(idx.getIndexName());
                ++pingCountSuccess;
                if (idx != null && Indexable.class.isAssignableFrom(idx.getClass())) {
                    idx.setRecordCount(iir.numberOfDocumentsInTheIndex);
                }
            } else {
                Cat.d(TAG, "@ iteration " + i + " was not valid info response ");
                ++failsInARow;
            }

            i = ++i % coreCount;

            if (pingCountSuccess == coreCount || noNetworkConnection) {
                Cat.d(TAG, "run - breaking from loop because: pingCount " + pingCountSuccess + " with coreCount " + coreCount + " and noNetworkConnection: " + noNetworkConnection);
                break;
            }
            ++superCount;
        }

        Cat.d(TAG, "CHECK CORE LOADED TASK pingCountSuccess is " + pingCountSuccess + " and coreCount is " + coreCount);

        if (pingCountSuccess == coreCount) {
            SRCH2Engine.isReady.set(true);
            if (!isCheckingCoresAfterCrash) {
                for (String indexName : validIndexes) {
                    HttpTask.executeTask(new IndexIsReadyResponse(indexName));
                }
                SRCH2Engine.reQueryLastOne();
            }
        }
        onExecutionCompleted(TASK_ID_SEARCH);
        onExecutionCompleted(TASK_ID_INSERT_UPDATE_DELETE_GETRECORD);
    }

    @Override
    public void run() {
        Thread.currentThread().setName("CHECK CORES LOADED");
        try {
            doCheckCoresLoadedTask();
        } catch (Exception e) {
            SRCH2Engine.isReady.set(false);
            Cat.ex(TAG, "GeneralException", e);
        }
    }
}
