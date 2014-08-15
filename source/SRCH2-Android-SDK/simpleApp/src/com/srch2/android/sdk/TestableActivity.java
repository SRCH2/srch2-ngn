package com.srch2.android.sdk;

import android.app.Activity;

import java.util.List;

/**
 * Created by jianfeng on 8/7/14.
 */
public abstract class TestableActivity extends Activity {
    public abstract List<String> getTestMethodNameListWithOrder();

    public abstract void beforeAll();

    public abstract void afterAll();

    public abstract void beforeEach();

    public abstract void afterEach();
}


