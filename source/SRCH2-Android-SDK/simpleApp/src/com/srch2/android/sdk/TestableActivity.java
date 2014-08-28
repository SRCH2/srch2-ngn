package com.srch2.android.sdk;

import android.app.Activity;
import android.content.Context;

import java.io.File;
import java.util.List;

public abstract class TestableActivity extends Activity {
    public abstract List<String> getTestMethodNameListWithOrder();

    public abstract void beforeAll();

    public abstract void afterAll();

    public abstract void beforeEach();

    public abstract void afterEach();

    void deleteRecursive(File fileOrDirectory) {
        if (fileOrDirectory.isDirectory())
            for (File child : fileOrDirectory.listFiles())
                deleteRecursive(child);

        fileOrDirectory.delete();
    }

    void deleteSrch2Files() {
        deleteRecursive(new File(SRCH2Engine.detectAppHomeDir(this.getApplicationContext()) + File.separator + SRCH2Configuration.SRCH2_HOME_FOLDER_DEFAULT_NAME));
    }

    void onStartAndWaitForIsReady(Context context,  int stopWaitingAfterDelayTime) {
        SRCH2Engine.onStart(context);
        int timeSum = 0;
        while (true) {
            if (SRCH2Engine.isReady()) {
                break;
            }
            sleep(200);
            timeSum += 200;
            if (timeSum > stopWaitingAfterDelayTime) {
                break;
            }
        }
    }

    void onStopAndWaitForNotIsReady(Context context, int stopWaitingAfterDelayTime) {
        SRCH2Engine.onStart(context);
        int timeSum = 0;
        while (true) {
            if (!SRCH2Engine.isReady()) {
                break;
            }
            sleep(200);
            timeSum += 200;
            if (timeSum > stopWaitingAfterDelayTime) {
                break;
            }
        }
    }

    void sleep(int sleepTime) {
        try {
            Thread.currentThread().sleep(sleepTime);
        } catch (InterruptedException e) {
        }
    }
}


