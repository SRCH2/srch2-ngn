#!/bin/bash
usage ()
{
  echo "usage: $0 --license=path/to/licence/file [--install-dir=/path/to/install/dir/]"
  exit
}

if [ $# -eq 0 ]
  then
  usage
fi
ra1=false
while [[ $1 = -* ]]; do
    arg=$1; shift           

    case $arg in
        --license)
            echo $1
            ra1=true
            shift    
            ;;
        --install-dir)
            echo $1   
            ;;
    esac
done

if [ ! ra1]
  then
  echo 'Please provide path to license directory\n'
  usage
  exit
fi

if [ -z "$1" ]
  then
    echo "Path"
    exit
fi
