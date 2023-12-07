/**
 *  include.h
 * 
 *  Main header file for the REACT-CPP wrapper library
 * 
 *  @copyright 2014 Copernica BV
 */
#ifndef include_H
#define include_H

/**
 *  C dependencies
 */
#include <ares.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/**
 *  C++ dependencies
 */
#include <memory>
#include <set>
#include <map>
#include <stdexcept>
#include <iostream>
#include <list>
#include <cstring>

/**
 *  Other include files
 */
#include "libev/ev.h"
#include "include/exception.h"
#include "include/types.h"
#include "include/loop.h"
#include "include/loopreference.h"
#include "include/mainloop.h"
#include "include/worker.h"
#include "include/watcher.h"
#include "include/deferred.h"
#include "include/uint128_t.h"
#include "include/watchers/read.h"
#include "include/watchers/write.h"
#include "include/watchers/signal.h"
#include "include/watchers/synchronize.h"
#include "include/watchers/cleanup.h"
#include "include/watchers/timeout.h"
#include "include/watchers/interval.h"
#include "include/watchers/status.h"
#include "include/fd.h"
#include "include/pipe.h"
#include "include/readpipe.h"
#include "include/writepipe.h"
#include "include/fullpipe.h"
#include "include/process.h"
#include "include/net/ipv4.h"
#include "include/net/ipv6.h"
#include "include/net/ip.h"
#include "include/net/address.h"
#include "include/dns/iprecord.h"
#include "include/dns/mxrecord.h"
#include "include/dns/mxresult.h"
#include "include/dns/channel.h"
#include "include/dns/types.h"
#include "include/dns/base.h"
#include "include/dns/resolver.h"
#include "include/tcp/exception.h"
#include "include/tcp/types.h"
#include "include/tcp/address.h"
#include "include/tcp/socketaddress.h"
#include "include/tcp/peeraddress.h"
#include "include/tcp/socket.h"
#include "include/tcp/server.h"
#include "include/tcp/connection.h"
#include "include/tcp/buffer.h"
#include "include/tcp/out.h"
#include "include/tcp/in.h"

/**
 *  End if
 */
#endif
