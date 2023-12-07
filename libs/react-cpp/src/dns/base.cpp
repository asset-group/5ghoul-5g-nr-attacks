/**
 *  Base.cpp
 *
 *  @copyright 2014 Copernica BV
 */
#include "includes.h"

/**
 *  Set up namespace
 */
namespace React { namespace Dns {

/**
 *  Constructor
 *  @param  loop        Loop in which the resolver is activated
 */
Base::Base(Loop *loop) : _loop(loop), _channel(new Channel(loop))
{}

/**
 *  End namespace
 */
}}

