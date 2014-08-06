# Install the Mkdocs
please run the ./setup-mkdocs.sh to install the [Mkdocs](http://www.mkdocs.org/)

# How use
This folder is created by the command `mkdocs new docs`

# To test the docs
We can run 
```bash
mkdocs serve
```
to start the local server to have a check about the contents and the style. To check the documentation,
visit
```bash
http://127.0.0.1:8000/
```

# To write the new markdown file
After creating a new .mkd file, you can add the corresponding page to the ./mkdocs.yml file. 
Then it will be listed on the Web site as the corresponding order.

# To deploy
Run
```
mkdocs build
```

It will build the all static site pages under the `site` folder.

# To change the theme
We are using the customized theme `srch2-theme`. We can change the corresponding *.html to add the contents.
We can also edit the `srch-theme/css/srch2.css` to customize the appearance of the existing elements.
