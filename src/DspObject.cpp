/*
 *  Copyright 2009,2010,2011,2012 Reality Jockey, Ltd.
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

#include "ArrayArithmetic.h"
#include "DspImplicitAdd.h"
#include "DspObject.h"
#include "PdGraph.h"


#pragma mark - Constructor/Destructor

DspObject::DspObject(int numMessageInlets, int numDspInlets, int numMessageOutlets, int numDspOutlets, PdGraph *graph) :
    MessageObject(numMessageInlets, numMessageOutlets, graph) {
  init(numDspInlets, numDspOutlets, graph->getBlockSize());
}

DspObject::DspObject(int numMessageInlets, int numDspInlets, int numMessageOutlets, int numDspOutlets, int blockSize, PdGraph *graph) : 
    MessageObject(numMessageInlets, numMessageOutlets, graph) {
  init(numDspInlets, numDspOutlets, blockSize);
}

void DspObject::init(int numDspInlets, int numDspOutlets, int blockSize) {
  blockSizeInt = blockSize;
  codepath = DSP_OBJECT_PROCESS_NO_MESSAGE;
  processFunction = &processFunctionNoMessage;
  
  // initialise the incoming dsp connections list
  incomingDspConnections = vector<list<ObjectLetPair> >(numDspInlets);
  
  // initialise the outgoing dsp connections list
  outgoingDspConnections = vector<list<ObjectLetPair> >(numDspOutlets);

  dspBufferAtInlet = (numDspInlets > 0) ? (float **) calloc(numDspInlets, sizeof(float *)) : NULL;
  
  // initialise the local output audio buffers
  dspBufferAtOutlet = (numDspOutlets > 0) ? (float **) calloc(numDspOutlets, sizeof(float *)) : NULL;
}

DspObject::~DspObject() {  
  
  // delete any messages still pending
  clearMessageQueue();
  
  // inlet and outlet buffers are managed by the BufferPool
  
  // free the inlet buffer index
  free(dspBufferAtInlet);
  
  // free the outlet buffer index
  free(dspBufferAtOutlet);
}

const char *DspObject::getObjectLabel() {
  return "obj~";
}


#pragma mark -

ConnectionType DspObject::getConnectionType(int outletIndex) {
  return DSP;
}

float *DspObject::getDspBufferAtOutlet(int outletIndex) {
  // sanity check on outletIndex
  return (outletIndex < outgoingDspConnections.size())
      ? dspBufferAtOutlet[outletIndex] : NULL;
}

bool DspObject::mayReuseBuffer(DspObject *dspObject, unsigned int outletIndex) {
  return outgoingDspConnections[outletIndex].front().first == dspObject;
}

list<ObjectLetPair> DspObject::getIncomingConnections(unsigned int inletIndex) {
  list<ObjectLetPair> messageConnectionList = MessageObject::getIncomingConnections(inletIndex);
  list<ObjectLetPair> dspConnectionList = incomingDspConnections.empty()
      ? list<ObjectLetPair>() : incomingDspConnections[inletIndex];
  messageConnectionList.insert(messageConnectionList.end(), dspConnectionList.begin(), dspConnectionList.end());
  return messageConnectionList;
}

list<ObjectLetPair> DspObject::getIncomingDspConnections(unsigned int inletIndex) {
  return (inletIndex < incomingDspConnections.size()) ? incomingDspConnections[inletIndex] : list<ObjectLetPair>();
}

list<ObjectLetPair> DspObject::getOutgoingConnections(unsigned int outletIndex) {
  list<ObjectLetPair> messageConnectionList = MessageObject::getOutgoingConnections(outletIndex);
  list<ObjectLetPair> dspConnectionList = outgoingDspConnections.empty()
      ? list<ObjectLetPair>() : outgoingDspConnections[outletIndex];
  messageConnectionList.insert(messageConnectionList.end(), dspConnectionList.begin(), dspConnectionList.end());
  return messageConnectionList;
}

list<ObjectLetPair> DspObject::getOutgoingDspConnections(unsigned int outletIndex) {
  return outgoingDspConnections[outletIndex];
}


#pragma mark - Add/Remove Connections

void DspObject::addConnectionFromObjectToInlet(MessageObject *messageObject, int outletIndex, int inletIndex) {
  MessageObject::addConnectionFromObjectToInlet(messageObject, outletIndex, inletIndex);
  
  if (messageObject->getConnectionType(outletIndex) == DSP) {
    list<ObjectLetPair> *connections = &incomingDspConnections[inletIndex];
    ObjectLetPair objectLetPair = make_pair(messageObject, outletIndex);
    connections->push_back(objectLetPair);
  }
  
  onInletConnectionUpdate(inletIndex);
}

void DspObject::removeConnectionFromObjectToInlet(MessageObject *messageObject, int outletIndex, int inletIndex) {
  if (messageObject->getConnectionType(outletIndex) == DSP) {
    list<ObjectLetPair> *connections = &incomingDspConnections[inletIndex];
    ObjectLetPair objectLetPair = make_pair(messageObject, outletIndex);
    connections->remove(objectLetPair); // NOTE(mhroth): does this work?
  } else {
    MessageObject::removeConnectionFromObjectToInlet(messageObject, outletIndex, inletIndex);
  }
  
  onInletConnectionUpdate(inletIndex);
}

void DspObject::onInletConnectionUpdate(unsigned int inletIndex) {
  // nothing to do
}

void DspObject::addConnectionToObjectFromOutlet(MessageObject *messageObject, int inletIndex, int outletIndex) {
  MessageObject::addConnectionToObjectFromOutlet(messageObject, inletIndex, outletIndex);
  
  // TODO(mhroth): it is assumed here that the input connection type of the destination object is DSP. Correct?
  if (getConnectionType(outletIndex) == DSP) {
    list<ObjectLetPair> *connections = &outgoingDspConnections[outletIndex];
    ObjectLetPair objectLetPair = make_pair(messageObject, inletIndex);
    connections->push_back(objectLetPair);
  }
}

void DspObject::removeConnectionToObjectFromOutlet(MessageObject *messageObject, int inletIndex, int outletIndex) {
  if (getConnectionType(outletIndex) == MESSAGE) {
    MessageObject::removeConnectionToObjectFromOutlet(messageObject, inletIndex, outletIndex);
  } else {
    list<ObjectLetPair> *outgoingConnections = &outgoingDspConnections[outletIndex];
    ObjectLetPair objectLetPair = make_pair(messageObject, inletIndex);
    outgoingConnections->remove(objectLetPair);
  }
}

void DspObject::setDspBufferAtInlet(float *buffer, unsigned int inletIndex) {
  dspBufferAtInlet[inletIndex] = buffer;
  
  // if the object must take any special steps when updating the input buffer, do so now
  onDspBufferAtInletUpdate(buffer, inletIndex);
}

void DspObject::setDspBufferAtOutlet(float *buffer, unsigned int outletIndex) {
  dspBufferAtOutlet[outletIndex] = buffer;
  
  onDspBufferAtOutletUpdate(buffer, outletIndex);
}


#pragma mark -

void DspObject::clearMessageQueue() {
  while (!messageQueue.empty()) {
    MessageLetPair messageLetPair = messageQueue.front();
    PdMessage *message = messageLetPair.first;
    message->freeMessage();
    messageQueue.pop();
  }
}

void DspObject::receiveMessage(int inletIndex, PdMessage *message) {
  // Queue the message to be processed during the DSP round only if the graph is switched on.
  // Otherwise messages would begin to pile up because the graph is not processed.
  if (graph->isSwitchedOn()) {
    // Copy the message to the heap so that it is available to process later.
    // The message is released once it is consumed in processDsp().
    messageQueue.push(make_pair(message->copyToHeap(), inletIndex));
    codepath = DSP_OBJECT_PROCESS_MESSAGE;
    
    // only process the message if the process function is set to the default no-message function.
    // If it is set to anything else, then it is assumed that messages should not be processed.
    if (processFunction == &processFunctionNoMessage) processFunction = &processFunctionMessage;
  }
}


#pragma mark - processDsp

void DspObject::processDsp() {
  switch (codepath) {
    case DSP_OBJECT_PROCESS_MESSAGE: {
      double blockIndexOfLastMessage = 0.0; // reset the block index of the last received message
      do { // there is at least one message
        MessageLetPair messageLetPair = messageQueue.front();
        PdMessage *message = messageLetPair.first;
        unsigned int inletIndex = messageLetPair.second;
        
        double blockIndexOfCurrentMessage = graph->getBlockIndex(message);
        processDspWithIndex(blockIndexOfLastMessage, blockIndexOfCurrentMessage);
        processMessage(inletIndex, message);
        message->freeMessage(); // free the message from the head, the message has been consumed.
        messageQueue.pop();
        
        blockIndexOfLastMessage = blockIndexOfCurrentMessage;
      } while (!messageQueue.empty());
      processDspWithIndex(blockIndexOfLastMessage, (double) blockSizeInt);
      codepath = DSP_OBJECT_PROCESS_NO_MESSAGE;
      break;
    }
    default:
    case DSP_OBJECT_PROCESS_NO_MESSAGE: {
      processDspWithIndex(0, blockSizeInt);
      break;
    }
  }
}

void DspObject::processFunctionNoMessage(DspObject *dspObject) {
  dspObject->processDspWithIndex(0, dspObject->blockSizeInt);
}

void DspObject::processFunctionMessage(DspObject *dspObject) {
  double blockIndexOfLastMessage = 0.0; // reset the block index of the last received message
  do { // there is at least one message
    MessageLetPair messageLetPair = dspObject->messageQueue.front();
    PdMessage *message = messageLetPair.first;
    unsigned int inletIndex = messageLetPair.second;
    
    double blockIndexOfCurrentMessage = dspObject->graph->getBlockIndex(message);
    dspObject->processDspWithIndex(blockIndexOfLastMessage, blockIndexOfCurrentMessage);
    dspObject->processMessage(inletIndex, message);
    message->freeMessage(); // free the message from the head, the message has been consumed.
    dspObject->messageQueue.pop();
    
    blockIndexOfLastMessage = blockIndexOfCurrentMessage;
  } while (!dspObject->messageQueue.empty());
  dspObject->processDspWithIndex(blockIndexOfLastMessage, (double) dspObject->blockSizeInt);
  
  // because messages are received much less often than on a per-block basis, one messages are
  // processed in this block, return to the default process function which assumes that no messages
  // are present. This improved performance because the messageQueue must not be checked for
  // any pending messages. It is assumed that there aren't any.
  dspObject->processFunction = &processFunctionNoMessage;
}

void DspObject::processDspWithIndex(double fromIndex, double toIndex) {
  // by default, this function just calls the integer version with adjusted block indicies
  processDspWithIndex((int) ceil(fromIndex), (int) ceil(toIndex));
}

void DspObject::processDspWithIndex(int fromIndex, int toIndex) {
  // by default, this function just calls the float version
  processDspWithIndex((float) fromIndex, (float) toIndex);
}


#pragma mark -

bool DspObject::isLeafNode() {
  if (!MessageObject::isLeafNode()) return false;
  else {
    for (int i = 0; i < outgoingDspConnections.size(); i++) {
      if (!outgoingDspConnections[i].empty()) return false;
    }
    return true;
  }
}

list<DspObject *> DspObject::getProcessOrder() {
  if (isOrdered) {
    // if this object has already been ordered, then move on
    return list<DspObject *>();
  } else {
    isOrdered = true;
    list<DspObject *> processList;
    for (int i = 0; i < incomingMessageConnections.size(); i++) {
      for (list<ObjectLetPair>::iterator it = incomingMessageConnections[i].begin();
          it != incomingMessageConnections[i].end(); it++) {
        ObjectLetPair objectLetPair = *it++;
        list<DspObject *> parentProcessList = objectLetPair.first->getProcessOrder();
        // DspObjects "connected" through message connections must be processed, but buffers
        // not otherwise calculated
        processList.splice(processList.end(), parentProcessList);
      }
    }
    
    BufferPool *bufferPool = graph->getBufferPool();
    PdMessage *dspAddInitMessage = PD_MESSAGE_ON_STACK(1);
    dspAddInitMessage->initWithTimestampAndFloat(0, 0.0f);
    for (int i = 0; i < incomingDspConnections.size(); i++) {
      switch (incomingDspConnections[i].size()) {
        case 0: {
          setDspBufferAtInlet(bufferPool->getZeroBuffer(), i);
          break;
        }
        case 1: {
          ObjectLetPair objectLetPair = incomingDspConnections[i].front();
          list<DspObject *> parentProcessList = objectLetPair.first->getProcessOrder();
          
          // configure the input buffers
          DspObject *dspObject = reinterpret_cast<DspObject *>(objectLetPair.first);
          float *buffer = dspObject->getDspBufferAtOutlet(objectLetPair.second);
          setDspBufferAtInlet(buffer, i);
          bufferPool->releaseBuffer(buffer);
          
          // conbine the process lists
          processList.splice(processList.end(), parentProcessList);
          break;
        }
        default: { // > 1
          list<ObjectLetPair>::iterator it = incomingDspConnections[i].begin();
          ObjectLetPair leftOlPair = *it++;
          list<DspObject *> parentProcessList = leftOlPair.first->getProcessOrder();
          processList.splice(processList.end(), parentProcessList);
          
          while (it != incomingDspConnections[i].end()) {
            ObjectLetPair objectLetPair = *it++;
            list<DspObject *> parentProcessList = objectLetPair.first->getProcessOrder();
            processList.splice(processList.end(), parentProcessList);
            
            DspImplicitAdd *dspAdd = new DspImplicitAdd(dspAddInitMessage, getGraph());
            float *buffer = reinterpret_cast<DspObject *>(leftOlPair.first)->getDspBufferAtOutlet(leftOlPair.second);
            dspAdd->setDspBufferAtInlet(buffer, 0);
            bufferPool->releaseBuffer(buffer);
            
            buffer = reinterpret_cast<DspObject *>(objectLetPair.first)->getDspBufferAtOutlet(objectLetPair.second);
            dspAdd->setDspBufferAtInlet(buffer, 1);
            bufferPool->releaseBuffer(buffer);
            
            // assign the output buffer of the +~~
            dspAdd->setDspBufferAtOutlet(bufferPool->getBuffer(1), 0);
            
            processList.push_back(dspAdd);
            leftOlPair = objectLetPair;
          }
          
          float *buffer = reinterpret_cast<DspObject *>(leftOlPair.first)->getDspBufferAtOutlet(leftOlPair.second);
          setDspBufferAtInlet(buffer, i);
          bufferPool->releaseBuffer(buffer);
          break;
        }
      }
    }
    
    for (int i = 0; i < getNumDspOutlets(); i++) {
      float *buffer = bufferPool->getBuffer(outgoingDspConnections[i].size());
      setDspBufferAtOutlet(buffer, i);
    }
    
    // NOTE(mhroth): even if an object does not process audio, its buffer still need to be connected.
    // They may be passed on to other objects, such as s~/r~ pairs
    if (doesProcessAudio()) processList.push_back(this);
    return processList;
  }
}
