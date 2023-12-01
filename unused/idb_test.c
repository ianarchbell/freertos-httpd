#include "idb_test.h"
#include "idb_router.h"

void run_tests(){
    testRoute();
}

/**
 * 
 * Test of a route without using HTTTP
 * 
*/
void testRoute(){
       
    char buffer[] = "/readlog/2023-10-17";
    NameFunction* fun_ptr = isRoute(buffer, HTTP_GET);
    if (fun_ptr){
        char buf[1024]; 
        route(fun_ptr, buf, sizeof buf, buffer);
    }
}