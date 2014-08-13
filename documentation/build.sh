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

red='\e[0;31m'
NC='\e[0m' # No Color

sdk_source="../source/SRCH2-Android-SDK/"
target="$sdk_source/target"
deploy="${target}/deploy"

echo "Starting the release ..."

# reminder to change the pom.xml
cd $sdk_source
version=`mvn org.apache.maven.plugins:maven-help-plugin:2.1.1:evaluate -Dexpression=project.version | grep -v '\['`
cd $OLDPWD

[[ $version == *SNAPSHOT ]] && { echo "ERROR:We should never release the SNAPSHOT version, please change the pom.xml" ; exit -1;}

echo "Don't forget to change the project.version in the pom.xml!"
echo -e "Detected releaes version is ${red} $version ${NC}, is this correct? [y|N]"
read yes
[ "$yes" == "y" ] || { echo "Stopped."; exit -1; }

release_file="./release_notes/release_$version"
[ -f "$release_file" ] || { echo "ERROR:$release_file doesn't exist! Stopped."; exit -1; }

\rm -rf site
mkdocs build
[ $? -ne 0 ] && { echo $'\nERROR: Mkdocs build to "site" folder failed, Stop the release'; exit -1;}

cd $sdk_source
mvn clean deploy
[ $? -ne 0 ] && { echo $'\nERROR: Error happens while deploying, Stop the release'; exit -1;}

# This javadoc:jar command will produce the cleanner java doc site.
mvn javadoc:jar
[ $? -ne 0 ] && { echo $'\nERROR: Error happens while creating javadoc, Stop the release'; exit -1;}
cd $OLDPWD

#cp ${deploy}/com/srch2/*/*/*.{jar,aar} ./site/download/releases/
#cp ${release_file} ./site/download/releases/
cp -r ${target}/apidocs ./site/javadoc

echo $'\nLocal release is done, wait for sync to the webmaster'

rsync -rav --ignore-existing ${deploy}/* rvijax@web240.webfaction.com:~/webapps/srch2/repo/maven/
rsync -rav ./site/ rvijax@web240.webfaction.com:~/webapps/srch2/android/releases/$version/

echo "Done. Version is $version"
