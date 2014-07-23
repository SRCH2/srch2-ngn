
Solution:

1. Moved the database adapter out of the server folder. 
2. Created a adapter folder to handle database connections.
3. The adapter provide the function to database connector as Input/Delete/Update/SaveChanges/ConfigLookUP.
4. The connector should provide implementation Init/RunListener/CreateNewIndexes to the engine.
5. The connector should be compiled as a shard library with extern "C" DataConnector* create() {} which is to create a new instance and extern "C" void destroy(DataConnector* p) {} which is to delete the instance.
6. Add system test. Add pymongo thirdparty.

How to make the test works.

1. Install thirdparty libraries:

   shell> cd srch2-ngn/thirdparty/
   shell> ./thirdparty-build.sh

2. Compile the engine:

    shell> cd srch2-ngn
    shell> mkdir build
    shell> cd build
    shell> cmake -DBUILD_RELEASE=ON ..
    shell> make -j 4

3. Compile the mongodb connector:

    shell> cd srch2-ngn/db_connectors
    shell> mkdir build
    shell> cd build
    shell> cmake ..
    shell> make

CHEN STOPPED HERE.

4. Install mongodb by following instructions on http://docs.mongodb.org/manual/installation/

5. Start mongodb, assuming we have a folder "~/mongodb_data/db" for storing
   mongo data:

   shell> mongod --dbpath ~/mongodb_data/db --replSet "rs0"

  This command will start the mongodb engine with the replication mode
  enabled. Instruction are available at:
  http://docs.mongodb.org/manual/tutorial/convert-standalone-to-replica-set/ 

5. Run the system test case

    shell> cd srch2-ngn/test/wrapper/system_tests
    shell> python ./adapter_mongo/MongoTest.py ../../../build/src/server/srch2-search-server ./adapter_mongo/queries.txt


