package com.srch2.android.http.service.test;

import android.test.SingleLaunchActivityTestCase;
import android.util.Log;
import android.widget.Toast;
import com.srch2.android.sdk.TestableActivity;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by jianfeng on 8/7/14.
 */
public abstract class AbstractTest<T extends TestableActivity> extends SingleLaunchActivityTestCase<T> {

    abstract String getTAG();
    /**
     * <b>NOTE:</b> The parameter <i>pkg</i> must refer to the package identifier of the
     * package hosting the activity to be launched, which is specified in the AndroidManifest.xml
     * file.  This is not necessarily the same as the java package name.
     *
     * @param pkg           The package hosting the activity to be launched.
     * @param activityClass The activity to test.
     */
    public AbstractTest(String pkg, Class<T> activityClass) {
        super(pkg, activityClass);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
    }

    public void testAll() throws Throwable {
        T activity = getActivity();
        runTest(activity);
    }

    private void runTest(T activity) throws Throwable {
        showBeginAll(activity);
        activity.beforeAll();

        int cSuccess = 0;
        int cFail = 0;
        Class clazz = activity.getClass();
        List<String> methodNames = activity.getTestMethodNameListWithOrder();
        ArrayList<AssertionError> errors = new ArrayList<AssertionError>();
        ArrayList<Exception> exceptions= new ArrayList<Exception>();
        for( String methodName: methodNames) {
            boolean success = false;
            try {
                showBeforeEach(methodName);
                activity.beforeEach();
                Method method = clazz.getMethod(methodName);
                method.invoke(activity);
                success = true;
            } catch (NoSuchMethodException e) {
                exceptions.add(e);
                showError(e);
            } catch (AssertionError e) {
                errors.add(e);
                showError(e);
            } catch (Exception e){
                exceptions.add(e);
                showError(e);
            } finally {
                activity.afterEach();
                showAfterEach(methodName, success);
                cSuccess += success ? 1 : 0;
                cFail += success ? 0 : 1;
            }
        }
        activity.afterAll();
        showAfterAll(activity, cSuccess, cFail );

        // throw them afterward to call afterAll()
        for(AssertionError e : errors){
            throw e;
        }

        for(Exception e :exceptions){
            throw e.getCause();
        }
    }

    private void showError(AssertionError e) {
        e.printStackTrace();
        String message = e.getMessage();
        if (message == null){
            message = "AssertionError unknown reason";
        }
        makeLog(message);
    }

    private void showError(Exception e) {
        e.printStackTrace();
        String message = e.getMessage();
        if (message == null){
            message = "Exception happens. No message provided";
        }
        makeLog(message);
    }

    private void showBeforeEach(String methodName){
        String message = methodName + "test start";
        makeLog(message);
    }

    private void showAfterEach(String methodName, boolean success) {
        String message = methodName + (success ? " test passed." : " test failed.");
        makeLog(message);
    }


    private void showBeginAll(T activity) {
        String message = activity.getPackageName() + activity.getTitle() + " test start";
        makeLog(message);
   }

   private void showAfterAll(T activity, int cSuccess, int cFail) {
       String message = activity.getPackageName() + activity.getTitle() + " test over";
       message += " success :" + cSuccess + " failed:" + cFail;
       makeLog(message);
   }

    private void makeLog(String message){
        Log.d(getTAG(), message);
        Toast.makeText(getActivity().getApplicationContext(), message, Toast.LENGTH_LONG).show();
    }

 }
