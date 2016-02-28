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
