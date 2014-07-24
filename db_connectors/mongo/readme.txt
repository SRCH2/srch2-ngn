
Compiling and running MongoDB connector
Author: Chen Liu and Chen Li

1. Install thirdparty libraries:

   shell> cd srch2-ngn/thirdparty/
   shell> ./thirdparty-build.sh

2. Compile the SRCH2 engine:

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

4. Install mongodb by following instructions on http://docs.mongodb.org/manual/installation/

5. Start mongodb, assuming we have a folder "~/mongodb_data/db" for storing
   mongo data:

   shell> mongod --dbpath ~/mongodb_data/db --replSet "rs0"

   Use another terminal to do the following to initialize mongodb (needed only once):

   This command will start the mongodb engine with the replication mode
   enabled. Instruction are available at:
   http://docs.mongodb.org/manual/tutorial/convert-standalone-to-replica-set/ 

   shell> mongo
   mongo> rs.initiate()

5. Run the system test case

    shell> cd srch2-ngn/test/wrapper/system_tests
    shell> python ./adapter_mongo/MongoTest.py ../../../build/src/server/srch2-search-server ./adapter_mongo/queries.txt

  If everything works, we will see a "Successful" message.
