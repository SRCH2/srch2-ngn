#!/bin/bash - 
#===============================================================================
#
#          FILE: update_documentation.sh
# 
#         USAGE: ./update_documentation.sh 
# 
#   DESCRIPTION: 
# 
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#       CREATED: 08/14/2014 12:08:00 PM PDT
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error

\rm -rf site 
mkdocs build && \
cd ../source/SRCH2-Android-SDK/ && \
version=`mvn org.apache.maven.plugins:maven-help-plugin:2.1.1:evaluate -Dexpression=project.version | grep -v '\['`
version=${version%'-SNAPSHOT'}
mvn javadoc:jar && \
cd $OLDPWD && \
cp -r ../source/SRCH2-Android-SDK/target/apidocs ./site/javadoc && \
rsync -ravI ./site/ rvijax@web240.webfaction.com:~/webapps/srch2/android/releases/$version/
[ $? -eq 0 ] || { echo "Error"; exit -1; }
echo "Done"
