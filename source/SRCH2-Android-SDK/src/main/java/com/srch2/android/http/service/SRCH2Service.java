package com.srch2.android.http.service;

import android.annotation.TargetApi;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.IBinder;
import android.util.Log;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.Semaphore;
import java.util.concurrent.atomic.AtomicBoolean;

final public class SRCH2Service extends Service {

    private static final String TAG = "Exe-Service";

    private class ExecutableServiceBroadcastReciever extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d("srch2:: " + TAG, "ExecutableServiceBroadcastReciever -- onRecieve");
            String value = intent.getStringExtra(IPCConstants.INTENT_KEY_START_AWAITING_SHUTDOWN);
            if (value.equals(IPCConstants.INTENT_KEY_START_AWAITING_SHUTDOWN)) {
                Log.d("srch2:: " + TAG, "ExecutableServiceBroadcastReciever -- onRecieve -- startAwaitingShutdown");
                startAwaitingShutdown();
            }
        }
    }

    private ExecutableServiceBroadcastReciever incomingIntentReciever;

    private String executableProcessPath;
    private String xmlConfigurationFilePath;
    private int executablePortNumber;
    private String executableShutdownUrlString;
    private String executableOAuthLiteral;

    private AtomicBoolean isAwaitingShutdown;
    private Timer awaitingShutdownTimer;
    private static int TIME_TO_WAIT_FOR_SHUTDOWN_MS = 60000;
    private static final int DEFAULT_TIME_TO_WAIT_FOR_SHUTDOWN_MS = 60000;

    private AtomicBoolean isShuttingDown;
    private Semaphore shutdownMutex;


    private final String PREFERENCES_NAME_SERVER_STARTED_LOG = "srch2-server-started-log";
    private final String PREFERENCES_KEY_SERVER_LOG_SHUTDOWN_URLS = "srch2-server-log-shutdown-urls";
    private final String PREFERENCES_KEY_SERVER_LOG_USED_PORT_NUMBER = "srch2-server-log-port-number";
    private final String PREFERENCES_KEY_SERVER_LOG_PREVIOUS_O_AUTH_CODE = "srch2-server-o-auth";
    private final String PREFERENCES_KEY_SERVER_LOG_EXECUTABLE_PATH = "exe-path";


    private final static String PREFERENCES_DEFAULT_NO_VALUE = "no-value";

    @Override
    public void onCreate() {
        super.onCreate();
        Log.d("srch2:: " + TAG, "onCreate");

        shutdownMutex = new Semaphore(1);

        isAwaitingShutdown = new AtomicBoolean(false);
        isShuttingDown = new AtomicBoolean(false);
        incomingIntentReciever = new ExecutableServiceBroadcastReciever();

        registerReceiver(incomingIntentReciever, IPCConstants.getSRCH2ServiceBroadcastRecieverIntentFilter(getApplicationContext()));

    }

    @Override
    public void onDestroy() {
        Log.d("srch2:: " + TAG, "onDestroy");
        try {
            unregisterReceiver(incomingIntentReciever);
        } catch (IllegalArgumentException ignore) {
        }
        super.onDestroy();
    }

    @TargetApi(Build.VERSION_CODES.ECLAIR)
    @Override
    public int onStartCommand(final Intent intent, int flags, int startId) {
        Log.d("srch2:: " + TAG, "onStartCommand");
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                resolveLifeCycleAction(intent);
            }
        });
        t.setName("SERVICE ON START COMMAND");
        t.start();
        return Service.START_STICKY;
    }

    private boolean checkIfProcessIsRunningWithoutHavingShutdownCalled() {

        Log.d("srch2:: " + TAG, "checkIfProcessIsRunningWithoutHavingShutdownCalled");

        SharedPreferences sharedpreferences = getSharedPreferences(PREFERENCES_NAME_SERVER_STARTED_LOG, Context.MODE_PRIVATE);
        String shutDownUrlsLiteral = sharedpreferences.getString(PREFERENCES_KEY_SERVER_LOG_SHUTDOWN_URLS, PREFERENCES_DEFAULT_NO_VALUE);
        int portNumber = sharedpreferences.getInt(PREFERENCES_KEY_SERVER_LOG_USED_PORT_NUMBER, 0);
        String oauth = sharedpreferences.getString(PREFERENCES_KEY_SERVER_LOG_PREVIOUS_O_AUTH_CODE, PREFERENCES_DEFAULT_NO_VALUE);
        String executablePath = sharedpreferences.getString(PREFERENCES_KEY_SERVER_LOG_EXECUTABLE_PATH, PREFERENCES_DEFAULT_NO_VALUE);

        if (!shutDownUrlsLiteral.equals(PREFERENCES_DEFAULT_NO_VALUE) && portNumber != 0 && !oauth.equals(PREFERENCES_DEFAULT_NO_VALUE) && !executablePath.equals(PREFERENCES_DEFAULT_NO_VALUE)) {

            executablePortNumber = portNumber;
            executableShutdownUrlString = shutDownUrlsLiteral;
            executableOAuthLiteral = oauth;
            executableProcessPath = executablePath;
            Log.d("srch2:: " + TAG, "checkIfProcessIsRunningWithoutHavingShutdownCalled was there with values " + portNumber + " " + shutDownUrlsLiteral);

            return true;
        } else {


            Log.d("srch2:: " + TAG, "checkIfProcessIsRunningWithoutHavingShutdownCalled was not there");

            return false;
        }
    }

    private void updateServerLog(int portNumberToPersist, String shutdownUrlToPersist, String oAuth, String executablePath) {
        Log.d("srch2:: " + TAG, "updateServerLog with port " + portNumberToPersist + " and shutdownurl " + shutdownUrlToPersist);


        SharedPreferences sharedpreferences = getSharedPreferences(PREFERENCES_NAME_SERVER_STARTED_LOG, Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedpreferences.edit();
        editor.putString(PREFERENCES_KEY_SERVER_LOG_SHUTDOWN_URLS, shutdownUrlToPersist);
        editor.putInt(PREFERENCES_KEY_SERVER_LOG_USED_PORT_NUMBER, portNumberToPersist);
        editor.putString(PREFERENCES_KEY_SERVER_LOG_PREVIOUS_O_AUTH_CODE, oAuth);
        editor.putString(PREFERENCES_KEY_SERVER_LOG_EXECUTABLE_PATH, executablePath);
        editor.commit();
    }

    private void clearServerLogEntries() {

        Log.d("srch2:: " + TAG, "clearServerLogEntries");


        SharedPreferences sharedpreferences = getSharedPreferences(PREFERENCES_NAME_SERVER_STARTED_LOG, Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = sharedpreferences.edit();
        editor.remove(PREFERENCES_KEY_SERVER_LOG_SHUTDOWN_URLS);
        editor.remove(PREFERENCES_KEY_SERVER_LOG_USED_PORT_NUMBER);
        editor.remove(PREFERENCES_KEY_SERVER_LOG_PREVIOUS_O_AUTH_CODE);
        editor.remove(PREFERENCES_KEY_SERVER_LOG_EXECUTABLE_PATH);
        editor.commit();
    }

    private void resolveLifeCycleAction(final Intent startCommandIntent) {
        Log.d("srch2:: " + TAG, "resolveLifeCycleAction");
        if (checkIfProcessIsRunningWithoutHavingShutdownCalled()) {
            Log.d("srch2:: " + TAG, "resolveLifeCycleAction - was running");
            stopAwaitingShutdown();
            if (isShuttingDown.get()) {
                Log.d("srch2:: " + TAG, "resolveLifeCycleAction - was shutting down");
                try {
                    Log.d("srch2:: " + TAG, "resolveLifeCycleAction - acquired mutex");
                    shutdownMutex.acquire();
                } catch (InterruptedException interruptedBySystem) {
                    interruptedBySystem.printStackTrace();
                    Log.d("srch2:: " + TAG, "resolveLifeCycleAction - MUTEX INTERRUPTED");
                    return;
                } finally {
                    Log.d("srch2:: " + TAG, "resolveLifeCycleAction - releasing mutex");
                    shutdownMutex.release();
                }
                Log.d("srch2:: " + TAG, "resolveLifeCycleAction - finished blocking on mutex, starting exectuabe...");
                startExecutable(startCommandIntent);
            } else {
                Log.d("srch2:: " + TAG, "resolveLifeCycleAction - was running without shutdown CONTINUE running");
                Log.d("srch2:: " + TAG, "resolveLifeCycleAction - CONTINUE RUNNING exe path " + executableProcessPath);

                signalSRCH2EngineToProceed(executablePortNumber, executableOAuthLiteral);
            }
        } else {
            Log.d("srch2:: " + TAG, "resolveLifeCycleAction - was not running starting");
            startExecutable(startCommandIntent);
        }
    }

    private void signalSRCH2EngineToProceed(int portNumberForSRCH2EngineToReuse, String oAuthCodeForSRCH2EngineToReuse) {
        Log.d("srch2:: " + TAG, "signalSRCH2EngineToProceed");
        Intent i = new Intent(IPCConstants.getSRCH2EngineBroadcastRecieverIntentAction(getApplicationContext()));
        i.putExtra(IPCConstants.INTENT_KEY_PORT_NUMBER, portNumberForSRCH2EngineToReuse);
        i.putExtra(IPCConstants.INTENT_KEY_OAUTH, oAuthCodeForSRCH2EngineToReuse);
        sendBroadcast(i);
    }

    private void stopAwaitingShutdown() {
        Log.d("srch2:: " + TAG, "stopAwaitingShutdown");
        if (awaitingShutdownTimer != null) {
            awaitingShutdownTimer.cancel();
            awaitingShutdownTimer = null;
        }
        isAwaitingShutdown.set(false);
    }


    private void startAwaitingShutdown() {
        Log.d("srch2:: " + TAG, "startAwaitingShutdown START AWAITING shutdown!");
        isAwaitingShutdown.set(true);
        awaitingShutdownTimer = new Timer();
        awaitingShutdownTimer.schedule(new TimerTask() {

            @Override
            public void run() {
                Thread.currentThread().setName("SHUTDOWN THREAD");
                if (!isAwaitingShutdown.get()) {
                    return;
                }
                Log.d("srch2:: " + TAG, "shutting down begin " + executableProcessPath);
                if (executableProcessPath != null) {
                    isShuttingDown.set(true);

                    try {
                        Log.d("srch2:: " + TAG, "shutting down begin - shutdownMutex acquired");
                        shutdownMutex.acquire();
                    } catch (InterruptedException ignore) {
                        Log.d("srch2:: " + TAG, "shutdowning MUTEX INTERRUPTED!");
                    }

                    try {

                        doShutdownNetworkCall();
                        Log.d("srch2:: " + TAG, "shutting down begin - about to enter while loop path is " + executableProcessPath);

                        int totalSleepTime = 0;
                        while (ps(executableProcessPath)) {
                            if (totalSleepTime % 400 == 0) {
                                Log.d("srch2:: " + TAG, "shutting down whiling while the ps is true");
                            }
                            if (totalSleepTime > TIME_TO_WAIT_FOR_SHUTDOWN_MS) {
                                Log.d("srch2:: " + TAG, "shuting down whileing ps is true breaking BREAKING");
                                break;
                            }

                            try {
                                Thread.currentThread().sleep(200);
                                totalSleepTime += 200;
                            } catch (InterruptedException e) {
                                Log.d("srch2:: " + TAG, "shutting down whiling while the ps is true INTERRUPED");
                                e.printStackTrace();
                            }
                        }
                        clearServerLogEntries();
                        } finally {
                            Log.d("srch2:: " + TAG, "shutting down finally block finished - releasing and setting to false");
                            shutdownMutex.release();
                            isShuttingDown.set(false);
                        }
                Log.d("srch2:: " + TAG, "shutting down finished - about to check to stop self");

                checkToStopSelf();
                }
            }
        }, TIME_TO_WAIT_FOR_SHUTDOWN_MS);
    }


    private void doShutdownNetworkCall() {
        Log.d("srch2:: " + TAG, "doShutdownNetworkCall");
        URL url = null;
        try {
            url = new URL(executableShutdownUrlString);
        } catch (MalformedURLException ignore) {
        }

        InputStream is = null;
        HttpURLConnection connection = null;
        try {
            connection = (HttpURLConnection) url.openConnection();
            connection.setDoOutput(true);
            connection.setRequestMethod("PUT");
            connection.setUseCaches(false);
            connection.connect();
            OutputStreamWriter out = new OutputStreamWriter(
                    connection.getOutputStream());
            out.write("X");
            out.close();

            int responseCode = connection.getResponseCode();

            Log.d("srch2:: " + TAG, "doShutdownNetworkCall - responseCode " + responseCode);


            String response = "";
            if (connection.getInputStream() != null) {
                response = readInputStream(connection.getInputStream());
            } else if (connection.getErrorStream() != null) {
                response = readInputStream(connection.getErrorStream());
            }

            Log.d("srch2:: " + TAG, "doShutdownNetworkCall response " + response);


        } catch (IOException networkError) {
            networkError.printStackTrace();
            Log.d("srch2:: " + TAG, "network error WHILE shutting down " + networkError.getMessage());
        } finally {
            if (connection != null) {
                connection.disconnect();
            }
        }
    }

    private void checkToStopSelf() {
        Log.d("srch2:: " + TAG, "checkToStopSelf");
        if (isAwaitingShutdown.get()) {
            Log.d("srch2:: " + TAG, "checkToStopSelf - stopping service");
            stopSelf();
        }
    }

    private void startExecutable(Intent startCommandIntent) {
        Log.d("srch2:: " + TAG, "startExecutable");
        executablePortNumber = startCommandIntent.getIntExtra(IPCConstants.INTENT_KEY_PORT_NUMBER, 0);
        executableShutdownUrlString = startCommandIntent.getStringExtra(IPCConstants.INTENT_KEY_SHUTDOWN_URL);
        executableOAuthLiteral = startCommandIntent.getStringExtra(IPCConstants.INTENT_KEY_OAUTH);
        boolean isDebugAndTestingMode = startCommandIntent.getBooleanExtra(IPCConstants.INTENT_KEY_IS_DEBUG_AND_TESTING_MODE, false);
        TIME_TO_WAIT_FOR_SHUTDOWN_MS = isDebugAndTestingMode ? 0 : DEFAULT_TIME_TO_WAIT_FOR_SHUTDOWN_MS;

        autoInstallCoreFilesAndOverwriteXMLConfigurationFile(startCommandIntent.getStringExtra(IPCConstants.INTENT_KEY_XML_CONFIGURATION_FILE_LITERAL));
        startRunningExecutable(executablePortNumber, executableShutdownUrlString, executableOAuthLiteral);

        Log.d("srch2:: " + TAG, "startExecutable port number " + executablePortNumber);
        Log.d("srch2:: " + TAG, "startExecutable shutdown string " + executableShutdownUrlString);

        int totalSleepTime = 0;
        while (!ps(executableProcessPath)) {
            try {
                Thread.currentThread().sleep(200);
                if (totalSleepTime > 1000) {
                    break;
                }
                totalSleepTime += 200;
            } catch (InterruptedException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
        signalSRCH2EngineToProceed(executablePortNumber, executableOAuthLiteral);
    }

    private void startRunningExecutable(final int portBeingUsedToStartService, final String shutDownUrl, final String oAuthCode) {
        Log.d("srch2:: " + TAG, "startRunningExecutable");
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                ProcessBuilder pb = new ProcessBuilder(executableProcessPath, "--config-file", xmlConfigurationFilePath);
                Process p;
                try {
                    updateServerLog(portBeingUsedToStartService, shutDownUrl, oAuthCode, executableProcessPath);

                    Log.d("srch2:: " + TAG, "startRunningExecutable - starting process");
                    p = pb.start();

                    if (p.getInputStream() != null) {
                        Log.d("srch2:: " + TAG, "PRINTING INPUT STREAM\n" + readInputStream(p.getInputStream()));
                    } else {
                        Log.d("srch2:: " + TAG, "NO INPUT STREAM from process");
                    }

                    if (p.getErrorStream() != null) {
                        Log.d("srch2:: " + TAG, "PRINTING ERROR STREAM\n" + readInputStream(p.getErrorStream()));
                    } else {
                        Log.d("srch2:: " + TAG, "NO ERROR STREAM from process");
                    }

                    p.destroy();

                } catch (IOException e) {
                    Log.d("srch2:: " + TAG, "IOEXCEPTION starting executable!");
                    e.printStackTrace();
                }
            }
        });
        t.setName("EXECUTABLE PROCESS THREAD");
        t.start();
    }

    @TargetApi(Build.VERSION_CODES.DONUT)
    private void autoInstallCoreFilesAndOverwriteXMLConfigurationFile(String xmlConfigurationFileLiteral) { //verify chmod bitmask to use
        Log.d("srch2:: " + TAG, "autoInstallCoreFilesAndOverwriteXMLConfigurationFile");
        final Context c = getApplicationContext();
        String dataDirectoryFilePath = c.getApplicationInfo().dataDir;
        File filesDirectory = c.getFilesDir();

        try {
            File binDirectory = new File(dataDirectoryFilePath, "bin");
            if (!binDirectory.exists()) {
                binDirectory.mkdir();
                chmod("755", binDirectory.getAbsolutePath());
            }

            File srch2RootDirectory = new File(filesDirectory, "srch2");
            if (!srch2RootDirectory.exists()) {
                srch2RootDirectory.mkdir();
                chmod("755", srch2RootDirectory.getAbsolutePath());
            }

            File stopWords = new File(srch2RootDirectory, "stop-words.txt");
            if (!stopWords.exists()) {
                InputStream sourceFile = c.getResources().openRawResource(R.raw.stopwords);
                FileOutputStream destinationFile = new FileOutputStream(stopWords);
                copyStream(destinationFile, sourceFile);
            }

            File executableBinary = new File(binDirectory, "srch2ngn.exe");
            if (!executableBinary.exists()) {
                InputStream sourceFile = c.getResources().openRawResource(R.raw.srch2engine434);
                FileOutputStream destinationFile = new FileOutputStream(executableBinary);
                copyStream(destinationFile, sourceFile);
                chmod("775", executableBinary.getAbsolutePath());
            }
            executableProcessPath = executableBinary.getAbsolutePath();
            Log.d("srch2:: " + TAG, "autoInstallCoreFilesAndOverwriteXMLConfigurationFile - executableProcessPath " + executableProcessPath);

            File configFile = new File(srch2RootDirectory, "srch2-config.xml");
            if (!configFile.exists()) {
                configFile.createNewFile();
            }
            BufferedWriter buf = new BufferedWriter(new FileWriter(configFile));
            buf.write(xmlConfigurationFileLiteral);
            buf.close();
            xmlConfigurationFilePath = configFile.getAbsolutePath();
            Log.d("srch2:: " + TAG, "autoInstallCoreFilesAndOverwriteXMLConfigurationFile - xmlConfigurationFilePath " + xmlConfigurationFilePath);
        } catch (Exception ignore) {
            ignore.printStackTrace();
        }
    }

    private boolean ps(String executablePathToCheck) {
        boolean isAlreadyRunning = false;
        ProcessBuilder checkPb = new ProcessBuilder("ps");
        Process checkP;
        try {
            checkP = checkPb.start();
            String listing = readInputStream(checkP.getInputStream());
            if (listing.contains(executablePathToCheck)) {
                isAlreadyRunning = true;
            } else {
                // not already running
            }
        } catch (IOException ignore) {
        }
        return isAlreadyRunning;
    }

    private void copyStream(OutputStream target, InputStream source) throws IOException {
        byte[] buffer = new byte[4096];
        int bytesRead;
        while ((bytesRead = source.read(buffer)) >= 0) {
            target.write(buffer, 0, bytesRead);
        }
        target.close();
    }

    private void chmod(String... args) throws IOException {
        String[] cmdline = new String[args.length + 1];
        cmdline[0] = "/system/bin/chmod";
        System.arraycopy(args, 0, cmdline, 1, args.length);
        new ProcessBuilder(cmdline).start();
    }

    private static String readInputStream(InputStream source) throws IOException {
        if (source == null) {
            return "NULL SOURCE INPUT STREAM";
        }
        StringBuilder sb = new StringBuilder();
        String line;
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new InputStreamReader(source, "UTF-8"));
            while ((line = reader.readLine()) != null) {
                sb.append(line);
            }
        } finally {
            if (reader != null) {
                reader.close();
            }
        }
        return sb.toString();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
