1. Simplify State Media's data to the format: primary_key^searchable_field
    > python simplifyData.py

2. Use the simplified data to generate a list of keywords 
    > python genWords.py data_simplified

3. Use the word list to generate different kinds of queries 
    > python genPrefixQueries.py words.txt

4. Run the test
    > srch2_license_dir=../../../../wrapper/conf index_dir=. ../../../../build/test/integration/Scalability_Test
