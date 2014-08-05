package com.srch2.android.http.service;


import android.os.Bundle;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

public class MySingleLaunchActivity extends  MyActivity{


    public static final String MY_TAG = "SingleLaunchActiviyTest";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setTitle(MY_TAG);


        Log.d(MY_TAG, "onCreate");

        super.initializeSRCH2Engine();
    }

    @Override
    protected void onResume() {
        super.onResume();

        Log.d(MY_TAG, "onResume");

        super.callSRCH2EngineStart();
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(MY_TAG, "onPause");
        super.callSRCH2EngineStop();

    }

}
