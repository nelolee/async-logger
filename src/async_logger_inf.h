#pragma once
#if defined(__linux__)
#define ASYNC_LOGGER_API 
#elif defined(_WIN32)
#if defined(SHARED_LIB)
#ifdef EXPORT_LOGGER_API
#define ASYNC_LOGGER_API	_declspec(dllexport)
#else
#define ASYNC_LOGGER_API   _declspec(dllimport)
#endif
#else
#define ASYNC_LOGGER_API
#endif
#endif

typedef enum em_async_log_error_code
{
	EM_ASYNC_LOG_ERROR_SEEK_FILE_FAILED = -6,
	EM_ASYNC_LOG_ERROR_OPEN_FILE_FAILED,
	EM_ASYNC_LOG_ERROR_CACHE_BUFFER_CREATE,
	EM_ASYNC_LOG_ERROR_LOG_DATA_TOO_BIG,
	EM_ASYNC_LOG_ERROR_PARAMS,
	EM_ASYNC_LOG_ERROR_UNDEFINED,
	EM_ASYNC_LOG_ERROR_NONE,//must be  0
}EM_ASYNC_LOG_ERROR_CODE;

const int AUTO_FLUSH_INTERVAL_TIME_MS = 2000;//2s ,2000ms
const int LOG_BUFFER_MAX_CACHE_SIZE = 2 * 1024 * 1024;//2MB

class c_async_logger_inf
{
public:
	virtual int start_logging(const char* lpc_full_path_name, unsigned long long ll_roll_size,
		const int i_auto_flush_time_ms = AUTO_FLUSH_INTERVAL_TIME_MS,
		const int i_log_buffer_max_cache_size = LOG_BUFFER_MAX_CACHE_SIZE)=0;
	virtual int append_log(const char* lpc_content, int i_data_len)=0;
	virtual int stop_logging()=0;
};

#ifdef __cplusplus
extern "C" {
#endif
	ASYNC_LOGGER_API  int init_async_logger_instance(void** lpp_instance,int iReserved);
	ASYNC_LOGGER_API  int destroy_async_logger_instance(void** lpp_instance);
#ifdef __cplusplus
};
#endif
