
# This package was written by Surendra, which can be used to analyze the dependencies of
# source files and generate a PDF file.  The result can be used to analyze the code base
# to do optimization.
# 
# Comments written by Chen Li

# install the graphViz pacakge
sudo apt-get install graphViz

# run the python program to analyze the source file dependencies and generate a dot file. Example:
python ./genDot.py /home/chenli/svnclient/mario/trunk/bimaple-engine/src ./result.dot

# convert the .dot file into a PDF file
dot -Tpdf result.dot -oresult.pdf
