
# Follow these steps to setup the Bimaple development environment
# Chen Li, July 2012

cd trunk/bimaple-engine
\rm -fr build
./runme-to-init-env.sh
# In the process it will start building the whole package
# It's OK to kill the compiling process
cd build
cmake -DBUILD_RELEASE=OFF ..
make -j 4; make

# to setup eclipse
cmake -G "Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug .
Then open eclipse

# For the first time, we need to install CDT for Eclipse

Eclipse -> Help -> "Install New Software"
Click "Available Software Sites"
Type in "CDT", and select "CDT http://download.eclipse.org/tools/cdt/releases/galileo".  Click the "Enable" button and "OK".
Check "CDT Main Features", and check "Eclipse C/C++ Development Tools".  Uncheck "Eclipse C/C++ Development Tools SDK". Click "OK".

# Import Bimaple project in Eclipse

File -> Import -> General -> Existing Projects into Workspace -> Next -> Browse -> the "trunk" folder -> OK

# initilize CDT
In the "trunk" folder, run the following:
cmake -G "Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug .

# setup the project environment

Right click the Bimaple project, click "Properties" -> "C/C++ Make Project".  In "Build Director", add "/build" to the default path.

Then press "Ctr-B" to build the project

To test the engine, please first set the "license-file" option in "bimaple-engine/wrapper/conf/conf.ini", and then run "ctest" under the "build" folder.

# Possible Compiling Errors
1. Your compiler may not support "-Wno-implicit". To fix it, you can remove it from "CMakeLists.txt". 
2. Your compiler may not find "libdl.a" in "/usr/lib/". It might be in "/usr/lib32". To fix it, you can copy "/usr/lib32/libdl.a" into "/usr/lib".
3. When compiling "jsoncpp-src-0.5.0/libs", there would generate a folder called "linux-gcc-4.7" (4.7 is supposed to be your gcc version), but if your gcc is not 4.7, say it is 4.7.2, the compiler will not find the folder "linux-gcc-4.7". To fix it, you can copy "linux-gcc-4.7" to "linux-gcc-4.7.2".   

