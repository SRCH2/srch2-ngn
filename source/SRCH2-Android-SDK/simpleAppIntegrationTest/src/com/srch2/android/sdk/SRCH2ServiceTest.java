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
