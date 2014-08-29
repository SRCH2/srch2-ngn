package com.srch2.android.sdk;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.RejectedExecutionException;

class AutoPing {

    private static final String TAG = "HeartBeatPing";

    // should be slightly less than the amount of time the SRCH2 server core waits to autoshutdown
    static final int HEART_BEAT_PING_DELAY = IPCConstants.HEART_BEAT_AUTO_PING_PING_DELAY_MILLISECONDS;
    URL pingUrl;
    synchronized URL getPingUrl() { return pingUrl; }
    synchronized void setPingUrl(URL url) { pingUrl = url; }
    ExecutorService pingPool;
    Timer timer;
    static AutoPing instance;

    AutoPing() {
        pingPool = Executors.newFixedThreadPool(1);
    }

    // from SRCH2Engine call when checkcores loaded finishes
    // from SRCH2Service call when signling SRCH2Engine to proceed
    static void start(String pingUrlString) {
        Cat.d(TAG, "start");
        if (instance == null) {
            Cat.d(TAG, "start - instance null - initializing");
            instance = new AutoPing();
        }
        URL url = null;
        try {
            url = new URL(pingUrlString);
        } catch (MalformedURLException ignore) {
        }
        instance.setPingUrl(url);
        instance.pingAndRepeat();
    }

    // call anytime need to start pinging
    static void ping() {
        Cat.d(TAG, "ping");
        if (instance != null) {
            Cat.d(TAG, "ping - instance not null - pingAndRepeating");
            instance.pingAndRepeat();
        }
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
                    PingTask pt = new PingTask(instance);
                    try {
                        instance.pingPool.execute(pt);
                    } catch (RejectedExecutionException ignore) {
                    }
                }
            }, HEART_BEAT_PING_DELAY);
        }
    }

    // call before beforing any CRUD that will itself serve as the ping (not necessary)
    static void interrupt() {
        Cat.d(TAG, "interrupt");
        if (instance != null) {
            Cat.d(TAG, "interrupt - instaqnce not null");
            if (instance.timer != null) {
                instance.timer.cancel();
                instance.timer = null;
            }
        }
    }

    // call when stopping executable
    static void stop() {
        Cat.d(TAG, "stop");
        interrupt();
        if (instance != null) {
            instance.pingPool.shutdownNow();
            Cat.d(TAG, "stop - instaqnce assigned to null");
            instance = null;
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
            if (autoPingInstance != null && !Thread.currentThread().isInterrupted()) {
                URL pingUrl = autoPingInstance.getPingUrl();
                if (pingUrl != null) {
                    Cat.d(TAG, "autopinging info url " + pingUrl);
                    InternalInfoTask t = new InternalInfoTask(autoPingInstance.pingUrl , 250, false);
                    InternalInfoResponse iir = t.getInfo();
                    Cat.d(TAG, "run - got info is valid? " + iir.isValidInfoResponse);
                }
            }
            if (autoPingInstance != null && !Thread.currentThread().isInterrupted()) {
                Cat.d(TAG, "run - finished doing info heartbeatping not null ping and repeating");
                autoPingInstance.pingAndRepeat();
            }
        }
    }

}
