
These files are related to http://www.srch2.com/document/, which is
the public documentation on how to install and run a SRCH2 engine with
a Web UI.  It also includes a demo at example.srch2.com.

index.html --> is the html file of the documentation.

example-demo/index.html --> user interface of the example demo.
example-demo/index.js   --> javascript file used by index.html in the example demo
example-demo/jquery-1.7.1.min.js --> jQuery library

*.png  --> images used in the documentation.
	
highlight/: It contains Javascript files to make the codes inside the
   documentation page colorful. It does the coloring automatically and
   supports many different programming languages. You can modify it if
   needed.

index.mkd --> is the 'markdown' source of the index.html. The
  index.html is written in 'markdown' and then the output is converted
  to an html format. 

  To work with 'markdown', you can install the application named
  'ReText', which gets the 'markdown' as an input and makes an output
  in html. ReText has GUI and it is really user friendly.

  To install ReText on Linux:
     $ sudo apt-get install retext

  To work with it, you should write your text in markdown format. Then
  go to the "edit->View HTML code". Then a window will pop up having
  the HTML code of what you wrote. Then you can copy it in a .html
  file and save it.



