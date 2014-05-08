Author: Prateek

This tests insertion and querying nodes in a cluster.  To run the code:

shell> cd test/sharding/integration
shell> python ./2n-insertAABB/testInsertAndQuery.py ../../../build/src/server/srch2-search-server ./2n-insertAABB/2n-insertAABB.txt ./2n-insertAABB/listOfNodes.txt

Logic behind the test case:
- Two records are inserted into two nodes:
- On inserting the records that is already inserted in one node, the other node should return, duplicate primary key error:

