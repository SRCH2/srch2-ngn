package com.srch2.android.sdk;

import android.os.Handler;

import java.net.URL;
import java.util.Iterator;

class HeartBeatPing {

    // should be slightly less than the amount of time the SRCH2 server core waits to autoshutdown
    static final int HEART_BEAT_PING_DELAY = 40000;
    private Handler timer;
    private DelayTask timerCallback;

    private static HeartBeatPing instance;

    private HeartBeatPing() {
        timer = new Handler();
    }

    // call when checkcores loaded finishes
    static void start() {
        if (instance == null) {
            instance = new HeartBeatPing();
        }
        instance.pingAndRepeat();
    }

    // call anytime need to start pinging
    static void ping() {
        if (instance != null) {
            instance.pingAndRepeat();
        }
    }

    private void pingAndRepeat() {
        if (instance != null && timer != null) {
            timerCallback = new DelayTask();
            timer.postDelayed(timerCallback, HEART_BEAT_PING_DELAY);
        }
    }

    // callback for handling delay, necessary so that callback can be removed/interrupted
    private class DelayTask implements Runnable {
        @Override
        public void run() {
            if (instance != null) {
                HttpTask.executeTask(new PingTask(instance));
            }
        }
    }

    // call before beforing any CRUD that will itself serve as the ping
    static void interrupt() {
        if (instance != null) {
            if (instance.timerCallback != null) {
                instance.timer.removeCallbacks(instance.timerCallback);
                instance.timerCallback = null;
            }
        }
    }

    // call when stopping executable
    static void stop() {
        interrupt();
        if (instance != null) {
            instance = null;
        }
    }

    static class PingTask extends HttpTask {
        private HeartBeatPing heartBeatPing;

        public PingTask(HeartBeatPing pinger) {
            heartBeatPing = pinger;
        }

        @Override
        public void run() {
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
                t.getInfo();
            }
            if (heartBeatPing != null) {
                heartBeatPing.pingAndRepeat();
            }
        }
    }
}
