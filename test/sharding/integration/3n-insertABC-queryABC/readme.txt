Author: Prateek

This tests insertion and querying nodes in a cluster.  To run the code:

shell> cd test/sharding/integration
shell> python ./3n-insertABC-queryABC/testInsertAndQuery.py ../../../build/src/server/srch2-search-server ./3n-insertABC-queryABC/3n-insertABC-queryABC.txt ./3n-insertABC-queryABC/listOfNodes.txt

Logic behind the test case:
- 3 different records are inserted into three nodes:
- All of the records shoulds be available from each node:
