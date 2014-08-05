package com.srch2.android.http.service;

import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;

import android.util.Log;


final class CheckCoresLoadedTask extends HttpTask {

    private static final String TAG = "CheckCoresLoadedTask";


    private static final int PING_RECONNECTION_TIME_MS = 250;

    private HashMap<String, URL> targetCoreUrlsMap;

    private boolean noNetworkConnection = false;

    CheckCoresLoadedTask(HashMap<String, URL> theTargetCoreUrls,
                         StateResponseListener theControlResponseListener) {
        targetCoreUrlsMap = theTargetCoreUrls;
        controlResponseObserver = theControlResponseListener;
    }

    @Override
    public void run() {
        Thread.currentThread().setName("CHECK CORES LOADED");

        Log.d("srch2:: " + TAG, "run start");

        int coreCount = targetCoreUrlsMap.size();
        Log.d("srch2:: " + TAG, "run coreCount " + coreCount);


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


            Log.d("srch2:: " + TAG, "run - core check loop @ iteration " + i);
            Log.d("srch2:: " + TAG, "run - core check targeting url " + targetUrl);

            InfoResponse ir = checkIfLoaded(targetUrl);

            if (ir != null) {
                Log.d("srch2:: " + TAG, "run - inforesponse from core " + indexName + " returned positive");


                Log.d("srch2:: " + TAG, "targetURL " + targetUrl.toExternalForm() + " connected!");

                Log.d("srch2:: " + TAG, "ir literal " + ir.restfulResponseLiteral);

                responseMap.put(indexName, ir);

                ++pingCountSuccess;
            } else {
                Log.d("srch2:: " + TAG, "run - inforesponse from core " + indexName + " returned negative");
            }

            ++i;
            if (i == coreCount) {
                i = 0;
            }

            if (pingCountSuccess == coreCount || noNetworkConnection) {
                Log.d("srch2:: " + TAG, "run - breaking from loop because: pingCount " + pingCountSuccess + " with coreCount " + coreCount + " and noNetworkConnection: " + noNetworkConnection);
                break;
            }

            ++superCount;
        }
        if (pingCountSuccess == coreCount) {
            Log.d("srch2:: " + TAG, "run - successful requerying etc");
            SRCH2Engine.isReady.set(true);
            SRCH2Engine.reQueryLastOne();
        }

        if (controlResponseObserver != null && !noNetworkConnection) {
            Log.d("srch2:: " + TAG, "run - notifying observer");
            controlResponseObserver.onSRCH2ServiceReady(responseMap);
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
