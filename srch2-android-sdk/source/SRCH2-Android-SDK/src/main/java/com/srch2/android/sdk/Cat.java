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

import android.util.Log;

/**
 * Logger class.
 * <br><br>
 * Use the static method <code>Cat.d(String tag, String message)</code> instead of the Android SDK Logger
 * class method <code>Log.d(String tag, String message)</code> whenever you need to log output.
 * <br><br>
 * All logs done with this class will have 's2sdk:: ' as the a prefix to the supplied tag, so that every
 * log output from this class can be filtered on by creating a new filter configuration and in the filter
 * tag field adding 's2sdk::|'. The OR operator | enables oring against other whatever follows the tag
 * prefix.
 * <br><br>
 * <b>When compiling for release be sure to set {@code isLogging} to false</b>.
 */
class Cat {
    private static boolean isLogging = false;
    private static final String TAG_PREFIX = "s2sdk:: ";

    static void d(String tag, String message) {
        if (isLogging) {
            Log.d(TAG_PREFIX.concat(tag), message);
        }
    }

    static void e(String tag, String message) {
        if (isLogging) {
            Log.e(TAG_PREFIX.concat(tag), message);
        }
    }

    static void ex(String tag, String message, Exception exception) {
        if (isLogging) {
            Log.e(tag, "$EXCEPTION--------------------------------");
            Log.e(tag, message);
            exception.printStackTrace();
        }
    }
}
