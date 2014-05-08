
Author: Prateek

This tests insertion and querying nodes in a cluster.  To run the code:

shell> cd test/sharding/integration 
shell> python ./2n-insertA-queryB/testInsertAndQuery.py ../../../build/src/server/srch2-search-server ./2n-insertA-queryB/2n-insertA-queryB.txt ./2n-insertA-queryB/listOfNodes.txt 

Logic behind the test case:
- A record is inserted into one node and queried from another node.

