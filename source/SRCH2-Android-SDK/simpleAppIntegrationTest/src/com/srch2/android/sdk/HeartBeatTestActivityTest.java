package com.srch2.android.sdk;

public class HeartBeatTestActivityTest extends AbstractTest<HeartBeatTestActivity>{
    static final String TAG = "SRCH2 LifeCycleActivity";

    public HeartBeatTestActivityTest(){
        super(HeartBeatTestActivity.class.getPackage().getName(), HeartBeatTestActivity.class);
    }

    @Override
    String getTAG() {
        return TAG;
    }
}
