#ABOUT
This is the folder to install the Android related eviroment
Run
```
./build_android.sh   
```
It will download the Android SDK and NDK into $HOME/software directory.
Then it will set the android-cmake enviroment
At last it will build Boost and Openssl for Android platform

Currently, it works only for arm chip set. 

When the script finished, you can build the libsrch2-core.so in build directry: $build/ibs/armeabi-v7a 
by following command
```
cd ..
source ~/.bashrc
mkdir build
android-cmake ..
```

