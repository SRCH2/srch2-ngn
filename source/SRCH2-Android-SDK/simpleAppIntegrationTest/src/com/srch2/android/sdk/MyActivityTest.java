package com.srch2.android.sdk;

/**
 * Created by jianfeng on 8/6/14.
 */
public class MyActivityTest extends AbstractTest<MyActivity>{
    static final String TAG = "SRCH2ActivityTEST";

    public MyActivityTest(){

        super(MyActivity.class.getPackage().getName(), MyActivity.class);
    }

    @Override
    String getTAG() {
        return TAG;
    }
}
