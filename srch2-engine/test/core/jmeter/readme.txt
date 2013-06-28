This folder contains files for testing the performance of text-only search and text-geo search using jmeter.

For each test:

1. Start jmeter and open a configureation file (*.jmx)
2. Modify the “HTTP Request Defaults” to set up the host name and port number.
3. Modify the “insert_record" and “delete_record" to set up the right paths.
4. Start the Srch2 search engine using the corresponding *_head_* file or a bigger data file containing all the records in the *_head_* file.
5. Start the jmeter test.
