Hello Again. Welcome to the SRCH2 Android SDK Advanced Topics Page.

###Plan of Action
```
Step 1: Make SDK
Step 2: ???
Step 3: Profit!
```
No one inside the organization knows what step two is, but assumes someone else inside the organization does. 

###Powerful Search API

// to do

###GeoSearch

// to do

###Testing

The SRCH2 Android SDK must be tested on a real device: the emulator does not support running the SRCH2 http server. When you initialize the `SRCH2Engine`, you can call `setDebugAndTestMode(true)` to enable the `SRCH2Engine` to quickly start and stop the SRCH2 http server. 

###Configuring for Proguard

Configuring the SRCH2-Android-SDK for Proguard is easy. Just add:

```
-keep class com.srch2.** { *; } 
-keep interface com.srch2.** { *; } 
-keep enum com.srch2.** { *; } 
-dontwarn class com.srch2.** { *; } 
-dontwarn interface com.srch2.** { *; } 
-dontwarn enum com.srch2.** { *; } 
```

to your proguard configuration file.

###Using the Eclipse IDE	
	
// to do