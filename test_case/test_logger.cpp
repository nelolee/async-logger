#include <stdio.h>
#include <string.h>
#include "async_logger_inf.h"


int main(int argc, char const *argv[])
{
    c_async_logger_inf* lp_logger_impl = NULL;
    do
    {
        int i_ret = init_async_logger_instance((void**)&lp_logger_impl,0);
        const int i_roll_size = 10*1024*1024;//10MB
        #ifdef WIN32
        lp_logger_impl->start_logging("./temp_logger.txt",i_roll_size);
        #else
        lp_logger_impl->start_logging("/tmp/temp_logger.txt",i_roll_size);
        #endif
        const char* lpc_temp_content = "this is a logger test content\r\n";
        lp_logger_impl->append_log(lpc_temp_content,strlen(lpc_temp_content));
        lp_logger_impl->stop_logging();
    }while(false);
    
    return 0;
}
