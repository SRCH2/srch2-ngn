
Author: Prateek

This tests insertion and querying nodes in a cluster.  To run the code:

shell> cd test/sharding/integration 
shell> python ./2n-insertA-deleteA/test-sharding.py ../../../build/src/server/srch2-search-server ./2n-insertA-deleteA/2n-insertA-deleteA.txt ./2n-insertA-deleteA/listOfNodes.txt 

Logic behind the test case:
- A record is inserted into one node and queried from another node.

