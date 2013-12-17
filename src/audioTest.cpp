/*
*  Copyright 2010,2011,2012 Reality Jockey, Ltd. and others.
*                 info@rjdj.me
*                 http://rjdj.me/
*
*  This file is part of ZenGarden.
*
*  ZenGarden is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Lesser General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  ZenGarden is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public License
*  along with ZenGarden.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#include <iostream>

#include "ZenGarden.h"
#include "PdGraph.h"

using namespace std;

#define BUFFER_SIZE 512
#define BUFFER_COUNT 2
static AudioQueueRef audioQueue;

PdContext *pdCtx;

extern "C" {
  void *callbackFunction(ZGCallbackFunction function, void *userData, void *ptr) {
    switch (function) {
      case ZG_PRINT_STD: printf("%s\n", (char *) ptr); break;
      case ZG_PRINT_ERR: printf("ERROR: %s\n", (char *) ptr); break;
      default: break;
    }
    return NULL;
  }
};

void timerDone(CFRunLoopTimerRef timer, void *info)
{
  cout << "Timer Done, stopping run loop." << endl;
  AudioQueueStop(audioQueue, true);
  zg_context_delete(pdCtx);
  
  CFRunLoopStop(CFRunLoopGetCurrent());
}

void audioCallback(void                 *aqData,
                   AudioQueueRef        outAQ,
                   AudioQueueBufferRef  outBuffer)
{
  SInt16* coreAudioBuffer = (SInt16*)outBuffer->mAudioData;
  
  // Specify how many bytes we're providing
  outBuffer->mAudioDataByteSize = BUFFER_SIZE * 4;
  zg_context_process_s(pdCtx, NULL, coreAudioBuffer);
  
  // Enqueue the buffer
  AudioQueueEnqueueBuffer( audioQueue, outBuffer, 0, NULL );
}

int setupAudioQueue()
{
  OSStatus err = noErr;
  
  // Setup the audio device.
  AudioStreamBasicDescription deviceFormat;
  deviceFormat.mSampleRate = 44100;
  deviceFormat.mFormatID = kAudioFormatLinearPCM;
  deviceFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger;
  deviceFormat.mBytesPerPacket = 4;
  deviceFormat.mFramesPerPacket = 1;
  deviceFormat.mBytesPerFrame = 4;
  deviceFormat.mChannelsPerFrame = 2;
  deviceFormat.mBitsPerChannel = 16;
  deviceFormat.mReserved = 0;
  
  // Create a new output AudioQueue for the device.
  err = AudioQueueNewOutput(&deviceFormat,
                            audioCallback,
                            NULL,
                            CFRunLoopGetCurrent(),
                            kCFRunLoopCommonModes,
                            0,
                            &audioQueue);
  
  // Allocate buffers for the audio
  UInt32 bufferSizeBytes = BUFFER_SIZE * deviceFormat.mBytesPerFrame;
  
  // Allocate buffers for the AudioQueue, and pre-fill them.
  for (int i = 0; i < BUFFER_COUNT; ++i) {
    AudioQueueBufferRef mBuffer;
    err = AudioQueueAllocateBuffer(audioQueue, bufferSizeBytes, &mBuffer);
    if (err != noErr) break;
    audioCallback(NULL, audioQueue, mBuffer);
  }
  
  if (err == noErr) err = AudioQueueStart(audioQueue, NULL);
  if (err == noErr) return 0;
  
  return 1;
}

int main(int argc, char * const argv[]) {
  const int blockSize = 512;
  const int numInputChannels = 0;
  const int numOutputChannels = 2;
  const float sampleRate = 44100.0f;
  
  // pass directory and filename of the patch to load
  pdCtx = zg_context_new(numInputChannels, numOutputChannels, blockSize, sampleRate,
                         callbackFunction, NULL);
  
  // create a graph manually
  PdGraph *graph = zg_context_new_empty_graph(pdCtx);
  ZGObject *objOsc = zg_graph_add_new_object(graph, "osc~ 440", 0.0f, 0.0f);
  ZGObject *objDac = zg_graph_add_new_object(graph, "dac~", 0.0f, 0.0f);
  zg_graph_add_connection(graph, objOsc, 0, objDac, 0);
  zg_graph_add_connection(graph, objOsc, 0, objDac, 1);
  // This is needed in order for manual graph construction to actually work.
  graph->computeDeepLocalDspProcessOrder();
  
  /*
  PdGraph *graph = zg_context_new_graph_from_file(pdCtx, "", "test.pd");
  if (graph == NULL) {
    zg_context_delete(pdCtx);
    return 1;
  }*/
  
  // attach the graph
  zg_graph_attach(graph);
  
  // Set up a timer that will fire after 5 seconds to stop the
  // audio processing.
  CFRunLoopTimerContext ctx;
  ctx.version = 0;
  //ctx.info = &aqData;
  ctx.retain = NULL;
  ctx.release = NULL;
  ctx.copyDescription = NULL;
  
  CFRunLoopTimerRef timer;
  timer = CFRunLoopTimerCreate(NULL,
                               CFAbsoluteTimeGetCurrent() + 20.0,
                               0,
                               0,
                               0,
                               timerDone,
                               &ctx);
  
  CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, kCFRunLoopCommonModes);
  setupAudioQueue();
  cout << "Starting run loop." << endl;
  
  // Invoke a run loop on the current thread to keep the app
  // running while we generate audio.
  CFRunLoopRun();

  
  return 0;
}