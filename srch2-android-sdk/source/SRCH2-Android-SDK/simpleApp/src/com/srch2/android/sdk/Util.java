package com.srch2.android.sdk;

import static junit.framework.Assert.assertTrue;

public class Util {

    static final int TOLERANCE = 30000;
    static final int SLEEP_PER_ROUOND = 100;

    public static enum ResponseType {
        Insert,
        Delete,
        Update,
        GetRecord
    }

    public static void waitForResponse(ResponseType responseTypeToWaitFor, TestableIndex index) {
        int tolerate = TOLERANCE;
        int sleepPerRound = SLEEP_PER_ROUOND;
        for (; tolerate > 0; tolerate -= sleepPerRound) {
            if (responseTypeToWaitFor == ResponseType.Insert) {
                if (checkIfNotNullElseSleep(responseTypeToWaitFor, index, sleepPerRound)) {
                    return;
                }
            } else if (responseTypeToWaitFor == ResponseType.Delete) {
                if (checkIfNotNullElseSleep(responseTypeToWaitFor, index, sleepPerRound)) {
                    return;
                }
            } else if (responseTypeToWaitFor == ResponseType.Update) {
                if (checkIfNotNullElseSleep(responseTypeToWaitFor, index, sleepPerRound)) {
                    return;
                }
            } else if (responseTypeToWaitFor == ResponseType.GetRecord) {
                if (checkIfNotNullElseSleep(responseTypeToWaitFor, index, sleepPerRound)) {
                    return;
                }
            }
        }
        assertTrue(tolerate > 0);
    }

    static boolean checkIfNotNullElseSleep(ResponseType responseType, TestableIndex index, int sleepPerRound) {
        String responseString = null;
        switch (responseType) {
            case Insert:
                responseString = index.insertResponse;
                break;
            case Update:
                responseString = index.updateResponse;
                break;
            case Delete:
                responseString = index.deleteResponse;
                break;
            case GetRecord:
                responseString = index.getRecordResponse;
                break;
        }

        if (responseString != null) {
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
