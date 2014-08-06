#!/bin/bash - 
#===============================================================================
#
#          FILE: pakcageme.sh
# 
#         USAGE: ./pakcageme.sh 
# 
#   DESCRIPTION: This is the script to build the site package
# 
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: Jianfeng Jia (), jianfeng.jia@gmail.com
#       CREATED: 08/06/2014 10:12:42 AM PDT
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error

\rm -rf site
mkdocs build
cp -r docs/example-demo/* site/example-demo/
echo "successufly build the online docs into 'site' folder"

