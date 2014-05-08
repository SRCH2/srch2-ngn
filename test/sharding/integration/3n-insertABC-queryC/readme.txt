Author: Prateek

This tests insertion and querying nodes in a cluster.  To run the code:

shell> cd test/sharding/integration
shell> python ./3n-insertABC-queryC/testInsertAndQuery.py ../../../build/src/server/srch2-search-server ./3n-insertABC-queryC/3n-insertABC-queryC.txt ./3n-insertABC-queryC/listOfNodes.txt

Logic behind the test case:
- Same record is inserted into three different nodes. On inserting in 2nd and 3rd node, duplicate primary key error is expected:
- On querying 3rd node, record inserted in 1st node is expected:

