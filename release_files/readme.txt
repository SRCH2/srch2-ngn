Run install.sh script with licence key path as an input parameter.

Note: We advise you to run the install script to setup the config file properly.

directory layout:

/bin 
contains the executable file

/docs

The files in 'docs' directory are related to http://www.srch2.com/document/, which is
the public documentation on how to install and run a SRCH2 engine with
a Web UI.  It also includes a demo at example.srch2.com.

docs/index.html --> is the html file of the documentation.

docs/example-demo/index.html --> user interface of the example demo.
docs/example-demo/index.js   --> javascript file used by index.html in the example demo
docs/example-demo/jquery-1.7.1.min.js --> jQuery library

docs/images/*.png  --> images used in the documentation.
	
docs/highlight/: It contains Javascript files to make the codes inside the
   documentation page colorful. It does the coloring automatically and
   supports many different programming languages. You can modify it if
   needed.

/srch2_data
The folder contains sample files for stemmer, stop words, and synonym. You can continue to
use these files or replace them with your own files. Please make sure that you update the 
config file accordingly

/conf
Contains config file
