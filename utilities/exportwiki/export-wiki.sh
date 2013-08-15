#!/bin/bash

# > sudo apt-get install libxalan2-java tidy

# Main variables.  Replace these defaults with ones suitable for your
# environment.

# pagesList=${pagesList:-"GettingStartedDoc"}
pagesList=${pagesList:-"ReleaseOne"}

server=${server:-"https://bimaple.vijay:r0um2ni2@trac.assembla.com/bimaple-root"}

xsl=${xsl:-"export-wiki.xsl"}

docsDir=${docsDir:-"./wiki"}
mkdir -p $docsDir/media

tmp="/tmp/export-wiki"
rm -rf $tmp
mkdir $tmp

#####################################################################

# generated temp variables
tmpIndex="/tmp/tmp.xml"
pages=($pagesList)

# login
read -p "Wiki username: " uname
stty -echo
read -p "Wiki password: " passwd
stty echo
wget --no-check-certificate --save-cookies $tmp/wget_cookie --keep-session-cookies  --output-document $tmp/login https://$uname:$passwd@trac.assembla.com/bimaple-root

# process each page
echo "<pages>" > $tmpIndex 
cnt=${#pages[@]}
for (( i = 0 ; i < cnt ; i++ ))
do
    wget --no-check-certificate --load-cookie $tmp/wget_cookie --output-document $tmp/${pages[$i]}.html ${server}/wiki/${pages[$i]}
    tidy -modify -utf8 $tmp/${pages[$i]}.html
    echo java -classpath /usr/share/java/xalan2.jar org.apache.xalan.xslt.Process -IN $tmp/${pages[$i]}.html -XSL $xsl -OUT $docsDir/${pages[$i]}.html
    #java -classpath /usr/share/java/xalan2.jar org.apache.xalan.xslt.Process -IN $tmp/${pages[$i]}.html -XSL $xsl -OUT $docsDir/${pages[$i]}.html
    xsltproc -o $docsDir/${pages[$i]}.html $xsl $tmp/${pages[$i]}.html
    echo "<page>${pages[$i]}</page>" >> $tmpIndex
done
echo "</pages>" >> $tmpIndex

# get the css and create the index page
wget --no-check-certificate --output-document $tmp/trac.css ${server}/chrome/common/css/trac.css  
sed 's/url(..\//url(/' $tmp/trac.css > $docsDir/media/trac.css

wget --no-check-certificate --output-document $docsDir/media/extlink.gif ${server}/chrome/common/extlink.gif

# java -cp /usr/share/java/xalan2.jar org.apache.xalan.xslt.Process -IN $tmpIndex -XSL $xsl -OUT $docsDir/index.html 

python export-images.py > $tmp/images
OLDIFS=$IFS
IFS='
'
for image in `cat $tmp/images`
do
    image_url=`echo $image | awk '{print $1}'`
    image_fname=`echo $image | awk '{print $2}'`
    wget --no-check-certificate --load-cookie $tmp/wget_cookie --output-document $docsDir/media/${image_fname} ${server}${image_url}
done
IFS=$OLDIFS

rm -rf $tmp

echo 'If you got "export-wiki.xsl (No such file or directory)" then run the script from its own directory.'
