package com.srch2.android.sdk;

public class DirtyIndexableTestActivityTest extends AbstractActivityTest<DirtyIndexableTestActivity> {
    static final String TAG = "SRCH2 LifeCycleActivity";

    public DirtyIndexableTestActivityTest(){
        super(DirtyIndexableTestActivity.class.getPackage().getName(), DirtyIndexableTestActivity.class);
    }

    @Override
    String getTAG() {
        return TAG;
    }
}
