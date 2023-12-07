/**
 *  Buffer.h
 *
 *  Class that buffers data that was received from a connection or that is
 *  going to be sent to a connection
 *
 *  @author Emiel Bruijntjes <emiel.bruijntjes@copernica.com>
 *  @copyright 2014 Copernica BV
 */

/**
 *  Set up namespace
 */
namespace React { namespace Tcp {

/**
 *  Class definition
 */
class Buffer
{
private:
    /**
     *  Buffer part helper class
     */
    template <int CAPACITY>
    class BufferPart
    {
    private:
        /**
         *  Array of iovec objects
         *  @var    iovec[]
         */
        struct iovec _data[CAPACITY];

        /**
         *  First allocated memory block
         *
         *  We need to keep this in case we are removing
         *  a partial iovec structure. We increment the
         *  data pointer in the structure, but we need to
         *  retain the original address to free the data
         */
        void *_allocated = nullptr;

        /**
         *  First item that is filled
         *  @var    int
         */
        int _first = 0;

        /**
         *  Item that is being filled
         *  @var    int
         */
        int _current = 0;

        /**
         *  Total size of the buffer
         *  @var    size_t
         */
        size_t _size = 0;

    public:
        /**
         *  Constructor
         */
        BufferPart()
        {
            // set everything to zero
            memset(_data, 0, sizeof(_data));
        }

        /**
         *  Destructor
         */
        virtual ~BufferPart()
        {
            // skip if empty
            if (_size == 0) return;

            // delete the first part
            free(_allocated);

            // loop through all following buffers
            // skip first part because we already deleted _allocated
            for (int i = _first + 1; i < _current; i++) free(_data[i].iov_base);
        }

        /**
         *  Add data to the buffer
         *  @param  data        data to add
         *  @param  size        size of the data
         *  @return             number of bytes added
         */
        size_t add(const void *data, size_t size)
        {
            // skip if capacity has been reached
            if (_current >= CAPACITY) return 0;

            // allocate a buffer
            auto *buffer = malloc(size);

            // allocate data in last element
            _data[_current].iov_base = buffer;
            _data[_current].iov_len = size;

            // copy data
            memcpy(_data[_current].iov_base, data, size);

            // update total size
            _size += size;

            // is this the first block? then update the
            // pointer to be deleted
            if (_first == _current) _allocated = buffer;

            // move current for next call
            _current++;

            // done
            return size;
        }

        /**
         *  Shrink the buffer with a number of bytes
         *  @param  size        size to shrink
         *  @return             actual number of bytes that the buffer got smaller
         */
        size_t shrink(size_t size)
        {
            // skip if buffer is empty
            if (_size == 0) return 0;

            // result variable
            size_t result = 0;

            // keep looping until there is data to shrink
            while (size > 0 && _size > 0)
            {
                // check the first buffer, is it in total big enough?
                if (_data[_first].iov_len <= size)
                {
                    // the first buffer can be removed in total
                    free(_allocated);

                    // update number of bytes
                    result += _data[_first].iov_len;
                    _size -= _data[_first].iov_len;
                    size -= _data[_first].iov_len;

                    // advance a position
                    ++_first;

                    // if no more vectors exist, there is nothing left to
                    // delete, otherwise we set the allocated member to the
                    // next vector that will be deleted
                    if (_first == _current) _allocated = nullptr;
                    else _allocated = _data[_first].iov_base;
                }
                else
                {
                    // we should only update the first buffer
                    _data[_first].iov_base = static_cast<void*>(static_cast<char*>(_data[_first].iov_base) + size);
                    _data[_first].iov_len -= size;

                    // update counters
                    result += size;
                    _size -= size;
                    size -= size;
                }
            }

            // done
            return result;
        }

        /**
         *  Find a certain character in the data
         *  @param  c       character to find
         *  @return         position of the character, or -1 if not found
         */
        ssize_t find(int c) const
        {
            // number of bytes processed so far
            size_t processed = 0;

            // loop through the elements
            for (int i=_first; i<_current; i++)
            {
                // check if found in this buffer
                char *result = (char *)memchr(_data[i].iov_base, c, _data[i].iov_len);
                if (result) return processed + (result - (char *)_data[i].iov_base);

                // move to next buffer
                processed += _data[i].iov_len;
            }

            // not found
            return -1;
        }

        /**
         *  Get a number of bytes (and shrink the buffer
         *  @param  buffer      buffer to fill
         *  @param  size_t      size of the buffer
         *  @return             number of bytes read
         */
        size_t read(char *buffer, size_t size)
        {
            // number of bytes processed
            size_t processed = 0;

            // keep looping
            for (int i=_first; i<_current; i++)
            {
                // number of bytes to copy
                size_t tocopy = std::min(size - processed, _data[i].iov_len);

                // copy this buffer
                memcpy(buffer + processed, _data[i].iov_base, tocopy);

                // update counters
                processed += tocopy;

                // are we done?
                if (processed >= size) return shrink(processed);
            }

            // buffer is now empty
            return shrink(processed);
        }

        /**
         *  Retrieve pointer to filled iovec structure
         *  @return struct iovec*
         */
        const struct iovec *iovec() const
        {
            return &_data[_first];
        }

        /**
         *  Retrieve the number of filled iovec records
         *  @return int
         */
        int count() const
        {
            return _current - _first;
        }

        /**
         *  Size of the buffer part
         *  @return size_t
         */
        size_t size() const
        {
            return _size;
        }
    };

    /**
     *  List of buffer parts
     *  @var    std::list
     */
    std::list<std::unique_ptr<BufferPart<128>>> _parts;

public:
    /**
     *  Constructor
     */
    Buffer() {}

    /**
     *  Destructor
     */
    virtual ~Buffer() {}

    /**
     *  Add data to the buffer
     *  @param  data        data to add
     *  @param  size        size of the data
     *  @return             number of bytes added
     */
    size_t add(const void *data, size_t size)
    {
        // add to the back
        size_t added = _parts.empty() ? 0 : _parts.back()->add(data, size);

        // keep adding parts until we are done
        while (added < size)
        {
            // create a new item
            _parts.emplace_back(new BufferPart<128>());

            // and store data in it
            added += _parts.back()->add((const char*)data + added, size);
        }

        // return the number of bytes added
        return added;
    }

    /**
     *  Shrink the buffer with a certain size
     *  @param  size        the number of bytes that the buffer should shrink
     *  @return             number of bytes that the buffer actually shrunk
     */
    size_t shrink(size_t size)
    {
        // result variable
        size_t result = 0;

        // keep doing this for as there are buffer parts
        while (_parts.size() > 0)
        {
            // shrink it
            result += _parts.front()->shrink(size - result);

            // is the first part now empty?
            if (_parts.front()->size() == 0) _parts.erase(_parts.begin());

            // leap out if we had enough
            if (result >= size) return result;
        }

        // the entire list is empty now
        return result;
    }

    /**
     *  Find a certain character in the data
     *  @param  c       character to find
     *  @return         position of the character, or -1 if not found
     */
    ssize_t find(int c) const
    {
        // number of bytes processed so far
        size_t processed = 0;

        // loop through the parts
        for (auto &part : _parts)
        {
            // try to find it
            ssize_t pos = part->find(c);
            if (pos >= 0) return processed + pos;

            // move on to the next part
            processed += part->size();
        }

        // not found
        return -1;
    }

    /**
     *  Get a number of bytes (and shrink the buffer
     *  @param  buffer      buffer to fill
     *  @param  size_t      size of the buffer
     *  @return             number of bytes read
     */
    size_t read(char *buffer, size_t size)
    {
        // number of bytes processed
        size_t processed = 0;

        // keep doing this for as there are buffer parts
        while (_parts.size() > 0)
        {
            // read data
            processed += _parts.front()->read(buffer + processed, size - processed);

            // is the first part now empty?
            if (_parts.front()->size() == 0) _parts.erase(_parts.begin());

            // are we done?
            if (processed >= size) return processed;
        }

        // not everything could be read, buffer is completely empty now
        return processed;
    }

    /**
     *  Total size of the buffer
     *  @return size_t
     */
    size_t size() const
    {
        // result variable
        size_t result = 0;

        // loop through the parts
        for (auto &part : _parts) result += part->size();

        // done
        return result;
    }

    /**
     *  Retrieve pointer to filled iovec structure
     *  @return struct iovec*
     */
    const struct iovec *iovec() const
    {
        // are there any parts in the buffer?
        if (!_parts.empty()) return _parts.front()->iovec();

        // nothing
        return nullptr;
    }

    /**
     *  Retrieve the number of filled iovec records
     *  @return int
     */
    int count() const
    {
        // are there any parts in the buffer?
        if (!_parts.empty()) return _parts.front()->count();

        // not found
        return 0;
    }

    /**
     *  Clear the entire buffer
     */
    void clear()
    {
        _parts.clear();
    }
};

/**
 *  End namespace
 */
}}

