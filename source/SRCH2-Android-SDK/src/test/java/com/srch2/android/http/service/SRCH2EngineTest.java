package com.srch2.android.http.service;

import junit.framework.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@Config(emulateSdk = 18)
@RunWith(RobolectricTestRunner.class)
public class SRCH2EngineTest {

    static {
        PrepareEngine.prepareEngine();
    }

    @Test(expected = IllegalArgumentException.class)
    public void nullConfiguration() {
        SRCH2Engine.initialize(null, null, null);
    }

    @Test
    public void listenerCouldBeNull() {
        SRCH2Engine.initialize(PrepareEngine.musicIndex, PrepareEngine.movieIndex, PrepareEngine.geoIndex);
    }

    @Test
    public void multipleInitializeShouldBeFine() {
        SRCH2Engine.initialize(PrepareEngine.musicIndex, null);

    }

    @Test
    public void shouldNotReadyWhenNotOnResumed() {
        SRCH2Engine.initialize(PrepareEngine.musicIndex, null);
        Assert.assertFalse(SRCH2Engine.isReady());
    }

    @Test
    public void testPlan() {
        //TODO
//        SRCH2Engine.isReady();
//        SRCH2Engine.onStop(null);
//        SRCH2Engine.onStart(null);
//        SRCH2Engine.powerSearchAllIndexes(null);
//        SRCH2Engine.searchAllIndexes(null);
//        SRCH2Engine.setStateResponseListener(null);
//        SRCH2Engine.setSearchResultsListener(null);
    }
    
}
