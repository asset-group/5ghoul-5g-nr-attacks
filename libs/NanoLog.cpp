/*

Distributed under the MIT License (MIT)

    Copyright (c) 2016 Karthik Iyengar

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in the 
Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished 
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS 
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER 
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "NanoLog.hpp"
#include <pthread.h>
#include <sched.h>
#include <cstring>
#include <chrono>
#include <ctime>
#include <thread>
#include <tuple>
#include <atomic>
#include <queue>
#include <fstream>

namespace
{

	/* Returns microseconds since epoch */
	static inline uint64_t timestamp_now()
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	}

	/* I want [2016-10-13 00:01:23.528514] */
	void format_timestamp(std::ostream &os, uint64_t timestamp)
	{
		// The next 3 lines do not work on MSVC!
		// auto duration = std::chrono::microseconds(timestamp);
		// std::chrono::high_resolution_clock::time_point time_point(duration);
		// std::time_t time_t = std::chrono::high_resolution_clock::to_time_t(time_point);
		if (!timestamp)
		{
			timestamp = timestamp_now();
		}
		std::time_t time_t = timestamp / 1000000;

		auto gmtime = std::localtime(&time_t);
		char buffer[32];
		strftime(buffer, 32, "%Y-%m-%d %T.", gmtime);

		char microseconds[7];
		sprintf(microseconds, "%06lu", timestamp % 1000000);
		os << '[' << buffer << microseconds << "] ";
	}

	std::thread::id this_thread_id()
	{
		static thread_local const std::thread::id id = std::this_thread::get_id();
		return id;
	}

	template <typename T, typename Tuple>
	struct TupleIndex;

	template <typename T, typename... Types>
	struct TupleIndex<T, std::tuple<T, Types...>>
	{
		static constexpr const std::size_t value = 0;
	};

	template <typename T, typename U, typename... Types>
	struct TupleIndex<T, std::tuple<U, Types...>>
	{
		static constexpr const std::size_t value = 1 + TupleIndex<T, std::tuple<Types...>>::value;
	};

} // anonymous namespace

namespace nanolog
{
	typedef std::tuple<char, uint32_t, uint64_t, int32_t, int64_t, double, NanoLogLine::string_literal_t, char *> SupportedTypes;

	char const *to_string(LogLevel loglevel)
	{
		switch (loglevel)
		{
		case LogLevel::INFO:
			return "INFO";
		case LogLevel::WARN:
			return "WARN";
		case LogLevel::CRIT:
			return "CRIT";
		}
		return "XXXX";
	}

	template <typename Arg>
	void NanoLogLine::encode(Arg arg)
	{
		*reinterpret_cast<Arg *>(buffer()) = arg;
		m_bytes_used += sizeof(Arg);
	}

	template <typename Arg>
	void NanoLogLine::encode(Arg arg, uint8_t type_id)
	{
		resize_buffer_if_needed(sizeof(Arg) + sizeof(uint8_t));
		encode<uint8_t>(type_id);
		encode<Arg>(arg);
	}

	NanoLogLine::NanoLogLine(LogLevel level, char const *file, char const *function, uint32_t line)
		: m_bytes_used(0), m_buffer_size(sizeof(m_stack_buffer))
	{
		encode<uint64_t>(timestamp_now());
		encode<std::thread::id>(this_thread_id());
		encode<string_literal_t>(string_literal_t(file));
		encode<string_literal_t>(string_literal_t(function));
		encode<uint32_t>(line);
		encode<LogLevel>(level);
	}

	NanoLogLine::~NanoLogLine() = default;

	void NanoLogLine::stringify(std::ostream &os)
	{
		char *b = (!m_heap_buffer ? &m_stack_buffer[m_bytes_used] : &(m_heap_buffer.get())[m_bytes_used]) + 1;
		char const *const end = b + m_bytes_used;
		format_timestamp(os, 0);
		os << b << std::endl;
	}

	template <typename Arg>
	char *decode(std::ostream &os, char *b, Arg *dummy)
	{
		Arg arg = *reinterpret_cast<Arg *>(b);
		os << arg;
		return b + sizeof(Arg);
	}

	template <>
	char *decode(std::ostream &os, char *b, NanoLogLine::string_literal_t *dummy)
	{
		NanoLogLine::string_literal_t s = *reinterpret_cast<NanoLogLine::string_literal_t *>(b);
		os << s.m_s;
		return b + sizeof(NanoLogLine::string_literal_t);
	}

	template <>
	char *decode(std::ostream &os, char *b, char **dummy)
	{
		while (*b != '\0')
		{
			os << *b;
			++b;
		}
		return ++b;
	}

	void NanoLogLine::stringify(std::ostream &os, char *start, char const *const end)
	{
		if (start == end)
			return;

		int type_id = static_cast<int>(*start);
		start++;

		switch (type_id)
		{
		case 0:
			stringify(os, decode(os, start, static_cast<std::tuple_element<0, SupportedTypes>::type *>(nullptr)), end);
			return;
		case 1:
			stringify(os, decode(os, start, static_cast<std::tuple_element<1, SupportedTypes>::type *>(nullptr)), end);
			return;
		case 2:
			stringify(os, decode(os, start, static_cast<std::tuple_element<2, SupportedTypes>::type *>(nullptr)), end);
			return;
		case 3:
			stringify(os, decode(os, start, static_cast<std::tuple_element<3, SupportedTypes>::type *>(nullptr)), end);
			return;
		case 4:
			stringify(os, decode(os, start, static_cast<std::tuple_element<4, SupportedTypes>::type *>(nullptr)), end);
			return;
		case 5:
			stringify(os, decode(os, start, static_cast<std::tuple_element<5, SupportedTypes>::type *>(nullptr)), end);
			return;
		case 6:
			stringify(os, decode(os, start, static_cast<std::tuple_element<6, SupportedTypes>::type *>(nullptr)), end);
			return;
		case 7:
			stringify(os, decode(os, start, static_cast<std::tuple_element<7, SupportedTypes>::type *>(nullptr)), end);
			return;
		}
	}

	char *NanoLogLine::buffer()
	{
		return !m_heap_buffer ? &m_stack_buffer[m_bytes_used] : &(m_heap_buffer.get())[m_bytes_used];
	}

	void NanoLogLine::resize_buffer_if_needed(size_t additional_bytes)
	{
		size_t const required_size = m_bytes_used + additional_bytes;

		if (required_size <= m_buffer_size)
			return;

		if (!m_heap_buffer)
		{
			m_buffer_size = std::max(static_cast<size_t>(512), required_size);
			m_heap_buffer.reset(new char[m_buffer_size]);
			memcpy(m_heap_buffer.get(), m_stack_buffer, m_bytes_used);
			return;
		}
		else
		{
			m_buffer_size = std::max(static_cast<size_t>(2 * m_buffer_size), required_size);
			std::unique_ptr<char[]> new_heap_buffer(new char[m_buffer_size]);
			memcpy(new_heap_buffer.get(), m_heap_buffer.get(), m_bytes_used);
			m_heap_buffer.swap(new_heap_buffer);
		}
	}

	void NanoLogLine::encode(char const *arg)
	{
		if (arg != nullptr)
			encode_c_string(arg, strlen(arg));
	}

	void NanoLogLine::encode(char *arg)
	{
		if (arg != nullptr)
			encode_c_string(arg, strlen(arg));
	}

	void NanoLogLine::encode_c_string(char const *arg, size_t length)
	{
		if (length == 0)
			return;
		resize_buffer_if_needed(1 + length + 1);
		char *b = buffer();
		auto type_id = TupleIndex<char *, SupportedTypes>::value;
		*reinterpret_cast<uint8_t *>(b++) = static_cast<uint8_t>(type_id);
		memcpy(b, arg, length + 1);
	}

	void NanoLogLine::encode(string_literal_t arg)
	{
		encode<string_literal_t>(arg, TupleIndex<string_literal_t, SupportedTypes>::value);
	}

	NanoLogLine &NanoLogLine::operator<<(std::string const &arg)
	{
		encode_c_string(arg.c_str(), arg.length());
		return *this;
	}

	NanoLogLine &NanoLogLine::operator<<(std::wstring const &arg)
	{
		const std::string s(arg.begin(), arg.end());
		encode_c_string(s.c_str(), s.length());
		return *this;
	}

	NanoLogLine &NanoLogLine::operator<<(int32_t arg)
	{
		encode<int32_t>(arg, TupleIndex<int32_t, SupportedTypes>::value);
		return *this;
	}

	NanoLogLine &NanoLogLine::operator<<(uint32_t arg)
	{
		encode<uint32_t>(arg, TupleIndex<uint32_t, SupportedTypes>::value);
		return *this;
	}

	NanoLogLine &NanoLogLine::operator<<(int64_t arg)
	{
		encode<int64_t>(arg, TupleIndex<int64_t, SupportedTypes>::value);
		return *this;
	}

	NanoLogLine &NanoLogLine::operator<<(uint64_t arg)
	{
		encode<uint64_t>(arg, TupleIndex<uint64_t, SupportedTypes>::value);
		return *this;
	}

	NanoLogLine &NanoLogLine::operator<<(double arg)
	{
		encode<double>(arg, TupleIndex<double, SupportedTypes>::value);
		return *this;
	}

	NanoLogLine &NanoLogLine::operator<<(char arg)
	{
		encode<char>(arg, TupleIndex<char, SupportedTypes>::value);
		return *this;
	}

	bool NanoLog::operator==(NanoLogLine &logline)
	{
		atomic_nanologger.load(std::memory_order_acquire)->add(std::move(logline));
		return true;
	}

	const char *append_timestamp_now(std::string const &log_file_name)
	{
		static char buf1[32];
		static char buf2[32];
		memset(buf1, 0, 32);
		memset(buf2, 0, 32);
		std::time_t time_t = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1000000;
		auto gmtime = std::gmtime(&time_t);

		strftime(buf1, 32, "%Y-%m-%d-%T", gmtime);
		snprintf(buf2, 32, "%s-%s", buf1, log_file_name.c_str());
		return buf2;
	}

	void NanoLog::initialize(NonGuaranteedLogger ngl, std::string const &log_directory, std::string const &log_file_name, uint32_t log_file_roll_size_mb, bool append_time, int exclude_core)
	{
		if (!initialized)
		{

			if (append_time)
				nanologger.reset(new NanoLogger(ngl, log_directory, append_timestamp_now(log_file_name), log_file_roll_size_mb, exclude_core));
			else
				nanologger.reset(new NanoLogger(ngl, log_directory, log_file_name, log_file_roll_size_mb, exclude_core));

			atomic_nanologger.store(nanologger.get(), std::memory_order_seq_cst);
			initialized = true;
		}
	}

	void NanoLog::initialize(NonGuaranteedLogger ngl, std::string const &log_directory, std::string const &log_file_name, uint32_t log_file_roll_size_mb)
	{
		initialize(ngl, log_directory, log_file_name, log_file_roll_size_mb, false);
	}

	void NanoLog::initialize(GuaranteedLogger gl, std::string const &log_directory, std::string const &log_file_name, uint32_t log_file_roll_size_mb, bool append_time)
	{
		if (!initialized)
		{
			if (append_time)
				nanologger.reset(new NanoLogger(gl, log_directory, append_timestamp_now(log_file_name), log_file_roll_size_mb));
			else
				nanologger.reset(new NanoLogger(gl, log_directory, log_file_name, log_file_roll_size_mb));

			initialized = true;
		}
	}

	void NanoLog::initialize(GuaranteedLogger gl, std::string const &log_directory, std::string const &log_file_name, uint32_t log_file_roll_size_mb)
	{
		initialize(gl, log_directory, log_file_name, log_file_roll_size_mb, false);
	}

	void NanoLog::set_log_level(LogLevel level)
	{
		loglevel.store(static_cast<unsigned int>(level), std::memory_order_release);
	}

	bool NanoLog::is_logged(LogLevel level)
	{
		return static_cast<unsigned int>(level) >= loglevel.load(std::memory_order_relaxed);
	}

	void NanoLogger::add(NanoLogLine &&logline)
	{
		m_buffer_base->push(std::move(logline));
	}

	void NanoLogger::pop()
	{

		pthread_setname_np(pthread_self(), "nanolog_thread");
		// Set schedule priority to IDLE(lowest priority)
		struct sched_param sp;
		sp.sched_priority = sched_get_priority_min(SCHED_IDLE);
		pthread_setschedparam(pthread_self(), SCHED_IDLE, &sp);

		// Wait for constructor to complete and pull all stores done there to this thread / core.
		while (m_state.load(std::memory_order_acquire) == State::INIT)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

		// Prevent thread from running on excluded core
		if (_exclude_core != -1)
		{
			cpu_set_t cpuset;
			int res = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
			if (res == 0)
			{
				CPU_CLR(_exclude_core, &cpuset);
				pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
			}
		}

		NanoLogLine logline(LogLevel::INFO, nullptr, nullptr, 0);

		while (m_state.load() == State::READY)
		{
			if (m_buffer_base->try_pop(logline))
			{
				m_file_writer.write(logline);
			}
			else
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		// Pop and log all remaining entries
		while (m_buffer_base->try_pop(logline))
		{
			m_file_writer.write(logline);
		}
	}

} // namespace nanolog
