These files here are related to the example.srch2.com, which is the public document and demo for SRCH2


Here is the explanation of files structure:


index.html --> is the user interface of the example demo.
index.js   --> is the javascript file used by index.html
jquery-1.7.1.min.js --> is the jQuery library


Inside the 'document' Directory:

	index.html   --> is the html file of the documentation.
	
	md.mkd       --> is the 'markdown' source of the index.html. The index.html is written in 'markdown' and then the output is converted to a html format. To work with 'markdown', you can install the application named 'ReText', which gets the 'markdown' as an input and makes an  output in html. ReText has GUI and it is really user friendly. 
To Install ReText:   $ sudo apt-get install retext
				*	To work with it, you should write your text in markdown format. Then go to the "edit->View HTML code". Then a window will pop up having the HTML code of what you wrote. Then you can copy it in a .html file and save it.


	*.png  --> are the images used in the documentation. 
	
	highlight directory: It contains some JavaScript files to make the codes inside the index.html colorful. It does the coloring automatically and supports so many different languages. You can modify it if you want something more than what it does. (To use this you just need to include it in your JavaScript files)


