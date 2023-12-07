%module wdissector

%{
#include <stdint.h>
#include "wdissector.h"
extern const char *wdissector_version_info();
extern const char *wdissector_profile_info();
%}

%include <carrays.i>
%include <stdint.i>

%include wdissector.h
