
#include "IntegrationTestHelper.h"

using std::string;

int main(int argc, char **argv)
{
    string index_dir = getenv("index_dir");

    buildIndex(index_dir);

    return 0;
}
