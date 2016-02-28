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

import android.app.Activity;
import android.content.Context;

import java.io.File;
import java.util.List;

public abstract class TestableActivity extends Activity {
    public abstract List<String> getTestMethodNameListWithOrder();

    public abstract void beforeAll();

    public abstract void afterAll();

    public abstract void beforeEach();

    public abstract void afterEach();

    void deleteRecursive(File fileOrDirectory) {
        if (fileOrDirectory.isDirectory())
            for (File child : fileOrDirectory.listFiles())
                deleteRecursive(child);

        fileOrDirectory.delete();
    }

    void deleteSrch2Files() {
        deleteRecursive(new File(SRCH2Engine.detectAppFilesDirectory(this.getApplicationContext()) + File.separator + SRCH2Configuration.SRCH2_HOME_FOLDER_DEFAULT_NAME));
    }

    void onStartAndWaitForIsReady(Context context,  int stopWaitingAfterDelayTime) {
        SRCH2Engine.onResume(context);
        int timeSum = 0;
        while (true) {
            if (SRCH2Engine.isReady()) {
                break;
            }
            sleep(200);
            timeSum += 200;
            if (timeSum > stopWaitingAfterDelayTime) {
                break;
            }
        }
    }

    void onStopAndWaitForNotIsReady(Context context, int stopWaitingAfterDelayTime) {
        SRCH2Engine.onPause(context);
        int timeSum = 0;
        while (true) {
            if (!SRCH2Engine.isReady()) {
                break;
            }
            sleep(200);
            timeSum += 200;
            if (timeSum > stopWaitingAfterDelayTime) {
                break;
            }
        }
    }

    void sleep(int sleepTime) {
        try {
            Thread.currentThread().sleep(sleepTime);
        } catch (InterruptedException e) {
        }
    }
}


