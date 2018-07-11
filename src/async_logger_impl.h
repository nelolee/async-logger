#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>
#include <stdio.h>
#include <string.h>
#include "async_logger_inf.h"


class count_down_latch
{
public:
	explicit count_down_latch(int count);
	void wait();
	void count_down();
	int	get_count() const;

private:
	mutable std::mutex		m_mutex;
	std::condition_variable	m_condition;
	int								m_count;
};

typedef struct _st_log_buffer_unit
{
	std::unique_ptr<char> lp_cache_bytes;
	int								i_actual_data_len;
	int							    i_cache_bytes_max_len;
	int init(int i_caches_bytes_max)
	{
		lp_cache_bytes.reset(new char[i_caches_bytes_max]);
		if (lp_cache_bytes)
		{
			memset(lp_cache_bytes.get(), 0, i_caches_bytes_max);
			i_cache_bytes_max_len = i_caches_bytes_max;
			i_actual_data_len = 0;
			return 0;
		}
		return -1;
	}
	int available_len()
	{
		return i_cache_bytes_max_len - i_actual_data_len;
	}
	void append_data(const char* lpc_data, int i_data_len)
	{
		if (i_data_len < available_len())
		{
			memcpy(lp_cache_bytes.get()+i_actual_data_len,lpc_data,i_data_len);
			i_actual_data_len += i_data_len;
		}
	}
	const char* get_data()
	{
		return lp_cache_bytes.get();
	}
	int get_actual_data_len()
	{
		return i_actual_data_len;
	}
	void reset()
	{
		memset(lp_cache_bytes.get(), 0, i_cache_bytes_max_len);
		i_actual_data_len = 0;
	}
}st_log_buffer_unit,*lp_st_log_buffer_unit;

typedef std::list<std::unique_ptr<st_log_buffer_unit>> buffer_container;
typedef buffer_container::iterator									buffer_container_it;
typedef buffer_container::const_iterator						buffer_container_cit;
class c_async_logger_impl:public c_async_logger_inf
{
public:
	c_async_logger_impl();
	~c_async_logger_impl();
	int start_logging(const char* lpc_full_path_name,unsigned long long ll_roll_size,
		const int i_auto_flush_time_ms= AUTO_FLUSH_INTERVAL_TIME_MS,
		const int i_log_buffer_max_cache_size = LOG_BUFFER_MAX_CACHE_SIZE);
	int append_log(const char* lpc_content,int i_data_len);
	int stop_logging();
protected:
private:
	unsigned long long					m_ll_roll_size;
	std::unique_ptr<std::thread>	m_thread;
	count_down_latch					m_latch;
	std::mutex								m_mutex;
	std::condition_variable				m_condition;
	std::string									m_str_filename;
	FILE*											m_file;
	int											m_i_filename_index;
	int											m_i_auto_flush_time_ms;
	int											m_i_log_buffer_max_cache_size;
	unsigned long long                   m_ll_current_file_size;
	bool											m_b_running;
	std::unique_ptr<st_log_buffer_unit>					m_current_buffer;
	std::unique_ptr<st_log_buffer_unit>					m_next_buffer;
	buffer_container		m_ls_buffers;
	unsigned	int			_work_thread();
	void							_append_to_file(const char* lpc_content,size_t data_len);
	void							_flush_to_file();
	int							_create_file(const char* lpc_filename);
	int							_close_file();
};