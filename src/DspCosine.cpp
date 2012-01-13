/*
 *  Copyright 2009,2010,2011 Reality Jockey, Ltd.
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
#include "DspCosine.h"
#include "PdGraph.h"

// initialise the static class variables
float *DspCosine::cos_table = NULL;
int DspCosine::refCount = 0;

MessageObject *DspCosine::newObject(PdMessage *initMessage, PdGraph *graph) {
  return new DspCosine(initMessage, graph);
}

DspCosine::DspCosine(PdMessage *initMessage, PdGraph *graph) : DspObject(0, 1, 0, 1, graph) {
  this->sampleRate = graph->getSampleRate();
  refCount++;
  if (cos_table == NULL) {
    int sampleRateInt = (int) sampleRate;
    cos_table = (float *) malloc((sampleRateInt + 1) * sizeof(float));
    for (int i = 0; i < sampleRateInt; i++) {
      cos_table[i] = cosf(2.0f * M_PI * ((float) i) / sampleRate);
    }
    cos_table[sampleRateInt] = cos_table[0];
  }
}

DspCosine::~DspCosine() {
  if (--refCount == 0) {
    free(cos_table);
    cos_table = NULL;
  }
}

const char *DspCosine::getObjectLabel() {
  return "cos~";
}

void DspCosine::processDsp() {
  // as no messages are received and there is only one inlet, processDsp does not need much of the
  // infrastructure provided by DspObject
  
  #if __APPLE__
  float twoPi = 2.0f*M_PI;
  vDSP_vsmul(dspBufferAtInlet[0], 1, &twoPi, dspBufferAtOutlet0, 1, blockSizeInt);
  vvcosf(dspBufferAtOutlet0, dspBufferAtOutlet0, &blockSizeInt);
  
//  float tempBuffer[blockSizeInt];
//  vDSP_vabs(dspBufferAtInlet0, 1, tempBuffer, 1, blockSizeInt); // abs(x)
//  vDSP_vfrac(tempBuffer, 1, tempBuffer, 1, blockSizeInt); // get the fractional part of x
//  vDSP_vsmul(tempBuffer, 1, &sampleRate, tempBuffer, 1, blockSizeInt); // * sampleRate
//  vDSP_vindex(cos_table, tempBuffer, 1, dspBufferAtOutlet0, 1, blockSizeInt); // perform a table lookup
  #else
  for (int i = 0; i < blockSizeInt; i++) {
    // works because cosine is symmetric about zero
    float f = fabsf(dspBufferAtInlet[0][i]);
    f -= floorf(f);
    dspBufferAtOutlet0[i] = cos_table[(int) (f * sampleRate)];
  }
  #endif
}
