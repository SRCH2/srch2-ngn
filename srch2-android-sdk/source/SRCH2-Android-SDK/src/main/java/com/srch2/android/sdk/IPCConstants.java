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

import android.content.Context;
import android.content.IntentFilter;

enum IPCConstants {
    INSTANCE;

    static final String INTENT_KEY_BROADCAST_ACTION = "do-action";
    static final String INTENT_VALUE_BROADCAST_ACTION_START_AWAITING_SHUTDOWN = "service_shutdown";
    static final String INTENT_VALUE_BROADCAST_ACTION_ENGINE_CRASHED_BUT_CAN_RESUME = "do-resume";
    static final String INTENT_VALUE_BROADCAST_ACTION_ENGINE_STARTED_PROCEED = "do-proceed";
    static final String INTENT_VALUE_BROADCAST_ACTION_VALIDATE_SRCH2ENGINE_ALIVE = "is-alive";
    static final String INTENT_VALUE_BROADCAST_ACTION_VALIDATE_SRCH2ENGINE_ALIVE_FOR_PING = "do-ping";

   /** Use this set the timer for the server core and autoping: nothing else! */
    static final int HEART_BEAT_SERVER_CORE_SHUTDOWN_DELAY_SECONDS = 60;
    /** Is determined by {@link #HEART_BEAT_SERVER_CORE_SHUTDOWN_DELAY_SECONDS} and something less than it, so that
     * the server doesn't shut down since it will get pinged before doing so. */
    static final int HEART_BEAT_AUTO_PING_PING_DELAY_MILLISECONDS = (int) ((HEART_BEAT_SERVER_CORE_SHUTDOWN_DELAY_SECONDS * 1000) * .9f);

    static final int DEBUG_AND_TESTING_MODE_QUICK_SHUTDOWN_DELAY = 1000;

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
