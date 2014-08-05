package com.srch2.android.http.service;

import android.util.Log;

import static junit.framework.Assert.assertTrue;

public class Util {

    static final int TOLERANCE = 60000;
    static final int SLEEP_PER_ROUOND = 100;

    public static void waitForResponse(MyActivity activity, Class clazz) {
        int tolerate = TOLERANCE;
        int sleepPerRound = SLEEP_PER_ROUOND;
        for (; tolerate > 0; tolerate -= sleepPerRound) {
            if (clazz == InsertResponse.class) {
                if (checkIfNotNullElseSleep(activity.mControlListener.insertResponse, sleepPerRound)) {
                    return;
                }
            } else if (clazz == InfoResponse.class) {
                if (checkIfNotNullElseSleep(activity.mControlListener.infoResponse, sleepPerRound)) {
                    return;
                }
            } else if (clazz == DeleteResponse.class) {
                if (checkIfNotNullElseSleep(activity.mControlListener.deleteResponse, sleepPerRound)) {
                    return;
                }
            } else if (clazz == UpdateResponse.class) {
                if (checkIfNotNullElseSleep(activity.mControlListener.updateResponse, sleepPerRound)) {
                    return;
                }
            } else if (clazz == GetRecordResponse.class) {
                if (checkIfNotNullElseSleep(activity.mControlListener.recordResponse, sleepPerRound)) {
                    return;
                }
            }
        }
        assertTrue(tolerate > 0);
    }

    static boolean checkIfNotNullElseSleep(RestfulResponse response, int sleepPerRound) {
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

    public static void waitForResultResponse(MyActivity activity) {
        int tolerate = TOLERANCE;
        int sleepPerRound = SLEEP_PER_ROUOND;
        for (; tolerate > 0 && activity.mResultListener.resultRecordMap == null; tolerate -= sleepPerRound) {
            try {
                Thread.currentThread().sleep(sleepPerRound);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        assertTrue(tolerate > 0);
    }
}
