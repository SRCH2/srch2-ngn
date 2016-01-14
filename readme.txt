
Compile SRCH2 codebase (64-bit Linux)

Step 1. Clone the srch2-ngn repository to your local system. 

Step 2.
    cd [/path/to/srch2-engine]
    sh ./runme-to-init-env.sh

It will compile related libraries and build the engine. 
After the command is finished, you can find the binary files in the "build/" folder. 
The executable is located under src/server and other test binaries are also 
included in the "build/" folder. If you have already the "build/" folder, please 
remove it before using runme-to-init-env.sh file.

  Possible errors:

  - The compiler may not support "-Wno-implicit". To fix it, you can remove 
    the flag from "CMakeLists.txt".

  - The compiler may not find "libdl.a" in "/usr/lib/". It might be in "/usr/lib32". 
    To fix it, you can copy "/usr/lib32/libdl.a" into "/usr/lib".

  - When compiling "jsoncpp-src-0.5.0/libs", there would generate a folder 
    called "linux-gcc-4.7" (4.7 is supposed to be your gcc version), but if your gcc 
    is not 4.7, say it is 4.7.2, the compiler will not find the folder 
    "linux-gcc-4.7". To fix it, you can copy "linux-gcc-4.7" to "linux-gcc-4.7.2".

  - If you see an error "Unable to find the requested Boost libraries. 
    Boost version: 1.46.1", run the following command:

    sudo apt-get install libboost1.46-all-dev

Step 3. Run ctest

     shell> cd build
     shell> ctest

  It will execute the contained tests in the build folder.

Step 4. (Optional) Changing the compiling mode: 
By default, the code is compiled in the release mode, which does not have debug 
options and asserts. For development purposes, you have to compile it in the
debug mode. So go to "build/" folder and run the following command:

   shell> cmake -DBUILD_RELEASE=OFF ..

  To compile it using the release mode:

   shell> cmake -DBUILD_RELEASE=ON ..

Step 5. (Optional) Importing the source code as a project into Eclipse.

After installing Eclipse CDT, using the following shell command to create the
necessary files to be able to import the codebase as an existing project into Eclipse.

   shell> cmake -G "Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug . 
