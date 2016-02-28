/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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

    @Test
    public void listenerCouldBeNull() {
        SRCH2Engine.setIndexables(PrepareEngine.musicIndex, PrepareEngine.movieIndex, PrepareEngine.geoIndex);
        SRCH2Engine.initialize();
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
        SRCH2Engine.initialize();
        Assert.assertEquals(PrepareEngine.musicIndex.getIndexName(), SRCH2Engine.getIndex(PrepareEngine.musicIndex.getIndexName()).getIndexName());
    }

    @Test(expected = IllegalArgumentException.class)
    public void getIndexThrowsWhenGettingIndexableWhenNameCorrespondsToSqliteIndexable() {
        SRCH2Engine.setIndexables(PrepareEngine.musicIndex);

        SRCH2Engine.initialize();
        SRCH2Engine.conf.indexableMap.put(PrepareEngine.dbIndex.getIndexName(), PrepareEngine.dbIndex);
        SRCH2Engine.getIndex(PrepareEngine.dbIndex.getIndexName());
    }

    @Test(expected = IllegalArgumentException.class)
    public void getIndexThrowsWhenGettingSqliteIndexableWhenNameCorrespondsToIndexable() {
        SRCH2Engine.setIndexables(PrepareEngine.musicIndex);
        SRCH2Engine.initialize();
        SRCH2Engine.getSQLiteIndex(PrepareEngine.musicIndex.getIndexName());
    }
}
