package com.srch2.android.sdk;

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

    @Test(expected = IllegalStateException.class)
    public void nullConfiguration() {
        SRCH2Engine.onStart();
    }

    @Test
    public void listenerCouldBeNull() {
        SRCH2Engine.setIndexables(PrepareEngine.musicIndex, PrepareEngine.movieIndex, PrepareEngine.geoIndex);
        SRCH2Engine.onStart();
    }

    @Test
    public void multipleInitializeShouldBeFine() {
        SRCH2Engine.setIndexables(PrepareEngine.musicIndex, (Indexable[]) null);
    }

    @Test
    public void shouldNotReadyWhenNotOnResumed() {
        SRCH2Engine.setIndexables(PrepareEngine.musicIndex, (Indexable[]) null);
        Assert.assertFalse(SRCH2Engine.isReady());
    }

    @Test
    public void getIndexGetsCorrectIndexable() {
        SRCH2Engine.setIndexables(PrepareEngine.musicIndex);
        SRCH2Engine.onStart();
        Assert.assertEquals(PrepareEngine.musicIndex.getIndexName(), SRCH2Engine.getIndex(PrepareEngine.musicIndex.getIndexName()).getIndexName());
    }

    @Test(expected = IllegalArgumentException.class)
    public void getIndexThrowsWhenGettingIndexableWhenNameCorrespondsToSqliteIndexable() {
        SRCH2Engine.setIndexables(PrepareEngine.musicIndex);

        SRCH2Engine.onStart();
        SRCH2Engine.conf.indexableMap.put(PrepareEngine.dbIndex.getIndexName(), PrepareEngine.dbIndex);
        SRCH2Engine.getIndex(PrepareEngine.dbIndex.getIndexName());
    }

    @Test(expected = IllegalArgumentException.class)
    public void getIndexThrowsWhenGettingSqliteIndexableWhenNameCorrespondsToIndexable() {
        SRCH2Engine.setIndexables(PrepareEngine.musicIndex);
        SRCH2Engine.onStart();
        SRCH2Engine.getSQLiteIndex(PrepareEngine.musicIndex.getIndexName());
    }

    @Test(expected = IllegalStateException.class)
    public void shouldNotStartWithoutIndexableOrSqliteIndexableSet() {
        SRCH2Engine.onStart();
    }
}
