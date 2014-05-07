Author: Prateek

This tests insertion and querying nodes in a cluster.  To run the code:

shell> cd test/sharding/integration
shell> python ./2n-insertAB-queryAB/testInsertAndQuery.py ../../../build/src/server/srch2-search-server ./2n-insertAB-queryAB/2n-insertAB-queryAB.txt ./2n-insertAB-queryAB/listOfNodes.txt

Command format:

python <python code> <srch2-engine binary> <path to the file that contains records for insertion/querying> <path to the file containing list of nodes>

The format of nodesList is:

<nodeId>||<port number>

The format of insertFile/nodesQueriesAndResults file is:

<nodeId>||<operation>||<input value>||<expected value>

