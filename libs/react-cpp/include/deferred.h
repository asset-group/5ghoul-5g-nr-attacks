/**
 *  Deferred.h
 *
 *  Object to return a deferred result
 *  from an asynchronous function
 *
 *  @copyright 2014 Copernica BV
 */

/**
 *  Set up namespace
 */
namespace React {

/**
 *  Deferred class
 */
template <typename value_type, typename Friend1, typename Friend2 = Friend1, typename Friend3 = Friend2>
class Deferred
{
private:
    /**
     *  Callback to execute on success
     */
    std::function<void(value_type)> _successCallback;

    /**
     *  Callback to execute on failure
     */
    std::function<void(const char *error)> _failureCallback;

    /**
     *  Callback to execute either way
     */
    std::function<void()> _completeCallback;

    /**
     *  Register success and run the callbacks
     *
     *  @param  result  the result of the operation
     */
    void success(value_type result)
    {
        // execute the callbacks
        if (_successCallback)   _successCallback(result);
        if (_completeCallback)  _completeCallback();
    }

    /**
     *  Register failure and run the callbacks
     *
     *  @param  error  a human readable error message
     */
    void failure(const char *error)
    {
        // execute the callbacks
        if (_failureCallback)   _failureCallback(error);
        if (_completeCallback)  _completeCallback();
    }
public:
    /**
     *  Constructor
     */
    Deferred() {}

    /**
     *  We cannot be copied
     */
    Deferred(const Deferred& that) = delete;

    /**
     *  Nor can we be moved
     */
    Deferred(Deferred&& that) = delete;

    /**
     *  Register callback to retrieve the result
     *  in case of successful execution
     */
    Deferred& onSuccess(const std::function<void(value_type)>& callback)
    {
        // store the callback
        _successCallback = callback;
        return *this;
    }

    /**
     *  Register callback to retrieve an error message
     *  in case of failed execution
     */
    Deferred& onFailure(const std::function<void(const char*)>& callback)
    {
        // store the callback
        _failureCallback = callback;
        return *this;
    }

    /**
     *  Register callback that will be execute upon completion
     *  regardless whether we were successful or not
     */
    Deferred& onComplete(const std::function<void()>& callback)
    {
        // store the callback
        _completeCallback = callback;
        return *this;
    }

    // We have three good friends that we trust with our internals
    friend Friend1;
    friend Friend2;
    friend Friend3;
};

/**
 *  End namespace
 */
}
