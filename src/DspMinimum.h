/*
 *  Copyright 2010,2011,2012 Reality Jockey, Ltd.
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

#ifndef _DSP_MINIMUM_H_
#define _DSP_MINIMUM_H_

#include "DspObject.h"

enum DspMinimumCodePath {
  DSP_MINIMUM_DSP_DSP,
  DSP_MINIMUM_DSP_MESSAGE,
};

/** [min~ float] */
class DspMinimum : public DspObject {
  
  public:
    static MessageObject *newObject(PdMessage *initMessage, PdGraph *graph);
    DspMinimum(PdMessage *initMessage, PdGraph *graph);
    ~DspMinimum();
    
    static const char *getObjectLabel();
    string toString();
    
  private:
    static void processSignal(DspObject *dspObject);
    void processMessage(int inletIndex, PdMessage *message);
    void processDspWithIndex(int fromIndex, int toIndex);
    
    float constant;
};

#endif // _DSP_MINIMUM_H_
