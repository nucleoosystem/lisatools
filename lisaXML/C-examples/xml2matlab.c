/* lisatools/lisaXML/C-examples/xml2matlab.c --- Copyright (c) 2006 Michele Vallisneri

   $Id$

   After making sure that mex is in you path (on OS X, something like
   /Applications/MATLAB72/bin), compile with:
   
   mex xml2matlab.c ../io-C/readxml.c ../io-C/ezxml.c -I..

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:
   
   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE. */

#include "mex.h" 
#include "io-C/readxml.h"

/* MATLAB syntax: array = xml2matlab('file.xml')

   This little MEX program will retrieve the first TDIObservable section in a lisaXML file
   as a MATLAB n x m array, where m is the number of observables on each column, and n
   is the number of samples. It will also print more information about the time series,
   such as the cadence, time offset, and name of the observables.

   It will not fail gracefully if anything goes wrong, but code to do that can be added
   easily. Code can also be added to return the time series information mentioned above.
   Please contact vallis@vallis.org if you have improvements to this code, or feature
   requests. */
 
static mxArray *parsefile(char *filename,int obsnum) {
    TimeSeries *timeseries;
    LISASource *firstsource = 0;
    
    mxArray *newmatrix;
    double *newdata;

    long i,j;

    mexPrintf("Opening file '%s'...\n",filename);

    timeseries = getmultipleTDIdata(filename,obsnum);

    if(timeseries == 0) {
        mexPrintf("...no TDI observables. Looking for first source...\n");

        firstsource = getLISASources(filename);

        if(firstsource == 0) {
            mexPrintf("...can't find it!\n");
            return 0;
        }

        timeseries = firstsource->TimeSeries;
    }

    mexPrintf("Reading Series '%s' from file '%s':\n",timeseries->Name,timeseries->FileName);
    mexPrintf("-> %d x %d values\n",timeseries->Length,timeseries->Records);
    mexPrintf("-> cadence = %g s, offset = %g s\n",timeseries->Cadence,timeseries->TimeOffset);

    mexPrintf("-> columns = ");
    for(i=0;i<timeseries->Records;i++) {
        mexPrintf("%s",timeseries->Data[i]->Name);
        if(i<timeseries->Records-1) {
            mexPrintf(", ");
        } else {
            mexPrintf("\n");
        }
    }

    newmatrix = mxCreateDoubleMatrix(timeseries->Length,timeseries->Records,mxREAL); 
    newdata = mxGetPr(newmatrix); 

    for(i=0;i<timeseries->Records;i++) {
        for(j=0;j<timeseries->Length;j++) {
            newdata[i*(timeseries->Length)+j] = timeseries->Data[i]->data[j];
        }
    }
        
    if(firstsource == 0) {
        freeTimeSeries(timeseries);
    } else {
        freeLISASources(firstsource);
    }
    
    return newmatrix;
}
 
void mexFunction( int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[] ) { 
    int buflen, obs;
    char *input_buf;
  
    if(nrhs != 1) { 
        mexErrMsgTxt("One input required."); 
    } /* else if(nlhs > 1) { 
        mexErrMsgTxt("Too many output arguments"); 
    } */
  
    if(mxIsChar(prhs[0]) != 1) 
        mexErrMsgTxt("Input must be a string.");
  
    if(mxGetM(prhs[0]) != 1) 
      mexErrMsgTxt("Input must be a row vector."); 

    buflen = (mxGetM(prhs[0]) * mxGetN(prhs[0])) + 1;
    input_buf = mxCalloc(buflen, sizeof(char)); 
    mxGetString(prhs[0], input_buf, buflen);

    for(obs=0; obs < nlhs; obs++)
        plhs[obs] = parsefile(input_buf,obs);
    
    return;
}
