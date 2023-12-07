%module wdissector

#pragma SWIG nowarn=315,317,389,451

%{
#include <mutex>
#include <csignal>
#include <iostream>
#include <stdlib.h>

// Libs - Misc
#include "json.hpp"
#include "libs/log_misc_utils.hpp"
#include "libs/mv_average.hpp"
#include "libs/oai_tracing/T_IDs.h"
#include "libs/termcolor.hpp"
#include "libs/zmq.hpp"
#include "libs/gvpp/gvpp.hpp"
#include "libs/cxxopts/cxxopts.hpp"

// Libs - Folly
#include "libs/folly/folly/FBVector.h"
#include "libs/folly/folly/concurrency/UnboundedQueue.h"
#include "libs/folly/folly/stats/TimeseriesHistogram.h"

#include <graphviz/gvc.h>

// Framework - Core Includes
#include "Machine.hpp"
#include "Framework.hpp"
#include "Modules.hpp"
#include "PacketLogger.hpp"
#include "Process.hpp"

// Framework - Drivers
#include "drivers/SHMDriver.hpp"

// Framework - Services
#include "services/ModemManager.hpp"
#include "services/ReportSender.hpp"
#include "services/USBHubControl.hpp"
#include "services/USBIP.hpp"

// Framework - Low-Level WDissector Library
extern "C" {
#include "libs/profiling.h"
#include "src/drivers/shm_interface/wd_shm.h"
}

%}

%include <stl.i>
%include <carrays.i>
%include <cstring.i>
%include <carrays.i>
%include <cdata.i>
%include <std_string.i>
%include <std_vector.i>
%include <std_map.i>
%include <stdint.i>
%include <pybuffer.i>

%pybuffer_mutable_string(unsigned char *);

%array_class(unsigned char, byteArray);
%apply unsigned int  { uint };



%typemap(out) uint8_t wd_read_filter {
	$result = SWIG_From_bool(static_cast< bool >($1));
}


%ignore Machine::ref_global_config;
%ignore badreadjmpbuf;

%immutable loop;
%immutable StateMachine;

%include src/wdissector.h
%include libs/react-cpp/reactcpp.h
%include libs/gvpp/gvpp.hpp
%include src/MiscUtils.hpp
%include src/GlobalConfig.hpp
%include src/Machine.hpp
%include src/Framework.hpp
%include src/Modules.hpp
%include src/PacketLogger.hpp
%include src/Process.hpp
%include src/services/ModemManager.hpp
%include src/services/ReportSender.hpp
%include src/services/USBHubControl.hpp
%include src/services/USBIP.hpp

%template(WDEventQueueInt) WDEventQueue<int>;
%template(WDEventQueuePacket) WDEventQueue<pkt_evt_t>;
%template(StateMapperVector) std::vector<state_mapper_t>;
%template(StringVector) std::vector<string>;
