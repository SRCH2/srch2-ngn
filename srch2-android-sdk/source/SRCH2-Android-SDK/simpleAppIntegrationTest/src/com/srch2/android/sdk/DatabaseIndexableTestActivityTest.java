package com.srch2.android.sdk;

public class DatabaseIndexableTestActivityTest extends AbstractActivityTest<DatabaseTestActivity> {
    static final String TAG = "SRCH2 DatabaseIndexableTest";

    public DatabaseIndexableTestActivityTest(){
        super(DatabaseTestActivity.class.getPackage().getName(), DatabaseTestActivity.class);
    }

    @Override
    String getTAG() {
        return TAG;
    }
}
