# Overview

**Note these projects must be opened with the correct IDE.**

- Hello-SRCH2-Search-Demo -> is an Android Studio project (navigate to this folder, and import from Android Studio)
- SRCH2-Android-SDK -> is an IntelliJ IDEA project (use pom.xml to import)
- SRCH2-Service-Demo -> is an Eclipse project (can be imported into either IntelliJ or Android Studio)

Note: mvn package -DskipTests in project source will create output builds (aar, apk) in /target/

# How to import the srch2-android-sdk.aar 

## In gradle

1. Add the `srch2.com/repo/maven` into the repositories.

We need to add this repo both inside and outside the `buildScript` block.
The `buildscript` block only sets up the repositories for your build script but not your application.

```
buildscripte{
    repositories {
        jcenter()
        maven { url 'http://srch2.com/repo/maven' }
    }
    dependencies {
        classpath 'com.android.tools.build:gradle:0.12.+'

        // NOTE: Do not place your application dependencies here; they belong
        // in the individual module build.gradle files
    }
}

allprojects {
    repositories {
        jcenter()
        maven { url 'http://srch2.com/repo/maven' }
    }
}
```

2. Add the dependency on the `srch2-android-sdk.aar` in the individual module build.gradle file 
```
dependencies {
    ...
    compile group: 'com.srch2', name: 'srch2-android-sdk', version: '1.0.0-SNAPSHOT',  ext:'aar'
}
```
