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

    CheckCoresLoadedTask(HashMap<String, URL> theTargetCoreUrls) {
        targetCoreUrlsMap = theTargetCoreUrls;
    }

    @Override
    public void run() {
        Thread.currentThread().setName("CHECK CORES LOADED");
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
        while (true) {
            if (superCount > 10) {
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
                Indexable idx = SRCH2Engine.conf.indexableMap.get(indexName);
                if (idx != null) {
                    idx.setRecordCount(iir.numberOfDocumentsInTheIndex);
                    validIndexes.add(idx.getIndexName());
                }
                ++pingCountSuccess;
            } else {
                Cat.d(TAG, "@ iteration " + i + " was UNARY_NOT valid info response ");
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
            for (String indexName : validIndexes) {
                HttpTask.executeTask(new IndexIsReadyResponse(indexName));
            }
            SRCH2Engine.startHeartBeatPing();
            SRCH2Engine.reQueryLastOne();
        }
    }
}
