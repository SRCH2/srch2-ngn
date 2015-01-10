
This case tests the engine for the case where we need to reassign ids
during a deletion.  To do the test manually, do the following:

- In one terminal, run:
  ../../../../build/src/server/srch2-search-server --config-file=srch2-config.xml 

- In two other terminals, prepare the following commands:
   sh stackoverflow-insert-1000.sh
   sh stackoverflow-delete-1000.sh 

- Start the insert command, immediately followed the delete command.
  The engine should not crash.
