#include "util/cowvector/cowvector.h"
#include "util/Assert.h"
using namespace std;

namespace srch2is = srch2::instantsearch;
using namespace srch2is;

void* writer(void* arg);
void* reader(void* arg);

void* writer2(void* arg);
void* reader2(void* arg);

void* writer3(void* arg);
void* reader3(void* arg);


cowvector<int> cowv;

// Main logic of this test case: it first uses push_back()
// to add 10 elements (0, 1, ..., 90 to the writeview and commits.
// Then it checks the vector view type, which should be a writeview.
// And it tests the needToFreeOldArray flag, it should be false.
// It will again call push_back() to add 10 elements (0, 1, ..., 9)
// and test the needToFreeOldArray flag. The flag should be true,
// since we have reallocated the memory of the array due to the more pushback calls.
void test1()
{
    vectorview<int>* &write = cowv.getWriteView();
    for (int i= 0; i< 10; i++) {
        write->push_back(i);
    }
    cowv.commit();
    ASSERT(write->isWriteView() == true);
    ASSERT(write->isReadView() == false);
    ASSERT(write->getNeedToFreeOldArray() == false);
    shared_ptr<vectorview<int> > read;
    cowv.getReadView(read);
    ASSERT(read->isWriteView() == false);
    ASSERT(read->isReadView() == true);
    for (int i= 0; i< 10; i++) {
        write->push_back(i);
    }
    ASSERT(write->getNeedToFreeOldArray() == true);
}

// single reader and single writer
// main logic: it will first push_back 10 elements [0,9] and merge
// then it will start two threads. One for reader, one for writer.
// the writer will first forceCreateCopy and do a modification by changing the element 3 to 100, at the same time the reader will read it.
// the reader will not see the change until the writer merges the change.
// the writer will then push_back 90 elements. After it does the merge, the reader will detect it(it will find there are 100 elements in the vecterview).
// the writer will then push_back 1000 elements. After it does the merge, the reader will detect it(it will find 1100 elements in the vecterview).
void test2() {
    pthread_t  t1,t2 ;
    vectorview<int>* &write = cowv.getWriteView();

    write->clear();
    cowv.merge();
    write = cowv.getWriteView();

    for (int i= 0; i< 10; i++) {
        write->push_back(i);
    }
    cowv.merge();

    int create1 = pthread_create( &t1, NULL, writer, NULL);
    if (create1 != 0) cout << "error" << endl;
    int create2 = pthread_create( &t2, NULL, reader, NULL);
    if (create2 != 0) cout << "error" << endl;
    pthread_join(t1,NULL) ;
    pthread_join(t2,NULL) ;
}

// write view test at operation
void* writer(void* arg) {
    vectorview<int>* &write = cowv.getWriteView();
    write->forceCreateCopy();
    sleep(1);
    // do change
    write->at(3) = 100;
    sleep(2);
    // merge the change
    cowv.merge();
    sleep(2);
    // push_back 90 elements with wirteview and merge
    write = cowv.getWriteView();
    for(int i = 0; i < 90; i++)
        write->push_back(i);
    cowv.merge();
    sleep(2);
    // push_back 1000 elements with wirteview and merge
    for(int i = 0; i < 1000; i++)
        write->push_back(i);
    cowv.merge();
    return NULL;
}

//read view test at operation
void* reader(void* arg) {
    shared_ptr<vectorview<int> > read;
    cowv.getReadView(read);
    //check the reader with no change
    for (int i = 0; i< read->size(); i++) {
        ASSERT(i == read->at(i));
    }
    sleep(2);
    //do not change before merge
    for (int i = 0; i< read->size(); i++) {
        ASSERT(i == read->at(i));
    }
    sleep(2);
    // see the change after merge
    cowv.getReadView(read);
    cout << read->at(3) << endl;
    ASSERT(read->at(3) == 100);
    sleep(2);
    // see the change of new insert item
    cowv.getReadView(read);
    cout << read->size() << endl;
    ASSERT(read->size() == 100);
    sleep(2);
    // see the change of further insert item
    cowv.getReadView(read);
    cout << read->size() << endl;
    ASSERT(read->size() == 1100);
    return NULL;
}


// multi reader and single writer
// main logic: we will first push_back 10 elements [0,9] and merge it. After that we will start 11 threads.
// 10 of the threads are readers, and one is a writer.
// the readers will not see the change made by the writer (element 3 -> 100) before the writer merges it.
void test3() {
    pthread_t  t1,t2[10];
    vectorview<int>* &write = cowv.getWriteView();

    write->clear();
    cowv.merge();

    write = cowv.getWriteView();

    for (int i= 0; i< 10; i++) {
        write->push_back(i);
    }
    cowv.merge();
    cout << write->size()<<endl;


    int create1 = pthread_create( &t1, NULL, writer2, NULL);

    if (create1 != 0) cout << "error" << endl;

    for (int i = 0; i< 10; i++) {
        int create2 = pthread_create( &t2[i], NULL, reader2, NULL);
        if (create2 != 0) cout << "error" << endl;
    }

    pthread_join(t1,NULL) ;
    for (int i = 0; i< 10; i++)
        pthread_join(t2[i],NULL) ;
}

//read view test push_back operation
void* reader2(void* arg) {
    shared_ptr<vectorview<int> > read;
    cowv.getReadView(read);
    sleep(1);
    //do not change before merge
    //cout << read->size() << endl;
    for (int i = 0; i< read->size(); i++) {
        ASSERT(i == read->at(i));
    }
    return NULL;
}

// write view test push_back operation
void* writer2(void* arg) {
    vectorview<int>* &write = cowv.getWriteView();
    write->push_back(100);
    sleep(1);
    cowv.merge();
    return NULL;
}

// multi reader and single writer for larger data
// The main logic is the same as test 2. We have 1 writer and 10 readers for a larger data size (1000 times larger).
void test4() {
    pthread_t  t1,t2[10];
    vectorview<int>* &write = cowv.getWriteView();

    write->clear();
    cowv.merge();

    write = cowv.getWriteView();

    for (int i= 0; i< 10000; i++) {
        write->push_back(i);
    }
    cowv.merge();
    cout << write->size()<<endl;


    int create1 = pthread_create( &t1, NULL, writer3, NULL);

    if (create1 != 0) cout << "error" << endl;

    for (int i = 0; i< 10; i++) {
        int create2 = pthread_create( &t2[i], NULL, reader3, NULL);
        if (create2 != 0) cout << "error" << endl;
    }

    pthread_join(t1,NULL) ;
    for (int i = 0; i< 10; i++)
        pthread_join(t2[i],NULL) ;
}

// write view test at operation
void* writer3(void* arg) {
    vectorview<int>* &write = cowv.getWriteView();
    write->forceCreateCopy();
    sleep(1);
    // do change
    write->at(3) = 100;
    write->at(200) = 100;
    write->at(1000) = 100;
    write->at(9999) = 100;
    sleep(2);
    // merge the change
    cowv.merge();
    sleep(2);
    // push_back 90 elements with wirteview and merge
    write = cowv.getWriteView();
    for(int i = 0; i < 90000; i++)
        write->push_back(i);
    cowv.merge();
    sleep(2);
    // push_back 1000 elements with wirteview and merge
    for(int i = 0; i < 1000000; i++)
        write->push_back(i);
    cowv.merge();
    return NULL;
}

//read view test at operation
void* reader3(void* arg) {
    shared_ptr<vectorview<int> > read;
    cowv.getReadView(read);
    //check the reader with no change
    for (int i = 0; i< read->size(); i++) {
        ASSERT(i == read->at(i));
    }
    sleep(2);
    //do not change before merge
    for (int i = 0; i< read->size(); i++) {
        ASSERT(i == read->at(i));
    }
    sleep(2);
    // see the change after merge
    cowv.getReadView(read);
    cout << read->at(3) << endl;
    ASSERT(read->at(3) == 100);
    ASSERT(read->at(200) == 100);
    ASSERT(read->at(1000) == 100);
    ASSERT(read->at(9999) == 100);
    sleep(2);
    // see the change of new insert item
    cowv.getReadView(read);
    cout << read->size() << endl;
    ASSERT(read->size() == 100000);
    sleep(2);
    // see the change of further insert item
    cowv.getReadView(read);
    cout << read->size() << endl;
    ASSERT(read->size() == 1100000);
    return NULL;
}



int main( void ) {
    test1();
    test2();
    test3();
    test4();
    return 0;
}



