/**
 *  Worker.cpp
 *
 *  @copyright 2014 Copernica BV
 */
#include "includes.h"

/**
 *  Set up namespace
 */
namespace React {

/**
 *  Constructor
 *
 *  Create a worker that will execute code from the context of the main thread.
 *
 *  @param  loop
 */
Worker::Worker(Loop *loop)
{
    // create a new worker around the loop
    _impl = new LoopWorkerImpl(loop);
}

/**
 *  Constructor
 *
 *  Create a worker that will execute code in another thread.
 */
Worker::Worker()
{
    // create a new threaded worker
    _impl = new ThreadWorkerImpl();
}

/**
 *  Destructor
 */
Worker::~Worker()
{
    // clean up implementation
    delete _impl;
}

/**
 *  Execute a function
 *
 *  @param  function    the code to execute
 */
void Worker::execute(const std::function<void()> &function)
{
    // let the worker execute the code
    _impl->execute(function);
}

/**
 *  End namespace
 */
}
