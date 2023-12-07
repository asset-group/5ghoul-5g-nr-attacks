/**
 *  pipe.h
 *
 *  A class representing a non-blocking pipe, used
 *  for communicating between different processes.
 *
 *  @copyright 2015 Copernica B.V.
 */

/**
 *  Set up namespace
 */
namespace React {

/**
 *  Class definition
 */
class Pipe
{
protected:
    /**
     *  The file descriptors used in the pipe
     *  @var    std::array<int, 2>
     */
    std::array<int, 2> _fds;

public:
    /**
     *  Constructor
     *  @param  flags       Optional flags (see man pipe2)
     */
    Pipe(int flags = 0)
    {
        // initialize the pipes
        if (pipe2(_fds.data(), flags)) throw std::runtime_error(strerror(errno));
    }

    /**
     *  Destructor
     */
    virtual ~Pipe()
    {
        // clean up the pipes
        for (auto fd : _fds)
        {
            // check if it is a valid file descriptor and close it
            // @todo we should check the return value
            if (fd >= 0) close(fd);
        }
    }

    /**
     *  Close the readable side of the pipe
     *  @return bool    true on success, false on failure
     */
    bool closeRead()
    {
        // try to close the fd
        if (_fds[0] <= 0 || close(_fds[0]) != 0) return false;

        // store invalid fd
        _fds[0] = -1;

        // done
        return true;
    }

    /**
     *  Close the writable side of the pipe
     *  @return bool    true on success, false on failure
     */
    bool closeWrite()
    {
        // try to close the fd
        if (_fds[1] <= 0 || close(_fds[1]) != 0) return false;

        // store invalid fd
        _fds[1] = -1;

        // done
        return true;
    }

    /**
     *  The filedescriptor for reading
     *  @return int
     */
    int readFd() const
    {
        return _fds[0];
    }

    /**
     *  The filedescriptor for writing
     *  @return int
     */
    int writeFd() const
    {
        return _fds[1];
    }
};

/**
 *  End namespace
 */
}
