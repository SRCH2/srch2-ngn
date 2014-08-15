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
 * <b>When compiling for release be sure to set <code>isLogging</code> to false</b>.
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
}
