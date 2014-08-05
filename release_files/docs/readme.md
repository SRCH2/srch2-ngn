# Install the Mkdocs
please run the ./setup-mkdocs.sh to install the [Mkdocs](http://www.mkdocs.org/)

# How use
This folder is created by the `mkdocs new docs`

# To test the docs
We can run 
```bash
mkdocs serve
```
to start the local server to have a check about the contents and the style

# To write the new markdown file
After finish writing the *.mkd, you can add the correspoding page into the ./mkdocs.yml file. 
Then it will be listed website as the specified order .

# To deploy
Run
```
mkdocs build
```

It will build the all static site pages under `site` folder.


