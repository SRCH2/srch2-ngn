Author: Prateek

This tests insertion and querying nodes in a cluster.  To run the code:

shell> cd test/sharding/integration
shell> python ./3n-insertA-queryABC/testInsertAndQuery.py ../../../build/src/server/srch2-search-server ./3n-insertA-queryABC/3n-insertA-queryABC.txt ./3n-insertA-queryABC/listOfNodes.txt

Logic:
- A record is inserted into one node and it should be available for search from all the three nodes.
