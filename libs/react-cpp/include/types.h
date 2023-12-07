/**
 *  Types.h
 *
 *  File containing all type definitions for the LIBEV-CPP library
 *
 *  @copyright 2014 Copernica BV
 */

#include <functional>

/**
 *  Set up namespace
 */
namespace React {

/**
 *  Forward declarations
 */
class ReadWatcher;
class WriteWatcher;
class TimeoutWatcher;
class IntervalWatcher;
class SynchronizeWatcher;
class CleanupWatcher;
class SignalWatcher;
class StatusWatcher;

/**
 *  Timestamp type is a wrapper around libev
 */
using Timestamp = ev_tstamp;
using ReadCallback = std::function<bool()>;
using WriteCallback = std::function<bool()>;
using TimeoutCallback = std::function<void()>;
using IntervalCallback = std::function<bool()>;
using SynchronizeCallback = std::function<void()>;
using CleanupCallback = std::function<void()>;
using SignalCallback = std::function<bool()>;
using StatusCallback = std::function<bool(pid_t,int)>;

/**
 *  End namespace
 */
}

