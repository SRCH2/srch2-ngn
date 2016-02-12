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

import android.content.Intent;
import android.test.ServiceTestCase;

import java.net.URL;
import java.util.Iterator;

public class SRCH2ServiceTest extends ServiceTestCase<SRCH2Service> {

    public SRCH2ServiceTest() {
        super(SRCH2Service.class);
    }

    Intent startServiceIntent;
    Intent stopServiceIntent;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        DumbIndex index = new DumbIndex();
        SRCH2Engine.setIndexables(index);
        SRCH2Engine.initialize();
        SRCH2Engine.initializeConfiguration(getContext());
        SRCH2Engine.registerReciever(getContext());
        Intent i = new Intent(getContext(), SRCH2Service.class);
        i.putExtra(IPCConstants.INTENT_KEY_XML_CONFIGURATION_FILE_LITERAL,
                SRCH2Configuration.generateConfigurationFileString(SRCH2Engine.conf));
        i.putExtra(IPCConstants.INTENT_KEY_SHUTDOWN_URL, UrlBuilder
                .getShutDownUrl(SRCH2Engine.getConfig()).toString());
        URL pingUrl = null;
        IndexableCore defaultIndexable = null;
        Iterator<IndexableCore> it = SRCH2Engine.conf.indexableMap.values().iterator();
        if (it.hasNext()) {
            defaultIndexable = it.next();
            pingUrl = UrlBuilder
                    .getInfoUrl(
                            SRCH2Engine.conf,
                            defaultIndexable.indexInternal.indexDescription);
        }
        i.putExtra(IPCConstants.INTENT_KEY_PING_URL,
                pingUrl == null ? UrlBuilder.getShutDownUrl(SRCH2Engine.getConfig()).toString() : pingUrl.toString());
        i.putExtra(IPCConstants.INTENT_KEY_PORT_NUMBER, SRCH2Engine.getConfig().getPort());
        i.putExtra(IPCConstants.INTENT_KEY_OAUTH, SRCH2Engine.getConfig().getAuthorizationKey());
        i.putExtra(IPCConstants.INTENT_KEY_IS_DEBUG_AND_TESTING_MODE, false);
        startServiceIntent = i;

        Intent ii = new Intent(
                IPCConstants
                        .getSRCH2ServiceBroadcastRecieverIntentAction(getContext()));
        ii.putExtra(IPCConstants.INTENT_KEY_BROADCAST_ACTION,
                IPCConstants.INTENT_VALUE_BROADCAST_ACTION_START_AWAITING_SHUTDOWN);
        stopServiceIntent = ii;
    }

    public void testAutoPingRelability() {
        assertNull(AutoPing.instance);

        startService(startServiceIntent);

        sleep(1000);

        assertNotNull(AutoPing.instance);

        SRCH2Service service = getService();

        sleep(60000);

        assertNotNull(AutoPing.instance);

        sleep(5000);

        getContext().sendBroadcast(stopServiceIntent);

        sleep(service.getTimeToWaitForShutdown() + 2000);

        assertNull(AutoPing.instance);
    }


    void sleep(int sleepTime) {
        try {
            Thread.currentThread().sleep(sleepTime);
        } catch (InterruptedException e) {
        }
    }

}
