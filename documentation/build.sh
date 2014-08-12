#!/bin/bash - 
#===============================================================================
#
#          FILE: build.sh
# 
#         USAGE: ./build.sh 
# 
#   DESCRIPTION: 
# 
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#       CREATED: 08/11/2014 06:18:54 PM PDT
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error

# create site folder 
mkdocs build
[ $? -ne 0 ] && { echo "\n Mkdocs build to 'site' folder failed, Stop the release"; exit -1;}

sdk_source="../source/SRCH2-Android-SDK/"
target="$sdk_source/target"
cd $sdk_source
mvn clean deploy
[ $? -ne 0 ] && { echo "\n Error happens while deploying, Stop the release"; exit -1;}

# This javadoc:jar command will produce the cleanner java doc site.
mvn javadoc:jar
[ $? -ne 0 ] && { echo "\n Error happens while creating javadoc, Stop the release"; exit -1;}
cd $OLDPWD

cp ${target}/deploy/com/srch2/SRCH2-Android-SDK/*/*.{jar,aar} ./site/download/latest-releases/
cp -r ${target}/apidocs ./site/javadoc

echo "local release is done, wait for sync to the webmaster"


