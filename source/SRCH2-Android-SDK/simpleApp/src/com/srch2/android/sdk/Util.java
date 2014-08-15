package com.srch2.android.sdk;

import static junit.framework.Assert.assertTrue;

public class Util {

    static final int TOLERANCE = 30000;
    static final int SLEEP_PER_ROUOND = 100;

    public static void waitForResponse(TestControlResponseListener controlResponseListener, Class clazz) {
        int tolerate = TOLERANCE;
        int sleepPerRound = SLEEP_PER_ROUOND;
        for (; tolerate > 0; tolerate -= sleepPerRound) {
            if (clazz == InsertResponse.class) {
                if (checkIfNotNullElseSleep(controlResponseListener.insertResponse, sleepPerRound)) {
                    return;
                }
            } else if (clazz == DeleteResponse.class) {
                if (checkIfNotNullElseSleep(controlResponseListener.deleteResponse, sleepPerRound)) {
                    return;
                }
            } else if (clazz == UpdateResponse.class) {
                if (checkIfNotNullElseSleep(controlResponseListener.updateResponse, sleepPerRound)) {
                    return;
                }
            } else if (clazz == GetRecordResponse.class) {
                if (checkIfNotNullElseSleep(controlResponseListener.recordResponse, sleepPerRound)) {
                    return;
                }
            }
        }
        assertTrue(tolerate > 0);
    }

    static boolean checkIfNotNullElseSleep(Object response, int sleepPerRound) {
        if (response != null) {
            return true;
        }
        try {
            Thread.currentThread().sleep(sleepPerRound);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        return false;
    }

    public static void waitSRCH2EngineIsReady() {
        int tolerate = TOLERANCE;
        int sleepPerRound = SLEEP_PER_ROUOND;
        for (; tolerate > 0 && !SRCH2Engine.isReady(); tolerate -= sleepPerRound) {
            try {
                Thread.currentThread().sleep(sleepPerRound);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        assertTrue(SRCH2Engine.isReady());
    }

    public static void waitForResultResponse(TestSearchResultsListener searchResultsListener) {
        int tolerate = TOLERANCE;
        int sleepPerRound = SLEEP_PER_ROUOND;
        for (; tolerate > 0 && searchResultsListener.resultRecordMap == null; tolerate -= sleepPerRound) {
            try {
                Thread.currentThread().sleep(sleepPerRound);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        assertTrue(tolerate > 0);
    }

}
