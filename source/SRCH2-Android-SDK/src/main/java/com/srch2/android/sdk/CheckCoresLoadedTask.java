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

        Log.d(TAG, "CHECK CORE LOADED TASK pingCountSuccess is " + pingCountSuccess + " and coreCount is " + coreCount);

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
