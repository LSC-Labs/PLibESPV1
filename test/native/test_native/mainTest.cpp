// google.github.io/googletest/primer.html
#include <gtest/gtest.h>
#include <includeModules.h>


int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc,argv);
    int nResult = RUN_ALL_TESTS();

    return(nResult);
}