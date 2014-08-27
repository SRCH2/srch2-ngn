package com.srch2.android.sdk;

import android.os.Handler;

import java.net.URL;
import java.util.Iterator;

class HeartBeatPing {

    private static final String TAG = "HeartBeatPing";

    // should be slightly less than the amount of time the SRCH2 server core waits to autoshutdown
    static final int HEART_BEAT_PING_DELAY = 50000;
    private Handler timer;
    private DelayTask timerCallback;

    private static HeartBeatPing instance;

    private HeartBeatPing() {
        Cat.d(TAG, "HeartBeatPing()");
        timer = new Handler();
    }

    // call when checkcores loaded finishes
    static void start() {
        Cat.d(TAG, "start");
        if (instance == null) {
            Cat.d(TAG, "start - instance null - initializing");
            instance = new HeartBeatPing();
        }
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
        if (instance != null && timer != null) {
            Cat.d(TAG, "pingAndRepeat - instance,timer not null");
            timerCallback = new DelayTask();
            timer.postDelayed(timerCallback, HEART_BEAT_PING_DELAY);
        }
    }

    // callback for handling delay, necessary so that callback can be removed/interrupted
    private class DelayTask implements Runnable {
        @Override
        public void run() {
            Cat.d(TAG, "delaytask::run()");
            if (instance != null) {
                Cat.d(TAG, "delaytask::run() instance not null executing pingtask");
                HttpTask.executeTask(new PingTask(instance));
            }
        }
    }

    // call before beforing any CRUD that will itself serve as the ping
    static void interrupt() {
        Cat.d(TAG, "interrupt");
        if (instance != null) {
            Cat.d(TAG, "interrupt - instaqnce not null");
            if (instance.timerCallback != null) {
                instance.timer.removeCallbacks(instance.timerCallback);
                instance.timerCallback = null;
            }
        }
    }

    // call when stopping executable
    static void stop() {
        Cat.d(TAG, "stop");
        interrupt();
        if (instance != null) {
            Cat.d(TAG, "stop - instaqnce assigned to null");
            instance = null;
        }
    }

    static class PingTask extends HttpTask {
        private HeartBeatPing heartBeatPing;

        public PingTask(HeartBeatPing pinger) {
            Cat.d(TAG, "PingTask()");
            heartBeatPing = pinger;
        }

        @Override
        public void run() {
            Cat.d(TAG, "run");
            Indexable defaultIndexable = null;
            Iterator<Indexable> it = SRCH2Engine.conf.indexableMap.values().iterator();
            if (it.hasNext()) {
                defaultIndexable = it.next();
            }
            if (defaultIndexable != null) {
                URL url = UrlBuilder
                        .getInfoUrl(
                                SRCH2Engine.conf,
                                defaultIndexable.indexInternal.indexDescription);
                InternalInfoTask t = new InternalInfoTask(url, 250, false);
                InternalInfoResponse iir = t.getInfo();
                Cat.d(TAG, "run - got info is valid? " + iir.isValidInfoResponse);
            }
            if (heartBeatPing != null) {
                Cat.d(TAG, "run - finished doing info heartbeatping not null ping and repeating");
                heartBeatPing.pingAndRepeat();
            }
        }
    }
}
