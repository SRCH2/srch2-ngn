# Authors: Jianfeng Jia and Chen Li
# August 2014
# 
# This folder includes files for SRCH2 engine documentation using the Markdown language
# and the Mdcos tool (http://www.mkdocs.org/)

# Install the Mkdocs

Please run the ./setup-mkdocs.sh to install the Mkdocs.

# How to use
This folder is created by the command `mkdocs new docs`

# To see the docs
We can run 
```bash
mkdocs serve
```
to start the local server to show the documentation. To check the documentation, visit
```bash
http://127.0.0.1:8000/
```

# To write a new markdown file:

After creating a new .mkd file, you can add the corresponding page to the ./mkdocs.yml file. 
Then it will be listed on the Web site as the corresponding order.

# To generate the documentation files 
Run
```
package_me.sh
```

which will run ```mkdocs build``` to build the all static site pages under the `site` folder.
Since `mkdocs` does not copy those HTML pages example-demo/basic/index.html and
example-demo/advanced/index.html, we have to use the script to manually copy those files.

# To change the theme
We are using the customized theme `srch2-theme`. We can change the corresponding *.html to add the contents.
We can also edit the `srch-theme/css/srch2.css` to customize the appearance of the existing elements.
