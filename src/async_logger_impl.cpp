#include "async_logger_impl.h"
#ifdef WIN32
#include <windows.h>
#endif
#include <sstream>

c_async_logger_impl::c_async_logger_impl()
	:m_latch(1),
	m_i_filename_index(0)
{

}

c_async_logger_impl::~c_async_logger_impl()
{

}

int c_async_logger_impl::start_logging(const char* lpc_full_path_name, unsigned long long ll_roll_size, const int i_auto_flush_time_ms/*= AUTO_FLUSH_INTERVAL_TIME_MS*/, const int i_log_buffer_max_cache_size /*= LOG_BUFFER_MAX_CACHE_SIZE*/)
{
	int i_ret = EM_ASYNC_LOG_ERROR_UNDEFINED;
	do 
	{
		if (NULL == lpc_full_path_name)
		{
			i_ret = EM_ASYNC_LOG_ERROR_PARAMS;
			break;
		}
		m_str_filename = lpc_full_path_name;
		m_ll_roll_size = ll_roll_size;
		m_i_auto_flush_time_ms = i_auto_flush_time_ms;
		m_i_log_buffer_max_cache_size = i_log_buffer_max_cache_size;
		m_current_buffer.reset(new st_log_buffer_unit());
		if (!m_current_buffer || 0 != m_current_buffer->init(i_log_buffer_max_cache_size))
		{
			i_ret = EM_ASYNC_LOG_ERROR_CACHE_BUFFER_CREATE;
			break;
		}

		m_next_buffer.reset(new st_log_buffer_unit());
		if (!m_next_buffer || 0 != m_next_buffer->init(i_log_buffer_max_cache_size))
		{
			i_ret = EM_ASYNC_LOG_ERROR_CACHE_BUFFER_CREATE;
			break;
		}

		i_ret = _create_file(lpc_full_path_name);
		if(EM_ASYNC_LOG_ERROR_NONE != i_ret)
			break;
		m_b_running = true;
		m_thread.reset(new std::thread(&c_async_logger_impl::_work_thread, this));
		m_latch.wait();
		i_ret = EM_ASYNC_LOG_ERROR_NONE;
	} while (false);
	return i_ret;
}

int c_async_logger_impl::append_log(const char* lpc_content, int i_data_len)
{
	int i_ret = EM_ASYNC_LOG_ERROR_UNDEFINED;
	do 
	{
		if (i_data_len > m_i_log_buffer_max_cache_size)
		{
			i_ret = EM_ASYNC_LOG_ERROR_LOG_DATA_TOO_BIG;
			break;
		}
		std::unique_lock<std::mutex> lock(m_mutex);
		if (m_current_buffer->available_len() > i_data_len)
		{
			m_current_buffer->append_data(lpc_content, i_data_len);
			i_ret = EM_ASYNC_LOG_ERROR_NONE;
			break;
		}
		m_ls_buffers.push_back(std::move(m_current_buffer));
		if (m_next_buffer)
		{
			m_current_buffer = std::move(m_next_buffer);
		}
		else
		{
			//new buffer;
			m_current_buffer.reset(new st_log_buffer_unit);
			if (0 != m_current_buffer->init(m_i_log_buffer_max_cache_size))
			{
				i_ret = EM_ASYNC_LOG_ERROR_CACHE_BUFFER_CREATE;
				break;
			}
		}
		m_current_buffer->append_data(lpc_content, i_data_len);
		m_condition.notify_one();
		i_ret = EM_ASYNC_LOG_ERROR_NONE;
	} while (false);
	return i_ret;
}

int c_async_logger_impl::stop_logging()
{
	m_b_running = false;
	m_condition.notify_one();
	m_thread->join();
	return 0;
}

unsigned	int c_async_logger_impl::_work_thread()
{
	m_latch.count_down();
	buffer_container temp_buffers;
	std::unique_ptr<st_log_buffer_unit> temp_current_buffer(new st_log_buffer_unit);
	temp_current_buffer->init(m_i_log_buffer_max_cache_size);
	std::unique_ptr<st_log_buffer_unit> temp_next_buffer(new st_log_buffer_unit);
	temp_next_buffer->init(m_i_log_buffer_max_cache_size);
	do 
	{
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			if (m_ls_buffers.empty())
			{
				m_condition.wait_for(lock, std::chrono::milliseconds(m_i_auto_flush_time_ms));
			}
			m_ls_buffers.push_back(std::move(m_current_buffer));
			m_current_buffer = std::move(temp_current_buffer);
			if (!m_next_buffer)
			{
				m_next_buffer = std::move(temp_next_buffer);
			}
			temp_buffers.swap(m_ls_buffers);
		}

		if (temp_buffers.size() > 3)
		{
			#ifdef WIN32
			OutputDebugStringA("buff too more \r\n");
			#else
			printf("buff too more \r\n");
			#endif
			//dop buffer;
			const char* lpc_notice = "drop buffer \r\n";
			const int i_notice_len = strlen(lpc_notice);
			_append_to_file(lpc_notice,i_notice_len);
			temp_buffers.resize(2);
		}
		buffer_container_cit it = temp_buffers.begin();
		for (; temp_buffers.end() != it; ++it)
		{
			_append_to_file((*it)->get_data(), (*it)->get_actual_data_len());
			(*it)->reset();
		}
		if (!temp_current_buffer)
		{
			temp_current_buffer = std::move(temp_buffers.back());
			temp_buffers.pop_back();
		}
		if (!temp_next_buffer)
		{
			temp_next_buffer = std::move(temp_buffers.back());
			temp_buffers.pop_back();
		}
		_flush_to_file();
		//roll 
		if (m_ll_current_file_size > m_ll_roll_size)
		{
			//do roll
			_close_file();
			std::ostringstream ss_roll_file_name;
			ss_roll_file_name << m_str_filename <<"."<<m_i_filename_index++;
			_create_file(ss_roll_file_name.str().c_str());
		}
	} while (m_b_running);
	_flush_to_file();
	return 0;
}

void c_async_logger_impl::_append_to_file(const char* lpc_content, size_t data_len)
{
	if (!m_file || !data_len)
		return;
	size_t writed_offset = fwrite(lpc_content, 1,data_len,m_file);
	size_t remain = data_len - writed_offset;
	while (remain > 0)
	{
		size_t x = fwrite(lpc_content+ writed_offset, 1, data_len, m_file);
		if (x == 0)
		{
			int err = ferror(m_file);
			if (err)
			{
				//
			}
			break;
		}
		writed_offset += x;
		remain = data_len - writed_offset; // remain -= x
	}
	m_ll_current_file_size += data_len;
}

void c_async_logger_impl::_flush_to_file()
{
	if (m_file)
		fflush(m_file);
}
#ifdef WIN32
int c_async_logger_impl::_create_file(const char* lpc_filename)
{
	int i_ret = EM_ASYNC_LOG_ERROR_UNDEFINED;
	do 
	{
		m_file = _fsopen(lpc_filename, "ab", _SH_DENYWR);
		if (!m_file)
		{
			i_ret = EM_ASYNC_LOG_ERROR_OPEN_FILE_FAILED;
			break;
		}
		if (0 != _fseeki64(m_file, 0, SEEK_END))
		{
			i_ret = EM_ASYNC_LOG_ERROR_SEEK_FILE_FAILED;
			break;
		}
		m_ll_current_file_size = _ftelli64(m_file);
		i_ret = EM_ASYNC_LOG_ERROR_NONE;
	} while (false);
	return i_ret;
}
#else
int c_async_logger_impl::_create_file(const char* lpc_filename)
{
	int i_ret = EM_ASYNC_LOG_ERROR_UNDEFINED;
	do 
	{
		m_file = fopen64(lpc_filename, "ab");
		if (!m_file)
		{
			i_ret = EM_ASYNC_LOG_ERROR_OPEN_FILE_FAILED;
			break;
		}
		if (0 != fseeko64(m_file, 0, SEEK_END))
		{
			i_ret = EM_ASYNC_LOG_ERROR_SEEK_FILE_FAILED;
			break;
		}
		m_ll_current_file_size = ftello64(m_file);
		i_ret = EM_ASYNC_LOG_ERROR_NONE;
	} while (false);
	return i_ret;
}
#endif
int c_async_logger_impl::_close_file()
{
	if (m_file)
	{
		fclose(m_file);
		m_file = NULL;
	}
	return EM_ASYNC_LOG_ERROR_NONE;
}

count_down_latch::count_down_latch(int count)
	:m_count(count)
{

}

void count_down_latch::wait()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	while (m_count > 0)
	{
		m_condition.wait(lock);
	}
}

void count_down_latch::count_down()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	--m_count;
	if (m_count == 0)
	{
		m_condition.notify_all();
	}
}

int count_down_latch::get_count() const
{
	std::unique_lock<std::mutex> lock(m_mutex);
	return m_count;
}
//
#ifdef __cplusplus
extern "C" {
#endif
	ASYNC_LOGGER_API  int init_async_logger_instance(void** lpp_instance, int iReserved)
	{
		int i_ret = 0;
		do
		{
			c_async_logger_impl* lp_impl = new c_async_logger_impl;
			*lpp_instance = lp_impl;
		}while(false);
		return 0;
	}
	ASYNC_LOGGER_API  int destroy_async_logger_instance(void** lpp_instance)
	{
		return 0;
	}
#ifdef __cplusplus
};
#endif