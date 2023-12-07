#include "process.hpp"

namespace TinyProcessLib
{

  Process::Process(const string_type command, const string_type path,
                   std::function<void(const char *bytes, size_t n)> read_stdout,
                   std::function<void(const char *bytes, size_t n)> read_stderr,
                   const std::string process_cwd,
                   bool detached,
                   bool open_stdin, size_t buffer_size) : closed(true), read_stdout(std::move(read_stdout)), read_stderr(std::move(read_stderr)),
                                                                   _detached(detached),
                                                                   open_stdin(open_stdin), buffer_size(buffer_size)
  {
    this->process_cwd = process_cwd;
    open(command, path);
    async_read();
  }


  Process::id_type Process::get_id()
  {
    return data.id;
  }

  bool Process::write(const std::string &data)
  {
    return write(data.c_str(), data.size());
  }

} // namespace TinyProcessLib
