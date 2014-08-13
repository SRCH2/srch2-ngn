package com.srch2.android.http.service;

import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;


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

        HashMap<String, InfoResponse> responseMap = new HashMap<String, InfoResponse>();

        int i = 0;
        int superCount = 0;
        while (true) {
            if (superCount > 10) {
                noNetworkConnection = true;
                break;
            }
            String indexName = indexNames.get(i);
            URL targetUrl = targetCoreUrlsMap.get(indexName);


            Cat.d(TAG, "run - core check loop @ iteration " + i);
            Cat.d(TAG, "run - core check targeting url " + targetUrl);

            InfoResponse ir = checkIfLoaded(targetUrl);

            if (ir != null) {
                Cat.d(TAG, "run - inforesponse from core " + indexName + " returned positive");


                Log.d(TAG, "targetURL " + targetUrl.toExternalForm() + " connected!");

                Log.d(TAG, "ir literal " + ir.getRESTfulResponseLiteral());

                responseMap.put(indexName, ir);

                ++pingCountSuccess;
            } else {
                Cat.d(TAG, "run - inforesponse from core " + indexName + " returned negative");
            }

            ++i;
            if (i == coreCount) {
                i = 0;
            }

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
                controlResponseObserver.onSRCH2ServiceReady(responseMap);
                SRCH2Engine.reQueryLastOne();
            }
            Cat.d(TAG, "run - notifying observer");
        }

        Thread.currentThread().setName("CHECK CORES LOADED FIN");
    }

    private InfoResponse checkIfLoaded(URL coreUrl) {

        InputStream is = null;
        HttpURLConnection connection = null;
        String infoResponseLiteral = null;
        int responseCode = RestfulResponse.FAILED_TO_CONNECT_RESPONSE_CODE;
        try {
            connection = (HttpURLConnection) coreUrl.openConnection();
            connection.setReadTimeout(PING_RECONNECTION_TIME_MS);
            connection.setConnectTimeout(PING_RECONNECTION_TIME_MS);
            connection.setDoInput(true);
            connection.connect();

            responseCode = connection.getResponseCode();

            if (connection.getInputStream() != null) {
                infoResponseLiteral = readInputStream(connection
                        .getInputStream());
            } else if (connection.getErrorStream() != null) {
                infoResponseLiteral = readInputStream(connection
                        .getErrorStream());
            }

        } catch (IOException networkError) {
            networkError.printStackTrace();
//			noNetworkConnection = true;
            infoResponseLiteral = networkError.getMessage();
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

        InfoResponse ir = new InfoResponse(responseCode, infoResponseLiteral);

        if (responseCode == 200 && !ir.getLastMergeTime().equals(InfoResponse.INVALID_LAST_MERGE_TIME)) {
            return ir;
        } else {
            return null;
        }
    }

    @Override
    protected void onTaskComplete(int returnedResponseCode,
                                  String returnedResponseLiteral) {
        // do nothing here
    }

}
