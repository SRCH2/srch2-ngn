package com.srch2.android.sdk;

import java.net.URL;
import java.util.Iterator;
import java.util.Timer;
import java.util.TimerTask;

class HeartBeatPing {

    static final int HEART_BEAT_PING_DELAY = 10000;
    Timer heartBeatTimer;

    void startPinging() {
        resetHeartBeatPing();
    }

    void resetHeartBeatPing() {
        if (heartBeatTimer != null) {
            heartBeatTimer.cancel();
        }
        heartBeatTimer.schedule(new TimerTask() {
            @Override
            public void run() {
                HttpTask.executeTask(new HeartBeatPingTask());
                resetHeartBeatPing();
            }
        }, HEART_BEAT_PING_DELAY);
    }

    void stopPinging() {
        if (heartBeatTimer != null) {
            heartBeatTimer.cancel();
        }
    }

    static class HeartBeatPingTask extends HttpTask {
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
        }
    }
}
