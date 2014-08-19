package com.srch2.android.sdk;

import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;


final class CheckCoresLoadedTask extends HttpTask {

    private static final String TAG = "CheckCoresLoadedTask";


    private static final int PING_RECONNECTION_TIME_MS = 250;

    private final HashMap<String, URL> targetCoreUrlsMap;

    private boolean noNetworkConnection = false;

    CheckCoresLoadedTask(HashMap<String, URL> theTargetCoreUrls,
                         StateResponseListener theControlResponseListener) {
        targetCoreUrlsMap = theTargetCoreUrls;
        controlResponseObserver = theControlResponseListener;
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
        int pingCountSuccess = 0;
        int i = 0;
        int superCount = 0;
        while (true) {
            if (superCount > 10) {
                noNetworkConnection = true;
                break;
            }
            String indexName = indexNames.get(i);
            URL targetUrl = targetCoreUrlsMap.get(indexName);

            Cat.d(TAG, "run - core check loop @ iteration " + i + " targetingUrl " + targetUrl);

            InternalInfoTask iit = new InternalInfoTask(targetUrl);
            InternalInfoResponse iir = iit.getInfo();

            if (iir.isValidInfoResponse) {
                Cat.d(TAG, "@ iteration " + i + " was valid info response ");
                Indexable idx = SRCH2Engine.conf.indexableMap.get(indexName);
                idx.setRecordCount(iir.numberOfDocumentsInTheIndex);
                ++pingCountSuccess;
            } else {
                Cat.d(TAG, "@ iteration " + i + " was NOT valid info response ");
            }

            i = ++i % coreCount;

            if (pingCountSuccess == coreCount || noNetworkConnection) {
                Cat.d(TAG, "run - breaking from loop because: pingCount " + pingCountSuccess + " with coreCount " + coreCount + " and noNetworkConnection: " + noNetworkConnection);
                break;
            }
            ++superCount;
        }

        if (controlResponseObserver != null && !noNetworkConnection) {
            if (pingCountSuccess == coreCount) {
                Cat.d(TAG, "run - successful requerying etc");
                SRCH2Engine.isReady.set(true);
                controlResponseObserver.onSRCH2ServiceReady();
                SRCH2Engine.reQueryLastOne();
            }
            Cat.d(TAG, "run - notifying observer");
        }

        Thread.currentThread().setName("CHECK CORES LOADED FIN");
    }

    @Override
    protected void onTaskComplete(int returnedResponseCode,
                                  String returnedResponseLiteral) {
        // do nothing here
    }

}
