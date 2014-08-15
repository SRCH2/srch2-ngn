package com.srch2.android.http.service.test;

import com.srch2.android.sdk.MyActivity;

/**
 * Created by jianfeng on 8/6/14.
 */
public class MyActivityTest extends AbstractTest<MyActivity>{
    static final String TAG = "SRCH2ActivityTEST";

    public MyActivityTest(){
        super("com.srch2.android.http.service", MyActivity.class);
    }

    @Override
    String getTAG() {
        return TAG;
    }
}
