package com.srch2.android.http.service.test2;

import android.test.SingleLaunchActivityTestCase;
import com.srch2.android.http.service.MyActivity;

/**
 * Created by jianfeng on 8/6/14.
 */
public class MyActivityTest  extends SingleLaunchActivityTestCase<MyActivity> {

    MyActivity activity;

    public MyActivityTest(){
        super("com.srch2.android.http.service", MyActivity.class);
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();
        activity = getActivity();
    }

    public void testAll(){
        activity.initializeSRCH2EngineAndCallStart();
        activity.testAll();
        activity.callSRCH2EngineStop();
    }
}
