
Author: Prateek

To run each integration test case for sharding, go to the individual folder and follow the
instructions in its own readme.txt file.

shell> python ./2n-insertA-queryB/testInsertAndQuery.py ../../../build/src/server/srch2-search-server ./2n-insertA-queryB/2n-insertA-queryB.txt ./2n-insertA-queryB/listOfNodes.txt


shell> python ./2n-insertAB-queryAB/testInsertAndQuery.py ../../../build/src/server/srch2-search-server ./2n-insertAB-queryAB/2n-insertAB-queryAB.txt ./2n-insertAB-queryAB/listOfNodes.txt

shell> python ./3n-insertABC-queryC/testInsertAndQuery.py ../../../build/src/server/srch2-search-server ./3n-insertABC-queryC/3n-insertABC-queryC.txt ./3n-insertABC-queryC/listOfNodes.txt

shell> python ./3n-insertABC-queryABC/testInsertAndQuery.py ../../../build/src/server/srch2-search-server ./3n-insertABC-queryABC/3n-insertABC-queryABC.txt ./3n-insertABC-queryABC/listOfNodes.txt

Command format:

python <python code> <srch2-engine binary> <path to the file that contains records for insertion/querying> <path to the file containing list of nodes>

The format of nodesList is:

<nodeId>||<port number>

The format of insertFile/nodesQueriesAndResults file is:

<nodeId>||<operation>||<input value>||<expected value>

