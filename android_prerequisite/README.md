#ABOUT
This is the folder to install the Android related eviroment
Run
```
./setup_android_env.sh
```
It will download the Android SDK and NDK into $HOME/android directory.
Then it will set the android-cmake enviroment
At last it will build Boost and Openssl for Android platform

Currently, it works only for arm chip set. 

After the setup is finished, you can build the libsrch2-core.so in build directry: $build/ibs/armeabi-v7a 
by following command
```
cd ..
source ~/.bashrc
mkdir build
android-cmake ..
```

or build srch2 standalone server for android. Make sure you have built the thirdparty depenenencies 
using "thirdparty-build-android.sh" in the thirdparty folder

cd <trunk_root>
mkdir build
android-cmake -DANDROID_SERVER=1 ..

