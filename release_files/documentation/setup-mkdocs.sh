#!/bin/bash - 
#===============================================================================
#
#          FILE: setup-mkdocs.sh
# 
#         USAGE: ./setup-mkdocs.sh 
# 
#   DESCRIPTION: Set up the Mkdocs http://www.mkdocs.org/ enviroment
#                Once set up, we can try
#
#                > mkdocs serve  # to review the docs and do some changes
#                > mkdocs build  # to generate the final static "/site" pages
# 
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: Jianfeng Jia (), jianfeng.jia@gmail.com
#       CREATED: 08/05/2014 10:11:05 AM PDT
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error

case $( uname -s ) in
    Linux) 
        sudo apt-get python-pip
        ;;
    Darwin)
        sudo easy_install pip
        ;;
esac

sudo pip install mkdocs

