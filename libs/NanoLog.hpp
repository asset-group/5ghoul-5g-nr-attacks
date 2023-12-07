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

#ifndef NANO_LOG_HEADER_GUARD
#define NANO_LOG_HEADER_GUARD

#include <thread>
#include <cstdint>
#include <memory>
#include <string>
#include <iosfwd>
#include <atomic>
#include <queue>
#include <fstream>
#include <type_traits>

namespace nanolog
{
	enum class LogLevel : uint8_t
	{
		INFO,
		WARN,
		CRIT
	};

	class NanoLogLine
	{
	public:
		NanoLogLine(LogLevel level, char const *file, char const *function, uint32_t line);
		~NanoLogLine();

		NanoLogLine(NanoLogLine &&) = default;
		NanoLogLine &operator=(NanoLogLine &&) = default;

		void stringify(std::ostream &os);

		NanoLogLine &operator<<(char arg);
		NanoLogLine &operator<<(wchar_t arg);
		NanoLogLine &operator<<(int32_t arg);
		NanoLogLine &operator<<(uint32_t arg);
		NanoLogLine &operator<<(int64_t arg);
		NanoLogLine &operator<<(uint64_t arg);
		NanoLogLine &operator<<(double arg);
		NanoLogLine &operator<<(std::string const &arg);
		NanoLogLine &operator<<(std::wstring const &arg);

		template <size_t N>
		NanoLogLine &operator<<(const char (&arg)[N])
		{
			encode(string_literal_t(arg));
			return *this;
		}

		template <typename Arg>
		typename std::enable_if<std::is_same<Arg, char const *>::value, NanoLogLine &>::type
		operator<<(Arg const &arg)
		{
			encode(arg);
			return *this;
		}

		template <typename Arg>
		typename std::enable_if<std::is_same<Arg, char *>::value, NanoLogLine &>::type
		operator<<(Arg const &arg)
		{
			encode(arg);
			return *this;
		}

		struct string_literal_t
		{
			explicit string_literal_t(char const *s) : m_s(s) {}
			char const *m_s;
		};

	private:
		char *buffer();

		template <typename Arg>
		void encode(Arg arg);

		template <typename Arg>
		void encode(Arg arg, uint8_t type_id);

		void encode(char *arg);
		void encode(char const *arg);
		void encode(string_literal_t arg);
		void encode_c_string(char const *arg, size_t length);
		void resize_buffer_if_needed(size_t additional_bytes);
		void stringify(std::ostream &os, char *start, char const *const end);

	private:
		size_t m_bytes_used;
		size_t m_buffer_size;
		std::unique_ptr<char[]> m_heap_buffer;
		char m_stack_buffer[256 - 2 * sizeof(size_t) - sizeof(decltype(m_heap_buffer)) - 8 /* Reserved */];
	};

	/*
     * Non guaranteed logging. Uses a ring buffer to hold log lines.
     * When the ring gets full, the previous log line in the slot will be dropped.
     * Does not block producer even if the ring buffer is full.
     * ring_buffer_size_mb - LogLines are pushed into a mpsc ring buffer whose size
     * is determined by this parameter. Since each LogLine is 256 bytes, 
     * ring_buffer_size = ring_buffer_size_mb * 1024 * 1024 / 256
     */
	struct NonGuaranteedLogger
	{
		NonGuaranteedLogger(uint32_t ring_buffer_size_mb_) : ring_buffer_size_mb(ring_buffer_size_mb_) {}
		uint32_t ring_buffer_size_mb;
	};

	/*
     * Provides a guarantee log lines will not be dropped. 
     */
	struct GuaranteedLogger
	{
	};

	struct BufferBase
	{
		virtual ~BufferBase() = default;
		virtual void push(NanoLogLine &&logline) = 0;
		virtual bool try_pop(NanoLogLine &logline) = 0;
	};

	struct SpinLock
	{
		SpinLock(std::atomic_flag &flag) : m_flag(flag)
		{
			while (m_flag.test_and_set(std::memory_order_acquire))
				;
		}

		~SpinLock()
		{
			m_flag.clear(std::memory_order_release);
		}

	private:
		std::atomic_flag &m_flag;
	};

	/* Multi Producer Single Consumer Ring Buffer */
	class RingBuffer : public BufferBase
	{
	public:
		struct alignas(64) Item
		{
			Item()

				: flag{ATOMIC_FLAG_INIT}, written(0), logline(LogLevel::INFO, nullptr, nullptr, 0)
			{
			}

			std::atomic_flag flag;
			char written;
			char padding[256 - sizeof(std::atomic_flag) - sizeof(char) - sizeof(NanoLogLine)];
			NanoLogLine logline;
		};

		RingBuffer(size_t const size)
			: m_size(size), m_ring(static_cast<Item *>(std::malloc(size * sizeof(Item)))), m_write_index(0), m_read_index(0)
		{
			for (size_t i = 0; i < m_size; ++i)
			{
				new (&m_ring[i]) Item();
			}
			static_assert(sizeof(Item) == 256, "Unexpected size != 256");
		}

		~RingBuffer()
		{
			for (size_t i = 0; i < m_size; ++i)
			{
				m_ring[i].~Item();
			}
			std::free(m_ring);
		}

		void push(NanoLogLine &&logline) override
		{
			unsigned int write_index = m_write_index.fetch_add(1, std::memory_order_relaxed) % m_size;
			Item &item = m_ring[write_index];
			SpinLock spinlock(item.flag);
			item.logline = std::move(logline);
			item.written = 1;
		}

		bool try_pop(NanoLogLine &logline) override
		{
			Item &item = m_ring[m_read_index % m_size];
			SpinLock spinlock(item.flag);
			if (item.written == 1)
			{
				logline = std::move(item.logline);
				item.written = 0;
				++m_read_index;
				return true;
			}
			return false;
		}

		RingBuffer(RingBuffer const &) = delete;
		RingBuffer &operator=(RingBuffer const &) = delete;

	private:
		size_t const m_size;
		Item *m_ring;
		std::atomic<unsigned int> m_write_index;
		unsigned int m_read_index;
	};

	class Buffer
	{
	public:
		struct Item
		{
			Item(NanoLogLine &&nanologline) : logline(std::move(nanologline)) {}
			char padding[256 - sizeof(NanoLogLine)];
			NanoLogLine logline;
		};

		static constexpr const size_t size = 32768; // 8MB. Helps reduce memory fragmentation

		Buffer() : m_buffer(static_cast<Item *>(std::malloc(size * sizeof(Item))))
		{
			for (size_t i = 0; i <= size; ++i)
			{
				m_write_state[i].store(0, std::memory_order_relaxed);
			}
			static_assert(sizeof(Item) == 256, "Unexpected size != 256");
		}

		~Buffer()
		{
			unsigned int write_count = m_write_state[size].load();
			for (size_t i = 0; i < write_count; ++i)
			{
				m_buffer[i].~Item();
			}
			std::free(m_buffer);
		}

		// Returns true if we need to switch to next buffer
		bool push(NanoLogLine &&logline, unsigned int const write_index)
		{
			new (&m_buffer[write_index]) Item(std::move(logline));
			m_write_state[write_index].store(1, std::memory_order_release);
			return m_write_state[size].fetch_add(1, std::memory_order_acquire) + 1 == size;
		}

		bool try_pop(NanoLogLine &logline, unsigned int const read_index)
		{
			if (m_write_state[read_index].load(std::memory_order_acquire))
			{
				Item &item = m_buffer[read_index];
				logline = std::move(item.logline);
				return true;
			}
			return false;
		}

		Buffer(Buffer const &) = delete;
		Buffer &operator=(Buffer const &) = delete;

	private:
		Item *m_buffer;
		std::atomic<unsigned int> m_write_state[size + 1];
	};

	class QueueBuffer : public BufferBase
	{
	public:
		QueueBuffer(QueueBuffer const &) = delete;
		QueueBuffer &operator=(QueueBuffer const &) = delete;

		QueueBuffer() : m_current_read_buffer{nullptr}, m_write_index(0), m_flag{ATOMIC_FLAG_INIT}, m_read_index(0)
		{
			setup_next_write_buffer();
		}

		void push(NanoLogLine &&logline) override
		{
			unsigned int write_index = m_write_index.fetch_add(1, std::memory_order_relaxed);
			if (write_index < Buffer::size)
			{
				if (m_current_write_buffer.load(std::memory_order_acquire)->push(std::move(logline), write_index))
				{
					setup_next_write_buffer();
				}
			}
			else
			{
				while (m_write_index.load(std::memory_order_acquire) >= Buffer::size)
					;
				push(std::move(logline));
			}
		}

		bool try_pop(NanoLogLine &logline) override
		{
			if (m_current_read_buffer == nullptr)
				m_current_read_buffer = get_next_read_buffer();

			Buffer *read_buffer = m_current_read_buffer;

			if (read_buffer == nullptr)
				return false;

			if (bool success = read_buffer->try_pop(logline, m_read_index))
			{
				m_read_index++;
				if (m_read_index == Buffer::size)
				{
					m_read_index = 0;
					m_current_read_buffer = nullptr;
					SpinLock spinlock(m_flag);
					m_buffers.pop();
				}
				return true;
			}

			return false;
		}

	private:
		void setup_next_write_buffer()
		{
			std::unique_ptr<Buffer> next_write_buffer(new Buffer());
			m_current_write_buffer.store(next_write_buffer.get(), std::memory_order_release);
			SpinLock spinlock(m_flag);
			m_buffers.push(std::move(next_write_buffer));
			m_write_index.store(0, std::memory_order_relaxed);
		}

		Buffer *get_next_read_buffer()
		{
			SpinLock spinlock(m_flag);
			return m_buffers.empty() ? nullptr : m_buffers.front().get();
		}

	private:
		std::queue<std::unique_ptr<Buffer>> m_buffers;
		std::atomic<Buffer *> m_current_write_buffer;
		Buffer *m_current_read_buffer;
		std::atomic<unsigned int> m_write_index;
		std::atomic_flag m_flag;
		unsigned int m_read_index;
	};

	class FileWriter
	{
	public:
		FileWriter(std::string const &log_directory, std::string const &log_file_name, uint32_t log_file_roll_size_mb)
			: m_log_file_roll_size_bytes(log_file_roll_size_mb * 1024 * 1024), m_name(log_directory + log_file_name)
		{
			roll_file();
		}

		void write(NanoLogLine &logline)
		{
			auto pos = m_os->tellp();
			logline.stringify(*m_os);
			m_bytes_written += m_os->tellp() - pos;
			if (m_bytes_written > m_log_file_roll_size_bytes)
			{
				roll_file();
			}
		}

	private:
		void roll_file()
		{
			if (m_os)
			{
				m_os->flush();
				m_os->close();
			}

			m_bytes_written = 0;
			m_os.reset(new std::ofstream());

			std::string log_file_name = m_name;
			log_file_name.append(".");
			log_file_name.append(std::to_string(++m_file_number));
			log_file_name.append(".txt");
			m_os->open(log_file_name, std::ofstream::out | std::ofstream::trunc);
		}

	private:
		uint32_t m_file_number = 0;
		std::streamoff m_bytes_written = 0;
		uint32_t const m_log_file_roll_size_bytes;
		std::string const m_name;
		std::unique_ptr<std::ofstream> m_os;
	};

	class NanoLogger
	{
	public:
		NanoLogger(NonGuaranteedLogger ngl, std::string const &log_directory, std::string const &log_file_name, uint32_t log_file_roll_size_mb, int exclude_core = -1)
			: _exclude_core(exclude_core),
			  m_state(State::INIT),
			  m_buffer_base(new RingBuffer(std::max(1u, ngl.ring_buffer_size_mb) * 1024 * 4)),
			  m_file_writer(log_directory, log_file_name, std::max(1u, log_file_roll_size_mb)),
			  m_thread(&NanoLogger::pop, this)
		{
			m_state.store(State::READY, std::memory_order_release);
		}

		NanoLogger(GuaranteedLogger gl, std::string const &log_directory, std::string const &log_file_name, uint32_t log_file_roll_size_mb)
			: m_state(State::INIT), m_buffer_base(new QueueBuffer()), m_file_writer(log_directory, log_file_name, std::max(1u, log_file_roll_size_mb)), m_thread(&NanoLogger::pop, this)
		{
			m_state.store(State::READY, std::memory_order_release);
		}

		~NanoLogger()
		{
			m_state.store(State::SHUTDOWN);
			m_thread.join();
		}

		void add(NanoLogLine &&logline);
		void pop();

	private:
		enum class State
		{
			INIT,
			READY,
			SHUTDOWN
		};

		int _exclude_core = -1;

		std::atomic<State> m_state;
		std::unique_ptr<BufferBase> m_buffer_base;
		FileWriter m_file_writer;
		std::thread m_thread;
	};

	struct NanoLog
	{

		std::unique_ptr<NanoLogger> nanologger;
		std::atomic<NanoLogger *> atomic_nanologger;
		std::atomic<unsigned int> loglevel = {0};
		bool initialized = false;

		/*
     * Ensure initialize() is called prior to any log statements.
     * log_directory - where to create the logs. For example - "/tmp/"
     * log_file_name - root of the file name. For example - "nanolog"
     * This will create log files of the form -
     * /tmp/nanolog.1.txt
     * /tmp/nanolog.2.txt
     * etc.
     * log_file_roll_size_mb - mega bytes after which we roll to next log file.
     */
		void initialize(NonGuaranteedLogger ngl, std::string const &log_directory, std::string const &log_file_name, uint32_t log_file_roll_size_mb, bool append_time, int exclude_core = -1);
		void initialize(NonGuaranteedLogger ngl, std::string const &log_directory, std::string const &log_file_name, uint32_t log_file_roll_size_mb);
		void initialize(GuaranteedLogger gl, std::string const &log_directory, std::string const &log_file_name, uint32_t log_file_roll_size_mb, bool append_time);
		void initialize(GuaranteedLogger gl, std::string const &log_directory, std::string const &log_file_name, uint32_t log_file_roll_size_mb);

		void set_log_level(LogLevel level);
		bool is_logged(LogLevel level);

		/*
	 * Ideally this should have been operator+=
	 * Could not get that to compile, so here we are...
	 */
		bool operator==(NanoLogLine &);
	};

} // namespace nanolog

#define NANO_LOG(LOG_INST, LEVEL) LOG_INST == nanolog::NanoLogLine(LEVEL, __FILE__, __func__, __LINE__)
#define NANO_LOG_INFO(LOG_INST) LOG_INST.is_logged(nanolog::LogLevel::INFO) && NANO_LOG(LOG_INST, nanolog::LogLevel::INFO)
#define NANO_LOG_WARN(LOG_INST) LOG_INST.is_logged(nanolog::LogLevel::WARN) && NANO_LOG(LOG_INST, nanolog::LogLevel::WARN)
#define NANO_LOG_CRIT(LOG_INST) LOG_INST.is_logged(nanolog::LogLevel::CRIT) && NANO_LOG(LOG_INST, nanolog::LogLevel::CRIT)

#endif /* NANO_LOG_HEADER_GUARD */
