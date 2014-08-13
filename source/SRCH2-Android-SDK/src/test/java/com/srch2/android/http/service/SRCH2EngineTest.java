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
    public void getIndexGetsCorrectIndexable() {
        SRCH2Engine.initialize(PrepareEngine.musicIndex);
        Assert.assertEquals(PrepareEngine.musicIndex.getIndexName(), SRCH2Engine.getIndex(PrepareEngine.musicIndex.getIndexName()));
    }
}
