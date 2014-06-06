//replace this code with actual code
#include <iostream>
int main () {
    std::cout << "hello world" << std::endl;
}
// illustrative code..
void runListener(void * engineCallBack) {
     // connect to db
     bool stop = false; 
     while(!stop) {
        // get inserts/updates/delete from db
        // ...
        // engineCallBack->insert(); 
        // engineCallBack->update();
        // engineCallBack->delete(); 
     }
}

