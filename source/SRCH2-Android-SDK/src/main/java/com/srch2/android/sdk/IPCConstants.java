package com.srch2.android.sdk;

import android.content.Context;
import android.content.IntentFilter;

enum IPCConstants {
    INSTANCE;

    static final String INTENT_KEY_BROADCAST_ACTION = "do-action";
    static final String INTENT_VALUE_BROADCAST_ACTION_START_AWAITING_SHUTDOWN = "service_shutdown";
    static final String INTENT_VALUE_BROADCAST_ACTION_ENGINE_CRASHED_BUT_CAN_RESUME = "do-resume";

    static final String INTENT_KEY_PORT_NUMBER = "exe_port_number";
    static final String INTENT_KEY_XML_CONFIGURATION_FILE_LITERAL = "xml_configuration_file";
    static final String INTENT_KEY_OAUTH = "exe_oath";
    static final String INTENT_KEY_SHUTDOWN_URL = "url_shutdown";
    static final String INTENT_KEY_IS_DEBUG_AND_TESTING_MODE = "debug-testing-mode-switch";
    static final String INTENT_KEY_PING_URL = "ping-url";

    static IntentFilter getSRCH2ServiceBroadcastRecieverIntentFilter(Context c) {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(getSRCH2ServiceBroadcastRecieverIntentAction(c));
        return intentFilter;
    }

    static String getSRCH2ServiceBroadcastRecieverIntentAction(Context c) {
        return c.getPackageName() + ".SRCH2.SERVICE_COMMAND";
    }

    static IntentFilter getSRCH2EngineBroadcastRecieverIntentFilter(Context c) {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(getSRCH2EngineBroadcastRecieverIntentAction(c));
        return intentFilter;
    }

    static String getSRCH2EngineBroadcastRecieverIntentAction(Context c) {
        return c.getPackageName() + ".SRCH2.ENGINE_COMMAND";
    }


}
