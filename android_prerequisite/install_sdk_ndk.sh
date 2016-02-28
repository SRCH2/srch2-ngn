#!/bin/bash - 
#===============================================================================
#
#          FILE: install_sdk_ndk.sh
# 
#         USAGE: ./install_sdk_ndk.sh 
# 
#   DESCRIPTION: install android sdk ndk enviroment.
#                change based on https://gist.github.com/tahl/1026610
# 
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: Jianfeng Jia ()
#       CREATED: 08/20/2013 10:24:15 PM PDT
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error
install_adb=false                           # Set this to true if you want to set up adb 
                                            # to develop Android application other than just 
                                            # build the ndk project.

[ ! -d "$HOME/software" ] && mkdir $HOME/software

#Download and install the Android SDK
if [ ! -d $ANDROID_SDK_HOME ]; then
	for a in $( wget -qO- http://developer.android.com/sdk/index.html | egrep -o "http://dl.google.com[^\"']*linux.tgz" ); do 
		wget $a && tar --wildcards --no-anchored -xvzf android-sdk_*-linux.tgz; mv android-sdk-linux $ANDROID_SDK_HOME; rm android-sdk_*-linux.tgz;
	done
else
     echo "Android SDK already installed to $ANDROID_SDK_HOME.  Skipping."
fi

#Download and install the Android NDK
if [ ! -d "$ANDROID_NDK_HOME" ]; then 
	for b in $(  wget -qO- http://developer.android.com/sdk/ndk/index.html | egrep -o "http://dl.google.com[^\"']*linux-x86.tar.bz2"
 ); do wget $b && tar --wildcards --no-anchored -xjvf android-ndk-*-linux-x86.tar.bz2; mv android-ndk-*/ $ANDROID_NDK_HOME; rm android-ndk-*-linux-x86.tar.bz2;
	done
else
    echo "Android NDK already installed to $ANDROID_NDK_HOME.  Skipping."
fi

#Determine if there is a 32 or 64-bit operating system installed and then install ia32-libs if necessary.
if $install_adb;then
    d=ia32-libs
    if [[ `getconf LONG_BIT` = "64" ]]; 
    then
        echo "64-bit operating system detected.  Checking to see if $d is installed."

        if [[ $(dpkg-query -f'${Status}' --show $d 2>/dev/null) = *\ installed ]]; then
            echo "$d already installed."
        else
            echo "Installing now..."
             sudo apt-get --force-yes -y install $d
        fi
    else
        echo "32-bit operating system detected.  Skipping."
    fi
fi

#Install the platform-tools
if [ ! -d "$ANDROID_SDK_HOME/platform-tools" ];then
    $ANDROID_SDK_HOME/tools/android update sdk --no-ui
fi

#Check if the ADB environment is set up.

if grep -q '$ANDROID_SDK_HOME/platform-tools' $HOME/.bashrc; 
then
    echo "ADB environment already set up"
else
    echo "export ANDROID_SDK_HOME=$ANDROID_SDK_HOME" >> $HOME/.bashrc
    echo "export ANDROID_NDK_HOME=$ANDROID_NDK_HOME" >> $HOME/.bashrc
    echo 'export PATH=$PATH:$ANDROID_SDK_HOME/platform-tools' >> $HOME/.bashrc
    echo 'export PATH=$PATH:$ANDROID_SDK_HOME/tools' >> $HOME/.bashrc
    echo '' >> $HOME/.bashrc
fi


