/*
 *Audiolyzer - A pin tool for giving a sound to sampled program synchronization
 *             events in arbitrary multithreaded programs.  
 *
 *Copyright Brandon Lucia 2010
 *blucia@gmail.com
 *****************************
 *
 *This program is intended to be build as a shared library (.so), and used
 *with the Pin (http://pintool.org) binary instrumentation infrastructure.
 *Audiolyzer also relies on the audiere sound library (audiere.sourceforge.net).
 *
 */

#include "pin.H"

#include <signal.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <sys/time.h>
#include "audiere.h"
#include "tones.h"


using namespace audiere;
using namespace std;

#define MAX_NTHREADS 256
#define NUM_FREQS 31 

unsigned int sampleCount;
unsigned long SAMPLERATE;
unsigned long PLAYDELAY;

static  TLS_KEY tls_key;

PIN_LOCK AllSharedDataLock;
AudioDevicePtr device;

float frequencies[] = {A3, B3, C4, D4, E4, F4, G4, A4, B4, C5, D5, E5, F5, G5, A5, B5, C6, D6, E6, F6, G6, A6, B6, C7, D7, E7, F7, G7, A7, B7, C8}; //c major scale

//// function to access thread-specific data
OutputStreamPtr *get_tls(THREADID thread_id)
{
  OutputStreamPtr *tdata =
    static_cast<OutputStreamPtr *>(PIN_GetThreadData(tls_key, thread_id));
  return tdata;

}



UINT32 stackAddrMask = 0x80000000;
#define IS_STACKREF(addr) ((addr & stackAddrMask) == stackAddrMask)

bool instrumentationStatus[MAX_NTHREADS];

enum MemOpType { MemRead = 0, MemWrite = 1 };

INT32 usage()
{
    cerr << "Audiolyzer - A program for generating interesting sequences of tones from concurrent programs.  (Plays tones on synchronization events)\n";

    cerr << KNOB_BASE::StringKnobSummary();
    cerr << endl;
    return -1;
}


VOID TurnInstrumentationOn(ADDRINT tid){
  instrumentationStatus[PIN_ThreadId()] = true; 
}

VOID TurnInstrumentationOff(ADDRINT tid){
  instrumentationStatus[PIN_ThreadId()] = false; 
}


VOID HandleLock(THREADID tid){

  if(sampleCount < SAMPLERATE){
    sampleCount++; 
    return;
  }

  OutputStreamPtr *tone = get_tls(tid);
  GetLock(&(AllSharedDataLock), 1);
  (*tone)->play();
  usleep(PLAYDELAY);
  (*tone)->stop();
  ReleaseLock(&(AllSharedDataLock));
  return;

}

VOID SetSampleRate(unsigned long rate){
  fprintf(stderr,"setting sample rate to %lu\n",rate);
  SAMPLERATE = rate;

}

VOID instrumentRoutine(RTN rtn, VOID *v){
 
  if(RTN_Valid(rtn)){

    if(strstr(RTN_Name(rtn).c_str(),"pthread_mutex_lock")){
    RTN_Open(rtn);
    RTN_InsertCall(rtn, 
                   IPOINT_AFTER, 
                   (AFUNPTR)HandleLock, 
                   IARG_THREAD_ID,
                   IARG_END);
    RTN_Close(rtn);
    } 
    
    if(strstr(RTN_Name(rtn).c_str(),"SET_SD_SAMPLE_RATE")){
    RTN_Open(rtn);
    RTN_InsertCall(rtn, 
                   IPOINT_BEFORE, 
                   (AFUNPTR)SetSampleRate, 
                   IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                   IARG_END);
    RTN_Close(rtn);
    } 
    

  }
}

VOID instrumentImage(IMG img, VOID *v)
{

}


VOID instrumentTrace(TRACE trace, VOID *v){
}

VOID threadBegin(UINT32 tid, VOID * sp, int flags, VOID *v)
{
  OutputStreamPtr *tone = (OutputStreamPtr*)malloc(sizeof(OutputStreamPtr));
  *tone = NULL;
  fprintf(stderr,"Creating a tonegen for thread %d\n",tid);
  int i = rand();
  *tone = OpenSound(device,CreateTone(frequencies[tid % NUM_FREQS]));
  if(*tone){
    fprintf(stderr,"Created a tonegen for thread %d\n",tid);
  }else{
    fprintf(stderr,"Failed to Create a tonegen for thread %d\n",tid);
  }
  PIN_SetThreadData(tls_key, tone, tid);


}
    
VOID threadEnd(UINT32 threadid, INT32 code, VOID *v)
{
}

VOID dumpInfo(){
}

VOID Fini(INT32 code, VOID *v)
{

}

BOOL segvHandler(THREADID threadid,INT32 sig,CONTEXT *ctx,BOOL hasHndlr,VOID*v){
  return TRUE;//let the program's handler run too
}

BOOL termHandler(THREADID threadid,INT32 sig,CONTEXT *ctx,BOOL hasHndlr,VOID*v){
  return TRUE;//let the program's handler run too
}


int main(int argc, char *argv[])
{
  srand(time(0));
  PIN_InitSymbols();
  if( PIN_Init(argc,argv) ) {
    return usage();
  }

  InitLock(&AllSharedDataLock);
  SAMPLERATE = 100;
  PLAYDELAY = 100000;

  const char* device_name = "";
  device = OpenDevice(device_name);
  if(device){
    fprintf(stderr,"Created a device\n");
  }else{
    fprintf(stderr,"Failed Creating a device\n");
  }

  int tid = 0;
  OutputStreamPtr *tone = (OutputStreamPtr*)malloc(sizeof(OutputStreamPtr));
  *tone = NULL;
  fprintf(stderr,"Creating a tonegen for thread %d\n",tid);
  *tone = OpenSound(device,CreateTone(frequencies[0]));
  if(*tone){
    fprintf(stderr,"Created a tonegen for thread %d\n",tid);
  }else{
    fprintf(stderr,"Failed to Create a tonegen for thread %d\n",tid);
  }
  PIN_SetThreadData(tls_key, tone, tid);


  RTN_AddInstrumentFunction(instrumentRoutine,0);
  TRACE_AddInstrumentFunction(instrumentTrace, 0);

  PIN_AddSignalInterceptFunction(SIGTERM,termHandler,0);
  PIN_AddSignalInterceptFunction(SIGSEGV,segvHandler,0);

  PIN_AddThreadBeginFunction(threadBegin, 0);
  PIN_AddThreadEndFunction(threadEnd, 0);
	
  PIN_AddFiniFunction(Fini, 0);
 
  PIN_StartProgram();
  
  return 0;
}
