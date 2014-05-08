Author: Prateek

This tests insertion and querying nodes in a cluster.  To run the code:

shell> cd test/sharding/integration
shell> python ./2n-insertAB-queryAB/testInsertAndQuery.py ../../../build/src/server/srch2-search-server ./2n-insertAB-queryAB/2n-insertAB-queryAB.txt ./2n-insertAB-queryAB/listOfNodes.txt

Logic behind the test case:

- Insert records to nodes 1 and 2;
- Query both nodes and expect results inserted from both nodes.
