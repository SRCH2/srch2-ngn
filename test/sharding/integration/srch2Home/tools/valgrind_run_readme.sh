#valgrind --leak-check=full --suppressions=./suppressions_srch2.supp --error-limit=no --track-origins=yes ../../../../../build/src/server/srch2-search-server --config=$1
valgrind --tool=memcheck --leak-check=full --num-callers=5 --show-reachable=yes --track-fds=yes --suppressions=./suppressions_srch2.supp --error-limit=no ../../../../../build/src/server/srch2-search-server --config=$1
#valgrind --tool=massif --error-limit=no ../../../../../build/src/server/srch2-search-server --config=$1
