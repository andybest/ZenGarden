/*
 *  Copyright 2009, 2010 Reality Jockey, Ltd.
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

#include "MessageSqrt.h"

MessageSqrt::MessageSqrt(PdMessage *initMessage, PdGraph *graph) : MessageObject(1, 1, graph) {
  // nothing to do
}


MessageSqrt::MessageSqrt(PdGraph *graph) : MessageObject(1, 1, graph) {
  // nothing to do
}

MessageSqrt::~MessageSqrt() {
  // nothing to do
}

const char *MessageSqrt::getObjectLabel() {
  return "sqrt";
}

void MessageSqrt::processMessage(int inletIndex, PdMessage *message) {
  if (inletIndex == 0) {
    MessageElement *messageElement = message->getElement(0);
    if (messageElement->getType() == FLOAT) {
      PdMessage *outgoingMessage = getNextOutgoingMessage(0);
      outgoingMessage->getElement(0)->setFloat(sqrtf(messageElement->getFloat()));
      outgoingMessage->setTimestamp(message->getTimestamp());
      sendMessage(0, outgoingMessage); // send a message from outlet 0
    }
  }
}

PdMessage *MessageSqrt::newCanonicalMessage(int outletIndex) {
  PdMessage *message = new PdMessage();
  message->addElement(new MessageElement(0.0f));
  return message;
}
