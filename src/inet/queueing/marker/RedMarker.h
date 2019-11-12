//
// Copyright (C) 2019 Marcel Marek
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __INET_REDMARKER_H_
#define __INET_REDMARKER_H_

#include "inet/common/packet/Packet.h"
#include "inet/queueing/filter/RedDropper.h"

namespace inet {
namespace queueing {

class INET_API RedMarker : public RedDropper
{
  public:
    RedMarker();

  protected:
    double mark;
    bool markNext;

  private:
    int packetCapacity;

  protected:

    enum Action { MARK, DROP, SEND };
    virtual ~RedMarker();
    virtual void initialize(int stage) override;
    Action chooseAction(Packet *packet);
    virtual bool matchesPacket(Packet *packet) override;
    void markPacket(Packet *packet);
};

} // namespace queueing
} // namespace inet

#endif

