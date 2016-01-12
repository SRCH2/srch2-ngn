This readme file has four parts:
1. Setup the development environment -- Linux
2. Setup the development environment -- MacOS
3. Getting started with Git
4. Development and test of the codebase

######################################################################################################################################
######################################################################################################################################
1. Setup the development environment -- Linux
Prerequisite:

1. If you are using Ubuntu, make sure your user account has super user privilege.

2. Make sure you have assembla account setup. Username and password used below are of assembla  account ( not Confluence)

3. Please make sure you have gcc 4.6 or later version for development.

3.a) Make sure to have g++

4. As of April 2013, we have cleaned up the codebase to speedup the compiling process. As a consequence, we need to use cmake with a version at least 2.8.8. To install a latest cmake, do the following:
Install cmake 2.8.8 if it's not available on your machine:

    -- Go to http://www.cmake.org/cmake/resources/software.html, download a cmake package at least 2.8.8 and save it to a local folder;

    -- Run "tar xcvf" to unzip the tarball to a local folder, e.g., /home/chenli/cmake-2.8.10.2-Linux-i386. Modify the ~/.bashrc file and add the following line to make sure we use this cmake package:

       export PATH="/home/chenli/cmake-2.8.10.2-Linux-i386/bin:$PATH"
    -- Restart the bash.
    -- After that, run "which cmake" and "cmake --version" to confirm if the right cmake version is on your PATH.
    -- The package could be 32-bit. If your OS is a 64-bit OS, run the following command to do install the ia32-libs libraries, which allow you to run the 32-bit process on your 64-bit OS:
       sudo apt-get install ia32-libs
######################################################################################################################################
######################################################################################################################################
2. Setup the development environment -- MacOS
The document describes how to how to compile the SRCH2 engine on MacOS for development. You only need to run the following steps on the machine once.

Step 1: Install Xcode

    Open the “App Store” app;

    Search for “xCode”;

    Hit the “Install” button.

Step 2: Install “Command Line Tools”

    Open xCode.

    Go to “Preferences” by hitting (command + ’,’)

    “Downloads” -> “Components” -> Install the “Command Line Tools”

Step 3: Install boost

- Download Boost from http://sourceforge.net/projects/boost/files/boost/1.50.0/

 Any of the packages should be fine.  Let's download "boost_1_50_0.tar.gz".

 Download it to the folder ~/tmp.



- Go to the ~/tmp folder, extract it and go inside the folder

 ~/tmp> tar xvf boost_1_50_0.tar.gz



- Run these commands in a shell:


~/tmp/boost_1_50_0> ./bootstrap. sh --prefix=/usr/local

~/tmp/boost_1_50_0> ./b2

~/tmp/boost_1_50_0> sudo ./b2 install



Step 4: Install openssl



- Download openssl library http://www.openssl.org/source/openssl-1.0.1e.tar.gz

 Put into the folder ~/tmp.



- Type in the following:



cd ~/tmp/

~/tmp/> tar xvf openssl-1.0.1e.tar.gz

~/tmp/> cd openssl-1.0.1e

~/tmp/openssl-1.0.1e/> ./config --prefix=/usr/local

~/tmp/openssl-1.0.1e/> make

~/tmp/openssl-1.0.1e/> sudo make install



Step 5: Install cmake

Download http://www.cmake.org/files/v2.8/cmake-2.8.11.1-Darwin64-universal.dmg

Install it as a Mac application.


Step 6 (optional): Remove all the downloaded files under ~/tmp



shell> cd ~/tmp

~/tmp> \rm -fr boost_1_50_0.tar.gz

~/tmp> \rm -fr boost_1_50

~/tmp> \rm -fr openssl-1.0.1e.tar.gz

~/tmp> \rm -fr openssl-1.0.1e




Setting up Debugger on eclipse on Mac:

In Mac, gcc seems to build 64-bit executables by default and Eclipse is not able to recognize them. So we need to change the "Binary Parser" to 'Mach-O 64 Parser' in Eclipse.

To change the Binary Parser, follow the steps below:

    Click on "Project" -> "Properties" -> "C/C++ Make Project".
    On the right pane, select "Binary Parser" and then check only the "Mach-O 64 Parser".

Now, you should be able to debug the code using the Eclipse debugger.

######################################################################################################################################
######################################################################################################################################
3. Getting started with Git
This is what we need to do:
Step 1. Clone the srch2-ngn repository on you local system. Go to https://bitbucket.org/???????. Click the "Clone" button.  Copy the command and run it on the local machine.
Step 2.

    checkout your branch
        git checkout <branchname>
            The git checkout command lets you navigate between the branches. Read more about git checkout
    now you can do the changes to your branch. Once you are done with your changes you need to merge this branch with your master branch. Follow the following commands
        git checkout master
            this will switch your working copy to match with the master branch.
    git merge <branchname>
        this will merge your branch into the master branch. If you see conflicts, you will have to resolve them. Don't forget to stage the resolved files for the next commit.  use following command to stage the files
    git add .
        '.' will stage all the files for the next commit, you may also use git add <filename> if you want to stage specific files only. Once you are ready write the following command
    git commit -m "enter your merge message here"
    compile your code and make sure it is working. Once this step is done follow the next step.
    git push
        push your changes to the repository

Step 3.

    create a pull request. Read here to know about pull request

        the master in the srch2-ngn repository might have changed during the time you were working on your fork. Make sure you have merged the changes from the srch2-ngn before creating the pull request.
        please make sure to add reviewers. This is important to do the code review.

Step 4.
The developer managing the  srch2-ngn repository will review your changes and will merge the changes. If everything works fine he will accept the pull request.
Congratulations. You are done.
Keep reading to add SSH for Git.
Setting SSH for Git on Mac/Linux

This will tell you how to use secure shell (SSH) to communicate with the Bitbucket server and avoid having to manually type a password every time.

Also check the following page  https://confluence.atlassian.com/pages/viewpage.action?pageId=270827678&src=email titled "Set up SSH for Git and Mercurial on Mac OSX/Linux."
Steps:

    ensure you have ssh installed. open terminal and enter



ssh -v



    If you have ssh installed, go to the next step. If you don't have ssh installed, install it now.
    List the contents of your ~/.ssh directory
    If you have defined a default identity, you'll see the two id_* files:

ls -a ~/.ssh
.        ..        id_rsa        id_rsa.pub    known_hosts

    if you don't see id_rsa and id_rsa.pub then go to the url (https://confluence.atlassian.com/pages/viewpage.action?pageId=270827678#SetupSSHforGitandMercurialonMacOSX%2FLinux-Step3.Setupyourdefaultidentity) and follow the instructions of "Step 3" on that page. Then go to the next step
    log into Bitbucket website and  go to  avatar > Manage Account   (avatar is your image on the top right corner).
    Click SSH keys, The SSH Keys page displays. It shows a list of any existing keys. Then, below that, a dialog for labeling and entering a new key.
    In your terminal enter following and copy the contents of your public key file



cat ~/.ssh/id_rsa.pub

    Back in your browser, enter a Label for your new key, for example, Default public key and Paste the copied public key into the SSH Key field

    add the key
    Step 7. Change your repo from HTTPS to the SSH protocol

    The URL you use for a repo depends on which protocol you are using, HTTPS or SSH.  The Bitbucket repository Overview page has a quick way for you to see these URLS for the srch2-ngn  repo.  On the repo's Overview page look for the Clone button.

    change the url from https to ssh and use this new url for all future purposes.

######################################################################################################################################
######################################################################################################################################
4. Development and test of the codebase

This page assumes you have setup the development environment (either on Linux or MacOS).

Getting and Building the Code

    git clone git@bitbucket.org:srch2inc/srch2-ngn.git

Install packages needed for building the engine.

    cd /path/to/srch2-engine

    sh runme-to-init-env.sh

NOTE: If you faced errors while compiling the code, read the following instructions:

    Your compiler may not support "-Wno-implicit". To fix it, you can remove it from "CMakeLists.txt".
    Your compiler may not find "libdl.a" in "/usr/lib/". It might be in "/usr/lib32". To fix it, you can copy "/usr/lib32/libdl.a" into "/usr/lib".
    When compiling "jsoncpp-src-0.5.0/libs", there would generate a folder called "linux-gcc-4.7" (4.7 is supposed to be your gcc version), but if your gcc is not 4.7, say it is 4.7.2, the compiler will not find the folder "linux-gcc-4.7". To fix it, you can copy "linux-gcc-4.7" to "linux-gcc-4.7.2".

NOTE: If you have already the "build" folder, please remove it before using runme-to-init-env.sh file.

The runme-to-init-env.sh script will start to build the engine in the end automatically. After the command finished, you can find the binary files in the build folder. The executable is located under src/server and other test binaries are also included in the build folder.

If you see an error "Unable to find the requested Boost libraries.  Boost version: 1.46.1", run the following command:



    sudo apt-get install libboost1.46-all-dev


Run ctest

Goto the build folder and type ctest . This will execute the contained tests in the build folder.
Building in Debug Mode

By default, the code is compiled in release mode which does not have debug options and asserts. For development purposes, you have to compile it in debug mode. So go to "build" folder and run the following command:

    cmake -DBUILD_RELEASE=OFF ..

This command creates the makefiles and you can use the command "make" in "build" folder to compile the code.

NOTE: you can find more about building options in the more building options section
Development with Eclipse

NOTE: You have to follow "building in debug mode" section before going through this section.

Download and run the latest stable version of Eclipse CDT at http://www.eclipse.org/cdt/downloads.php.  Make sure that you download the correct 32 or 64 bit eclipse CDT.  You can check the version of Ubuntu using the following command : "uname -a"

Run another script to prepare the codebase for Eclipse.

    sh eclipse-bootstrap.sh

Import SRCH2 project in Eclipse.

    File -> Import -> General -> Existing Projects into Workspace -> Next -> Browse -> "srch2-engine" folder -> OK

    Right click the SRCH2 project, click "Properties" -> "C/C++ Make Project".

        In the "Build Directory" field, append "/build" to the default path.
        Change the "Build command" variable to : /usr/bin/make -C [ProjectLoc]/build, where "[ProjectLoc]" is the root of the source code.
        Clear the field for "Build (Incremental Build)". (It is "all" by default and you have to make it empty.)


Go to a terminal, go to the build folder, then type in the following:

    cmake -DBUILD_RELEASE=OFF ..


After making some changes, "Refresh" on Eclipse.  Then press "Ctr-B" to build the engine.



More Building Options

To change back to release mode:

    cmake -DBUILD_RELEASE=ON ..

Check out here for more build options.

For fresh build , you could choose to parallelize the build process using following command

make -jN ( where N is the number of cores/cpu on your machine)

Note: The build dependencies are not setup properly for parallel build. You would see an error while running a parallel build. There are few workarounds which are mentioned below.
~/srch2-engine/build$ cmake ..
~/srch2-engine/build$ make -j 4; make
or
~/srch2-engine/build$cmake ..
~/srch2-engine/build$make srch2_instantsearch/fast -j4   ( Builds core srch2 library in parallel. /FAST option skips dependencies check)
~/srch2/build$make -j4                                  ( Builds again the whole stuff in parallel. Core library is already built in previous step )
######################################################################################################################################
