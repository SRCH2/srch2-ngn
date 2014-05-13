
Author: Prateek

This tests insertion and querying nodes in a cluster.  To run the code:

shell> cd test/sharding/integration 
shell> python ./AllTestCases/test-sharding.py ../../../build/src/server/srch2-search-server ./AllTestCases/2n-insertA-deleteB.txt ./AllTestCases/list-of-2-nodes.txt
shell> python ./AllTestCases/test-sharding.py ../../../build/src/server/srch2-search-server ./AllTestCases/2n-insertA-R1-B-R2-deleteA-R1-B-R2.txt ./AllTestCases/list-of-2-nodes.txt
shell> python ./AllTestCases/test-sharding.py ../../../build/src/server/srch2-search-server ./AllTestCases/2n-insertA-R1-B-R2-deleteB-R1-A-R1.txt ./AllTestCases/list-of-2-nodes.txt
shell> python ./AllTestCases/test-sharding.py ../../../build/src/server/srch2-search-server ./AllTestCases/2n-insertA-R1-B-R2-deleteB-R1-queryA-R1.txt ./AllTestCases/list-of-2-nodes.txt
shell> python ./AllTestCases/test-sharding.py ../../../build/src/server/srch2-search-server ./AllTestCases/2n-insertAB-queryAB.txt ./AllTestCases/list-of-2-nodes.txt
shell> python ./AllTestCases/test-sharding.py ../../../build/src/server/srch2-search-server ./AllTestCases/2n-insertAABB.txt ./AllTestCases/list-of-2-nodes.txt
shell> python ./AllTestCases/test-sharding.py ../../../build/src/server/srch2-search-server ./AllTestCases/3n-insertABC-queryABC ./AllTestCases/list-of-3-nodes.txt
shell> python ./AllTestCases/test-sharding.py ../../../build/src/server/srch2-search-server ./AllTestCases/3n-insertABC-queryC ./AllTestCases/list-of-3-nodes.txt
shell> python ./AllTestCases/test-sharding.py ../../../build/src/server/srch2-search-server ./AllTestCases/3n-insertA-queryABC ./AllTestCases/list-of-3-nodes.txt


