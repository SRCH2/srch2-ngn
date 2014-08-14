This documentation uses mkdocs to generate website for documentation.

To edit the webpages, edit the *.md files in docs.

Currently the generated documentation does not link to the API reference docs.

We use the release.sh to do the release. 

It will 
1. Build the .aar file
2. Generate the javadoc
3. Deploy the .aar file to our maven repository srch2.com/repo/maven
4. Build the documentation use `mkdocs build`
5. Copy the javadoc to the `site`
6. Deploy the site to the srch2.com/android/release/$version
7. Tag the git code base
8. Move the code base to the new version
