package com.srch2.android.sdk;

import java.net.URL;
import java.util.Iterator;
import java.util.Timer;
import java.util.TimerTask;

class HeartBeatPing {

    private static final String TAG = "HeartBeatPing";

    // should be slightly less than the amount of time the SRCH2 server core waits to autoshutdown
    static final int HEART_BEAT_PING_DELAY = 50000;
    private Timer timer;
    private static HeartBeatPing instance;

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
        if (instance != null) {
            if (timer != null) {
                timer.cancel();
            }
            timer = new Timer();
            timer.schedule(new TimerTask() {
                @Override
                public void run() {
                    HttpTask.executeTask(new PingTask(instance));
                }
            }, HEART_BEAT_PING_DELAY);
        }
    }

    // call before beforing any CRUD that will itself serve as the ping
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
