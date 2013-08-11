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

// test type of vectorview
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
void test2() {
    pthread_t  t1,t2 ;
    vectorview<int>* &write = cowv.getWriteView();

    write->clear();
    cowv.commit();
    write = cowv.getWriteView();

    for (int i= 0; i< 10; i++) {
        write->push_back(i);
    }
    cowv.commit();

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
    // commit the change
    cowv.commit();
    sleep(2);
    // push_back 90 elements with wirteview and commit
    write = cowv.getWriteView();
    for(int i = 0; i < 90; i++)
        write->push_back(i);
    cowv.commit();
    sleep(2);
    // push_back 1000 elements with wirteview and commit
    for(int i = 0; i < 1000; i++)
        write->push_back(i);
    cowv.commit();
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
    //do not change before commit
    for (int i = 0; i< read->size(); i++) {
        ASSERT(i == read->at(i));
    }
    sleep(2);
    // see the change after commit
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
void test3() {
    pthread_t  t1,t2[10];
    vectorview<int>* &write = cowv.getWriteView();

    write->clear();
    cowv.commit();

    write = cowv.getWriteView();

    for (int i= 0; i< 10; i++) {
        write->push_back(i);
    }
    cowv.commit();
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
    //do not change before commit
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
    cowv.commit();
    return NULL;
}

// multi reader and single writer for larger data
void test4() {
    pthread_t  t1,t2[10];
    vectorview<int>* &write = cowv.getWriteView();

    write->clear();
    cowv.commit();

    write = cowv.getWriteView();

    for (int i= 0; i< 10000; i++) {
        write->push_back(i);
    }
    cowv.commit();
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
    // commit the change
    cowv.commit();
    sleep(2);
    // push_back 90 elements with wirteview and commit
    write = cowv.getWriteView();
    for(int i = 0; i < 90000; i++)
        write->push_back(i);
    cowv.commit();
    sleep(2);
    // push_back 1000 elements with wirteview and commit
    for(int i = 0; i < 1000000; i++)
        write->push_back(i);
    cowv.commit();
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
    //do not change before commit
    for (int i = 0; i< read->size(); i++) {
        ASSERT(i == read->at(i));
    }
    sleep(2);
    // see the change after commit
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



