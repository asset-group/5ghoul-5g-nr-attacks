%module pylibmpsse
%include "stdint.i"
%{
#include "mpsse.h"
%}

%typemap(in) (char *data, int size)
{
        if(!PyString_Check($input))
        {
                PyErr_SetString(PyExc_ValueError, "String value required");
                return NULL;
        }

        $1 = PyString_AsString($input);
        $2 = PyString_Size($input);
}

%typemap(out) swig_string_data
{
        $result = PyBytes_FromStringAndSize($1.data, $1.size);
        free($1.data);
}

%include "mpsse.h"
