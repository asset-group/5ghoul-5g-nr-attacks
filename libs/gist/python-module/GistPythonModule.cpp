#include <iostream>
#include <Python.h>
#include <assert.h>
#include <numpy/arrayobject.h>
#include "../src/Gist.h"

//=======================================================================
/** The Gist module */
Gist<double> gist (512, 44100);

//=======================================================================
static PyObject* setAudioFrameSize (PyObject *dummy, PyObject *args)
{
    int audioFrameSize;
    
    if (!PyArg_ParseTuple(args, "i", &audioFrameSize))
    {
        return NULL;
    }
    
    gist.setAudioFrameSize (audioFrameSize);
    
    return Py_BuildValue("");
}

//=======================================================================
static PyObject* setSamplingFrequency (PyObject *dummy, PyObject *args)
{
    int samplingFrequency;
    
    if (!PyArg_ParseTuple(args, "i", &samplingFrequency))
    {
        return NULL;
    }
    
    gist.setSamplingFrequency (samplingFrequency);
    
    return Py_BuildValue("");
}

//=======================================================================
static PyObject* getAudioFrameSize (PyObject *dummy, PyObject *args)
{
    return PyLong_FromLong((long) gist.getAudioFrameSize());
}

//=======================================================================
static PyObject* getSamplingFrequency (PyObject *dummy, PyObject *args)
{
    return PyLong_FromLong((long) gist.getSamplingFrequency());
}

//=======================================================================
static PyObject * processFrame (PyObject *dummy, PyObject *args)
{
    PyObject *arg1 = NULL;
    PyObject *arr1 = NULL;
    
    if (!PyArg_ParseTuple (args, "O", &arg1))
    {
        return NULL;
    }
    
    arr1 = PyArray_FROM_OTF (arg1, NPY_DOUBLE, NPY_IN_ARRAY);
    if (arr1 == NULL)
    {
        return NULL;
    }
    
    double* audioFrame = (double*) PyArray_DATA(arr1);
    long audioFrameSize = (int) PyArray_Size ((PyObject*)arr1);
    
    if (audioFrameSize != gist.getAudioFrameSize())
    {
        PyErr_SetString (PyExc_ValueError, "You are passing an audio frame with a different size to the frame size set in Gist. Use gist.getAudioFrameSize() to find out what is being used and change it with gist.setAudioFrameSize(frameSize)");
        return NULL;
    }
    
    gist.processAudioFrame (audioFrame, (unsigned long) audioFrameSize);
    
    Py_DECREF (arr1);
    
    return Py_BuildValue("");
}

//=======================================================================//
//===================  CORE TIME DOMAIN FEATURES ========================//
//=======================================================================//

//=======================================================================
static PyObject * rootMeanSquare (PyObject *dummy, PyObject *args)
{
    double rms = gist.rootMeanSquare();
    
    return PyFloat_FromDouble (rms);
}

//=======================================================================
static PyObject * peakEnergy (PyObject *dummy, PyObject *args)
{
    double p = gist.peakEnergy();
    
    return PyFloat_FromDouble (p);
}

//=======================================================================
static PyObject * zeroCrossingRate (PyObject *dummy, PyObject *args)
{
    double zcr = gist.zeroCrossingRate();
    
    return PyFloat_FromDouble (zcr);
}

//=======================================================================//
//================= CORE FREQUENCY DOMAIN FEATURES ======================//
//=======================================================================//

//=======================================================================
static PyObject * spectralCentroid (PyObject *dummy, PyObject *args)
{
    double sc = gist.spectralCentroid();
    
    return PyFloat_FromDouble (sc);
}

//=======================================================================
static PyObject * spectralCrest (PyObject *dummy, PyObject *args)
{
    double sc = gist.spectralCrest();
    
    return PyFloat_FromDouble (sc);
}

//=======================================================================
static PyObject * spectralFlatness (PyObject *dummy, PyObject *args)
{
    double sf = gist.spectralFlatness();
    
    return PyFloat_FromDouble (sf);
}

//=======================================================================
static PyObject * spectralRolloff (PyObject *dummy, PyObject *args)
{
    double sr = gist.spectralRolloff();
    
    return PyFloat_FromDouble (sr);
}

//=======================================================================
static PyObject * spectralKurtosis (PyObject *dummy, PyObject *args)
{
    double sk = gist.spectralKurtosis();
    
    return PyFloat_FromDouble (sk);
}

//=======================================================================//
//==================== ONSET DETECTION FUNCTIONS ========================//
//=======================================================================//

//=======================================================================
static PyObject * energyDifference (PyObject *dummy, PyObject *args)
{
    double ed = gist.energyDifference();
    
    return PyFloat_FromDouble (ed);
}

//=======================================================================
static PyObject * spectralDifference (PyObject *dummy, PyObject *args)
{
    double sd = gist.spectralDifference();
    
    return PyFloat_FromDouble (sd);
}

//=======================================================================
static PyObject * spectralDifferenceHWR (PyObject *dummy, PyObject *args)
{
    double sdhwr = gist.spectralDifferenceHWR();
    
    return PyFloat_FromDouble (sdhwr);
}

//=======================================================================
static PyObject * complexSpectralDifference (PyObject *dummy, PyObject *args)
{
    double csd = gist.complexSpectralDifference();
    
    return PyFloat_FromDouble (csd);
}

//=======================================================================
static PyObject * highFrequencyContent (PyObject *dummy, PyObject *args)
{
    double hfc = gist.highFrequencyContent();
    
    return PyFloat_FromDouble (hfc);
}

//=======================================================================//
//============================== PITCH ==================================//
//=======================================================================//

//=======================================================================
static PyObject * pitch (PyObject *dummy, PyObject *args)
{
    double p = gist.pitch();
    
    return PyFloat_FromDouble (p);
}

//=======================================================================//
//============================= SPECTRA =================================//
//=======================================================================//

//=======================================================================
static PyObject * magnitudeSpectrum (PyObject *dummy, PyObject *args)
{
    std::vector<double> magnitudeSpectrum = gist.getMagnitudeSpectrum();
    
    int numDimensions = 1;
    npy_intp numElements = magnitudeSpectrum.size();
    
    PyObject* c = PyArray_SimpleNew (numDimensions, &numElements, NPY_DOUBLE);
    void* arrayData = PyArray_DATA ( (PyArrayObject*)c);
    memcpy (arrayData, &magnitudeSpectrum[0], PyArray_ITEMSIZE((PyArrayObject*) c) * numElements);
    
    return c;
}

//=======================================================================
static PyObject * melFrequencySpectrum (PyObject *dummy, PyObject *args)
{
    const std::vector<double>& melFrequencySpectrum = gist.getMelFrequencySpectrum();
    
    int numDimensions = 1;
    npy_intp numElements = melFrequencySpectrum.size();
    
    PyObject* c = PyArray_SimpleNew (numDimensions, &numElements, NPY_DOUBLE);
    void* arrayData = PyArray_DATA ( (PyArrayObject*)c);
    memcpy (arrayData, &melFrequencySpectrum[0], PyArray_ITEMSIZE((PyArrayObject*) c) * numElements);
    
    return c;
}

//=======================================================================
static PyObject * mfccs (PyObject *dummy, PyObject *args)
{
    const std::vector<double>& mfccs = gist.getMelFrequencyCepstralCoefficients();
    
    int numDimensions = 1;
    npy_intp numElements = mfccs.size();
    
    PyObject* c = PyArray_SimpleNew (numDimensions, &numElements, NPY_DOUBLE);
    void* arrayData = PyArray_DATA ( (PyArrayObject*)c);
    memcpy (arrayData, &mfccs[0], PyArray_ITEMSIZE((PyArrayObject*) c) * numElements);
    
    return c;
}

//=======================================================================
static PyMethodDef gist_methods[] = {
    
    /** Configuration methods */
    {"setAudioFrameSize",           setAudioFrameSize,          METH_VARARGS,   "Set the audio frame size to be used"},
    {"getAudioFrameSize",           getAudioFrameSize,          METH_VARARGS,   "Get the audio frame size currently being used"},
    {"setSamplingFrequency",        setSamplingFrequency,       METH_VARARGS,   "Set the audio sampling frequency to be used"},
    {"getSamplingFrequency",        getSamplingFrequency,       METH_VARARGS,   "Get the audio sampling frequency currently being used"},
    
    {"processFrame",                processFrame,               METH_VARARGS,   "Process a single audio frame"},
    
    /** Core Time Domain Features */
    {"rms",                         rootMeanSquare,             METH_VARARGS,   "Return the RMS of the most recent audio frame"},
    {"peakEnergy",                  peakEnergy,                 METH_VARARGS,   "Return the peak energy of the most recent audio frame"},
    {"zeroCrossingRate",            zeroCrossingRate,           METH_VARARGS,   "Return the zero crossing rate of the most recent audio frame"},
    
    /** Core Frequency Domain Features */
    {"spectralCentroid",            spectralCentroid,           METH_VARARGS,   "Return the spectral centroid of the most recent audio frame"},
    {"spectralCrest",               spectralCrest,              METH_VARARGS,   "Return the spectral crest of the most recent audio frame"},
    {"spectralFlatness",            spectralFlatness,           METH_VARARGS,   "Return the spectral flatness of the most recent audio frame"},
    {"spectralRolloff",             spectralRolloff,            METH_VARARGS,   "Return the spectral rolloff of the most recent audio frame"},
    {"spectralKurtosis",            spectralKurtosis,           METH_VARARGS,   "Return the spectral kurtosis of the most recent audio frame"},
    
    /** Onset Detection Functions */
    {"energyDifference",            energyDifference,           METH_VARARGS,   "Return the energy difference onset detection function of the most recent audio frame"},
    {"spectralDifference",          spectralDifference,         METH_VARARGS,   "Return the spectral difference onset detection function of the most recent audio frame"},
    {"spectralDifferenceHWR",       spectralDifferenceHWR,      METH_VARARGS,   "Return the spectral difference (half-wave rectified) onset detection function of the most recent audio frame"},
    {"complexSpectralDifference",   complexSpectralDifference,  METH_VARARGS,   "Return the complex spectral difference onset detection function of the most recent audio frame"},
    {"highFrequencyContent",        highFrequencyContent,       METH_VARARGS,   "Return the high frequency content onset detection function of the most recent audio frame"},
    
    /** Pitch */
    {"pitch",                       pitch,                      METH_VARARGS,   "Return the monophonic pitch estimate the most recent audio frame"},
    
    /** Spectra */
    {"magnitudeSpectrum",           magnitudeSpectrum,          METH_VARARGS,   "Return the magnitude spectrum for the most recent audio frame"},
    {"melFrequencySpectrum",        melFrequencySpectrum,       METH_VARARGS,   "Return the mel-frequency spectrum for the most recent audio frame"},
    {"mfccs",                       mfccs,                      METH_VARARGS,   "Return the mel-frequency cepstral coefficients for the most recent audio frame"},
    
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static struct PyModuleDef gist_definition = {
    PyModuleDef_HEAD_INIT,
    "gist",
    "Python bindings for the Gist C++ based audio analysis library.",
    -1,
    gist_methods
};

//=======================================================================
PyMODINIT_FUNC PyInit_gist (void)
{
    import_array();
    return PyModule_Create(&gist_definition);
}

//=======================================================================
int main (int argc, char *argv[])
{
    //Convert char* to const wchar_t
    wchar_t *program = Py_DecodeLocale(argv[0], NULL);

    /* Pass argv[0] to the Python interpreter */
    Py_SetProgramName(program);
    
    /* Initialize the Python interpreter.  Required. */
    Py_Initialize();
    
    /* Add a static module */
    PyInit_gist();
}
