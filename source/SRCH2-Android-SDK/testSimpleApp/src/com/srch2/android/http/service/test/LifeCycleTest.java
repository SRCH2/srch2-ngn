package com.srch2.android.http.service.test;

import android.os.SystemClock;
import android.test.SingleLaunchActivityTestCase;
import android.util.Log;
import com.srch2.android.http.service.LifeCycleTestActivity;
import com.srch2.android.http.service.SRCH2Engine;

/**
 * Created by ashton on 7/29/2014.
 */
public class LifeCycleTest extends SingleLaunchActivityTestCase<LifeCycleTestActivity> {

    private static final String TAG = "srch2:: LifeCycleTest";


    LifeCycleTestActivity activity;
    int count;

    public static final int SRCH2_SERVICE_SHUTDOWN_TIMEOUT_MS = 60000;


    public LifeCycleTest() {
        super("com.srch2.android.http.service", LifeCycleTestActivity.class);
        ++count;
        Log.d(TAG, "LifeCycleTest() - " + count);

    }


    @Override
    public void setUp() throws Exception {
        super.setUp();
        Log.d(TAG, "setUp()");
    }

    @Override
    public void tearDown() throws Exception {
        super.tearDown();
        Log.d(TAG, "tearDown");
    }

    public void testLifeCycle() {
        Log.d(TAG, "testLifeCycle - START");
        activity = getActivity();
        activity.initializeSRCH2Engine(false);
        Log.d(TAG, "testLifeCycle - initialized, starting srch2Engine");
        doStartIsReadyTest();
       // doInsertAndVerifyRecordCountTest();
        waitRandomAmountOfTime(2000, 3000);


        doReuseTest();
        waitRandomAmountOfTime(2000, 3000);



        doStopIsNotReadyTest();
        waitRandomAmountOfTime(2000, 3000);








/*
        for (int i = 0; i < 10; ++i) {
            Log.d(TAG, "testLifeCycle iterating @ count " + i);
            waitRandomAmountOfTime(SRCH2_SERVICE_SHUTDOWN_TIMEOUT_MS + 1000, SRCH2_SERVICE_SHUTDOWN_TIMEOUT_MS + 5000);
            doStartIsReadyTest();
            waitRandomAmountOfTime(1000, 2000);
            doVerifyTenRecordsInIndexTest();
            waitRandomAmountOfTime(2000, 2000);
            doStopIsNotReadyTest();
        }

*/
        Log.d(TAG, "testLifeCycle DONE");
        activity.finish();
    }

    private void doExecutableWasReusedByVerifyingPortIsNotNewTest() {
        if (!activity.getSRCH2EngineIsReady()) {
            activity.startSRCH2Engine();
            waitRandomAmountOfTime(1000, 3000);
        }

        doReuseTest();
    }

    /** Assumes engine is already running. */
    private void doReuseTest() {

        int portBeingUsed = activity.getPort();

        activity.stopSRCH2Engine();

        waitRandomAmountOfTime(1000, 5000);

        activity.startSRCH2Engine();

        waitRandomAmountOfTime(2000, 3000);

        int newPortShouldMatchOldPortIfReuseHappenning = activity.getPort();

        if (portBeingUsed != 0 && newPortShouldMatchOldPortIfReuseHappenning != 0) {
            assertEquals("Did previousily used port stay the same indicating the already running engine was reused?", portBeingUsed, newPortShouldMatchOldPortIfReuseHappenning);
        } else {
            assertEquals("Did previousily used port stay the same indicating the already running engine was reused?", portBeingUsed, -1);
        }
    }







    private void doStartIsReadyTest() {

        Log.d(TAG, "doStartIsReadyTest - START");

        activity.startSRCH2Engine();
        Log.d(TAG, "doStartIsReadyTest srch2engineisready ---------------------------" + activity.getSRCH2EngineIsReady());
        activity.control.reset();
        boolean breakWait = false;
        long waitUntilTime = SystemClock.uptimeMillis() + 20000;
        while (!breakWait) {
            breakWait = activity.getSRCH2EngineIsReady();
            Log.d(TAG, "doStartIsReadyTest, isready breakwait is " + breakWait);
            try {
                Thread.currentThread().sleep(500);
            } catch (InterruptedException ignore) {
            }

            if (SystemClock.uptimeMillis() > waitUntilTime) {
                break;
            }
        }
        Log.d(TAG, "doStartIsReadyTest srch2engineisready ---------------------------" + activity.getSRCH2EngineIsReady());
        boolean startSuccess = activity.getSRCH2EngineIsReady();
        assertEquals("Did engine start with success", true, startSuccess);
        Log.d(TAG, "doStartIsReadyTest - FINISH did engine start? " + startSuccess);
    }

    private void doInsertAndVerifyRecordCountTest() {
        Log.d(TAG, "doLifeCycle - doInsertAndVerifyRecordCountTest - START");
        activity.control.reset();
        activity.insertTenRecordsStartingAtPrimaryKeyEqualTo1();
        long waitUntilTime = SystemClock.uptimeMillis() + 20000;
        boolean breakWait = false;
        while (!breakWait) {
            breakWait = activity.verifyTenRecordsHaveBeenInserted();
            try {
                Thread.currentThread().sleep(250);
            } catch (InterruptedException ignore) {
            }

            if (SystemClock.uptimeMillis() > waitUntilTime) {
                break;
            }
        }
        assertEquals("Did engine insert ten records?", true, breakWait);
        Log.d(TAG, "doLifeCycle - doInsertAndVerifyRecordCountTest FINISH - did insert ten records? " + breakWait);
    }


    private void doStopIsNotReadyTest() {
        Log.d(TAG, "doLifeCycle doStopIsNotReadyTest - START");
        Log.d(TAG, "doStopIsNotReadyTest --- srch2engineisready ---------------------------" + activity.getSRCH2EngineIsReady());
        activity.stopSRCH2Engine();
        long waitUntilTime = SystemClock.uptimeMillis() + SRCH2_SERVICE_SHUTDOWN_TIMEOUT_MS + 1000;
        boolean isReady = true;
        while (isReady) {
            //isReady = activity.getSRCH2EngineIsReady();
            try {
                Thread.currentThread().sleep(250);
            } catch (InterruptedException ignore) {
            }

            if (SystemClock.uptimeMillis() > waitUntilTime) {
                break;
            }
        }
        Log.d(TAG, "doStopIsNotReadyTest --- srch2engineisready ---------------------------" + activity.getSRCH2EngineIsReady());

        boolean isStopped = SRCH2Engine.isReady();
        assertEquals("Did engine come to a complete stop?", false, isStopped);
        Log.d(TAG, "doLifeCycle doStopIsNotReadyTest - FINISH did the engine stop? " + !isStopped);
    }

    private void doVerifyTenRecordsInIndexTest() {
        Log.d(TAG, "doVerifyTenRecordsInIndexTest - START");
        long waitUntilTime = SystemClock.uptimeMillis() + 20000;
        boolean breakWait = false;
        while (!breakWait) {
            activity.control.reset();
            activity.doTestIndexInfo();
            try {
                Thread.currentThread().sleep(500);
            } catch (InterruptedException ignore) {
            }
            breakWait = activity.verifyTenRecordsInInfoResponse();

            if (SystemClock.uptimeMillis() > waitUntilTime) {
                break;
            }
        }
        assertEquals("Were records deleted?", true, breakWait);
        Log.d(TAG, "doVerifyTenRecordsInIndexTest - FINISH did engine have records? " + breakWait);
    }

    private void waitRandomAmountOfTime(int minimumTime, int maximumTime) {
        minimumTime = minimumTime < 2000 ? 2000 : minimumTime;
        Log.d(TAG, "waitRandomAmountOfTime - START waiting at least " + minimumTime);
        long breakTime = SystemClock.uptimeMillis() + (int) ((Math.random() * maximumTime) + minimumTime);

        while (SystemClock.uptimeMillis() < breakTime) {
            try {
                Thread.currentThread().sleep(250);
            } catch (InterruptedException e) { }
        }
        Log.d(TAG, "waitRandomAmountOfTime - FINISH");
    }



}
