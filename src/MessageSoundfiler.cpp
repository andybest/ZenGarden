/*
 *  Copyright 2009,2010 Reality Jockey, Ltd.
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

#include <sndfile.h>
#include "MessageSoundfiler.h"
#include "MessageTable.h"
#include "PdGraph.h"

MessageObject *MessageSoundfiler::newObject(PdMessage *initMessage, PdGraph *graph) {
  return new MessageSoundfiler(initMessage, graph);
}

MessageSoundfiler::MessageSoundfiler(PdMessage *initMessage, PdGraph *graph) : MessageObject(1, 1, graph) {
  // TODO(mhroth)
}

MessageSoundfiler::~MessageSoundfiler() {
  // nothing to do
}

const char *MessageSoundfiler::getObjectLabel() {
  return "soundfiler";
}

void MessageSoundfiler::processMessage(int inletIndex, PdMessage *message) {

  if (message->isSymbol(0, "read")) {
    int currentElementIndex = 1;
    bool shouldResizeTable = false;
    
    while (currentElementIndex < message->getNumElements()) {
      if (message->isSymbol(currentElementIndex)) {
        
        if(message->getSymbol(currentElementIndex)[0] == '-')
        {
          // Symbol is a flag
          if (message->isSymbol(currentElementIndex, "-resize")) {
            shouldResizeTable = true;
          }else{
            graph->printErr("[soundfiler]: unsupported flag %s", message->getSymbol(currentElementIndex));
          }
          currentElementIndex++;
          
        } else {
          if(message->getNumElements() < currentElementIndex + 2)
          {
            graph->printErr("[soundfiler]: not enough arguments.");
          }
          
          MessageAtom *fileAtom = message->getElement(currentElementIndex++);
          // all of the flags should have been seen now and now we expect the last two parameters,
          // which are file location and destination table name
          MessageAtom *tableNameElement = message->getElement(currentElementIndex++);
          if (tableNameElement != NULL) {
            MessageTable *table = graph->getTable(tableNameElement->symbol);
            if (table == NULL) {
              graph->printErr("[soundfiler]: table %s does not exist.", tableNameElement->symbol);
            }else{
              char *fullPath = graph->resolveFullPath(fileAtom->symbol);
              if (fullPath == NULL) {
                graph->printErr("[soundfiler]: file %s cannot be found.", fileAtom->symbol);
                return;
              }
              
              // use libsndfile to load and read the file (also converting the samples to [-1,1] float)
              SF_INFO sfInfo;
              
              SNDFILE *sndFile = sf_open(fullPath, SFM_READ, &sfInfo);
              if (sndFile == NULL) {
                graph->printErr("[soundfiler]: file %s cannot be opened.", fullPath);
                free(fullPath);
                return; // there was an error reading the file. Move on with life.
              } 
              free(fullPath);
              
              // It is assumed that the channels are interleaved.
              int samplesPerChannel = sfInfo.frames;
              int bufferLength = samplesPerChannel * sfInfo.channels;
              // create a buffer in memory for the file data
              float *buffer = (float *) malloc(bufferLength * sizeof(float));
              sf_read_float(sndFile, buffer, bufferLength); // read the whole file into memory
              sf_close(sndFile); // release the handle to the file
              
              if (sfInfo.channels > 0) { // sanity check
                // get the table's buffer. Resize the buffer if necessary.
                int tableLength = samplesPerChannel;
                float *tableBuffer = shouldResizeTable ? table->resizeBuffer(samplesPerChannel) :
                table->getBuffer(&tableLength);
                if (tableLength > samplesPerChannel) {
                  // avoid trying to read more into the table buffer than is available
                  tableLength = samplesPerChannel;
                }
                
                // extract the first channel
                for (int i = 0, j = 0; i < bufferLength; i+=sfInfo.channels, j++) {
                  tableBuffer[j] = buffer[i];
                }
                
                // extract the second channel (if it exists and if there is a table to write it to)
                if (sfInfo.channels > 1) {
                  tableLength = samplesPerChannel;
                  tableBuffer = shouldResizeTable ? table->resizeBuffer(samplesPerChannel) :
                  table->getBuffer(&tableLength);
                  if (tableLength > samplesPerChannel) {
                    // avoid trying to read more into the table buffer than is available
                    tableLength = samplesPerChannel;
                  }
                  for (int i = 1, j = 0; i < bufferLength; i+=sfInfo.channels, j++) {
                    tableBuffer[j] = buffer[i];
                  }
                }
              }
              free(buffer);
              
              // send message with sample length when all tables have been filled
              PdMessage *outgoingMessage = PD_MESSAGE_ON_STACK(0);
              outgoingMessage->initWithTimestampAndFloat(message->getTimestamp(), (float)samplesPerChannel);
              sendMessage(0, outgoingMessage);
            }
          }
        }
      }
    }
  } else if (message->isSymbol(0, "write")) {
    // TODO(mhroth): not supported yet
    graph->printErr("The \"write\" command to soundfiler is not supported.");
  }
}
