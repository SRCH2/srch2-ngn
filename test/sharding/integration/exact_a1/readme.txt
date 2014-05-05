Readme:

This tests insertion and querying
To run the code go to the parent directory and execute following command
python ./exact_a1/testInsertAndQuery.py ../../../build/src/server/srch2-search-server ./exact_a1/insertFile.txt ./exact_a1/nodesList.txt
python <python code> <srch2-engine binary> <path to the file that contains records for insertion/querying> <path to the file containing list of nodes>

The format of nodesList is
<nodeId>||<port number>

The format of insertFile/nodesQueriesAndResults file is
<nodeId>||<operation>||<input value>||<expected value>

