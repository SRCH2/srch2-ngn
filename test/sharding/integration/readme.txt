
Author: Prateek

To run each the integration test case 2n-insertA-deleteA, run the following command

shell> python ./2n-insertA-deleteA/testDelete.py ../../../build/src/server/srch2-search-server ./2n-insertA-deleteA/2n-insertA-deleteA.txt ./2n-insertA-deleteA/listOfNodes.txt

To run all the test cases present in director AllTestCases run the command
shell> runme.sh

Command format:

python <python code> <srch2-engine binary> <path to the file that contains records for insertion/querying> <path to the file containing list of nodes>

The format of nodesList is:

<nodeId>||<port number>

The format of insertFile/nodesQueriesAndResults file is:

<nodeId>||<operation>||<input value>||<expected value>

