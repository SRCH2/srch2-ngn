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

import java.net.MalformedURLException;
import java.net.URL;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.atomic.AtomicReference;

class AutoPing {

    interface ValidatePingCommandCallback {
        void validateIfSRCH2EngineAlive();
    }

    private static final String TAG = "HeartBeatPing";

    // should be slightly less than the amount of time the SRCH2 server core waits to autoshutdown
    static final int HEART_BEAT_PING_DELAY = IPCConstants.HEART_BEAT_AUTO_PING_PING_DELAY_MILLISECONDS;
    URL pingUrl;
    synchronized URL getPingUrl() { return pingUrl; }
    synchronized void setPingUrl(URL url) { pingUrl = url; }
    ExecutorService pingPool;
    Timer timer;
    static AtomicReference<AutoPing> instance;
    ValidatePingCommandCallback callback;

    AutoPing() {
        pingPool = Executors.newFixedThreadPool(1);
    }

    // from SRCH2Engine call when checkcores loaded finishes
    // from SRCH2Service call when signling SRCH2Engine to proceed
    static void start(ValidatePingCommandCallback theCallback, String pingUrlString) {
        Cat.d(TAG, "start");
        if (instance == null) {
            Cat.d(TAG, "start - instance null - initializing");
            instance = new AtomicReference<AutoPing>(new AutoPing());
        }
        URL url = null;
        try {
            url = new URL(pingUrlString);
        } catch (MalformedURLException ignore) {
        }
        AutoPing autoPing = instance.get();


        autoPing.setPingUrl(url);
        autoPing.pingAndRepeat();
        autoPing.callback = theCallback;
    }


    private void pingAndRepeat() {
        Cat.d(TAG, "pingAndRepeat");
        if (instance != null) {
            if (timer != null) {
                timer.cancel();
            }
            timer = new Timer();
            timer.schedule(new TimerTask() {
                @Override
                public void run() {
                    if (instance != null) {
                        AutoPing autoPing = instance.get();

                        if (autoPing != null && autoPing.callback != null) {
                            autoPing.callback.validateIfSRCH2EngineAlive();
                        }
                    }
                }
            }, HEART_BEAT_PING_DELAY);
        }
    }

    static void doPing() {
        if (instance != null) {
            AutoPing autoPing = instance.get();

            PingTask pt = new PingTask(autoPing);
            try {
                Cat.d(TAG, "doing autoping");
                autoPing.pingPool.execute(pt);
            } catch (RejectedExecutionException ignore) {
            }
        }

    }

    // call before beforing any CRUD that will itself serve as the ping (not necessary)
    static void interrupt() {
        Cat.d(TAG, "interrupt");
        if (instance != null) {
            AutoPing autoPing = instance.get();
            Cat.d(TAG, "interrupt - instance not null");
            if (autoPing != null && autoPing.timer != null) {
                autoPing.timer.cancel();
                autoPing.timer = null;
            }
        }
    }

    // call when stopping executable
    static void stop() {
        Cat.d(TAG, "stop");
        interrupt();
        if (instance != null) {
            AutoPing autoPing = instance.get();
            if (autoPing != null) {
                autoPing.pingPool.shutdownNow();
                Cat.d(TAG, "stop - instaqnce assigned to null");
                autoPing = null;
                instance.set(null);
                instance = null;
            }

        }
    }

    static class PingTask implements Runnable {
        private AutoPing autoPingInstance;

        public PingTask(AutoPing instance) {
            Cat.d(TAG, "PingTask()");
            autoPingInstance = instance;
        }

        @Override
        public void run() {
            Cat.d(TAG, "run");
            boolean wasValid = true;
            if (autoPingInstance != null && !Thread.currentThread().isInterrupted()) {
                URL pingUrl = autoPingInstance.getPingUrl();
                if (pingUrl != null) {
                    Cat.d(TAG, "autopinging info url " + pingUrl);
                    InternalInfoTask t = new InternalInfoTask(autoPingInstance.pingUrl , 250, false);
                    InternalInfoResponse iir = t.getInfo();
                    wasValid = iir.isValidInfoResponse;
                    Cat.d(TAG, "run - got info is valid? " + iir.isValidInfoResponse);
                }
            }
            if (wasValid && autoPingInstance != null && !Thread.currentThread().isInterrupted()) {
                Cat.d(TAG, "run - finished doing info heartbeatping not null ping and repeating");
                autoPingInstance.pingAndRepeat();
            }
        }
    }

}
