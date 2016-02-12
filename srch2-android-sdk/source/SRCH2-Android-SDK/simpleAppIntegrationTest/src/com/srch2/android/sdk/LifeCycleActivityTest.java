package com.srch2.android.sdk;

public class LifeCycleActivityTest extends AbstractActivityTest<LifeCycleTestActivity> {
    static final String TAG = "SRCH2 LifeCycleActivity";

    public LifeCycleActivityTest(){
        super(LifeCycleTestActivity.class.getPackage().getName(), LifeCycleTestActivity.class);
    }

    @Override
    String getTAG() {
        return TAG;
    }
}
