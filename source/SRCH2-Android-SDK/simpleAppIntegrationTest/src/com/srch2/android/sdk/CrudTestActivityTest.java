package com.srch2.android.sdk;

public class CrudTestActivityTest extends AbstractTest<CrudTestActivity>{
    static final String TAG = "SRCH2ActivityTEST";

    public CrudTestActivityTest(){

        super(CrudTestActivity.class.getPackage().getName(), CrudTestActivity.class);
    }

    @Override
    String getTAG() {
        return TAG;
    }
}
