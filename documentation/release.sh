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

sdk_source="../source/SRCH2-Android-SDK"
javadoc="${sdk_source}/target/apidocs"

echo "Starting the release ..."
current_branch=`git rev-parse --symbolic-full-name --abbrev-ref HEAD`
[ $current_branch == "master" ] || { echo $'\nERROR: Should only run in the master branch. Stop the release'; exit -1;}

git pull origin master
[ $? -ne 0 ] && { echo $'\nERROR: Pull origin master failed. Stop the release'; exit -1;}

# reminder to change the pom.xml
cd $sdk_source
version=`mvn org.apache.maven.plugins:maven-help-plugin:2.1.1:evaluate -Dexpression=project.version | grep -v '\['`
cd $OLDPWD

version=${version%'-SNAPSHOT'}

echo -e "Detected releaes version is ${red} $version ${NC}, is this correct? [y|N]"
read yes
[ "$yes" == "y" ] || { echo "Stopped."; exit -1; }

release_file="./docs/release_notes/release_${version}.txt"
[ -f "$release_file" ] || { echo "ERROR:$release_file doesn't exist! Stopped."; exit -1; }

\rm -rf site
mkdocs build
[ $? -ne 0 ] && { echo $'\nERROR: Mkdocs build to "site" folder failed, Stop the release'; exit -1;}

cd $sdk_source
    
    # check if the LogCat is disabled 
    grep "private static boolean isLogging = false;" src/main/java/com/srch2/android/sdk/Cat.java
    [ $? -ne 0 ] && { echo $'\nERROR: LogCat is open, please set the LogCat.isLogging = false.'; exit -1;}

    mvn clean deploy
    [ $? -ne 0 ] && { echo $'\nERROR: Error happens while deploying, Stop the release'; exit -1;}

    mvn release:prepare release:perform release:clean 
    [ $? -ne 0 ] && { echo $'\nERROR: Maven release failed, Stop the release'; exit -1;}

    # deploy again for the release version
    newVersion=`mvn org.apache.maven.plugins:maven-help-plugin:2.1.1:evaluate -Dexpression=project.version | grep -v '\['`
    
    mvn versions:set -DnewVersion=$version
    [ $? -ne 0 ] && { echo $'\nERROR: Error happens while set release version, Stop the release'; exit -1;}

    mvn deploy
    [ $? -ne 0 ] && { echo $'\nERROR: Error happens while deploy release version, Stop the release'; exit -1;}

    #set it back
    mvn versions:set -DnewVersion=$newVersion
    [ $? -ne 0 ] && { echo $'\nERROR: Error happens while set new master version, Stop the release'; exit -1;}

    git add pom.xml && git commit -m "move to new version : $newVersion" && git push origin master

    # This javadoc:jar command will produce the cleanner java doc site.
    mvn javadoc:jar
    [ $? -ne 0 ] && { echo $'\nERROR: Error happens while creating javadoc, Stop the release'; exit -1;}

cd $OLDPWD

cp -r $javadoc ./site/javadoc

echo $'\nLocal release is done, wait for sync to the webmaster'

rsync -rav ./site/ rvijax@web240.webfaction.com:~/webapps/srch2/android/releases/$version/

echo "Done. Version is $version"
