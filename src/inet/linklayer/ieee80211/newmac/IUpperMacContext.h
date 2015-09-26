//
// Copyright (C) 2015 Andras Varga
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

#ifndef __INET_IUPPERMACCONTEXT_H
#define __INET_IUPPERMACCONTEXT_H

#include "inet/common/INETDefs.h"
#include "inet/linklayer/common/MACAddress.h"

namespace inet {
namespace ieee80211 {

class Ieee80211Frame;
class Ieee80211DataOrMgmtFrame;
class Ieee80211ACKFrame;
class Ieee80211RTSFrame;
class Ieee80211CTSFrame;
class ITxCallback;

class IUpperMacContext
{
    public:
        IUpperMacContext() {}
        virtual ~IUpperMacContext() {}

        virtual const MACAddress& getAddress() const = 0;
        virtual int getNumAccessCategories() const = 0;

        virtual simtime_t getSlotTime() const = 0;
        virtual simtime_t getAifsTime(int accessCategory) const = 0;
        virtual simtime_t getSifsTime() const = 0;
        virtual simtime_t getDifsTime() const = 0;
        virtual simtime_t getEifsTime(int accessCategory) const = 0;
        virtual simtime_t getPifsTime() const = 0;
        virtual simtime_t getRifsTime() const = 0;

        virtual int getCwMin(int accessCategory) const = 0;
        virtual int getCwMax(int accessCategory) const = 0;
        virtual int getCwMulticast(int accessCategory) const = 0;
        virtual int getShortRetryLimit() const = 0;
        virtual int getRtsThreshold() const = 0;
        virtual simtime_t getTxopLimit(int accessCategory) const = 0;

        virtual simtime_t getAckTimeout() const = 0;
        virtual simtime_t getAckDuration() const = 0;
        virtual simtime_t getCtsTimeout() const = 0;
        virtual simtime_t getCtsDuration() const = 0;

        virtual Ieee80211RTSFrame *buildRtsFrame(Ieee80211DataOrMgmtFrame *frame) const = 0;
        virtual Ieee80211CTSFrame *buildCtsFrame(Ieee80211RTSFrame *frame) const = 0;
        virtual Ieee80211ACKFrame *buildAckFrame(Ieee80211DataOrMgmtFrame *frameToACK) const = 0;
        virtual Ieee80211DataOrMgmtFrame *buildBroadcastFrame(Ieee80211DataOrMgmtFrame *frameToSend) const = 0;

        virtual double computeFrameDuration(int bits, double bitrate) const = 0;
        virtual Ieee80211Frame *setBasicBitrate(Ieee80211Frame *frame) const = 0;
        virtual Ieee80211Frame *setDataBitrate(Ieee80211Frame *frame) const = 0;
        virtual Ieee80211Frame *setControlBitrate(Ieee80211Frame *frame) const = 0;

        virtual bool isForUs(Ieee80211Frame *frame) const = 0;
        virtual bool isMulticast(Ieee80211Frame *frame) const = 0;
        virtual bool isBroadcast(Ieee80211Frame *frame) const = 0;
        virtual bool isCts(Ieee80211Frame *frame) const = 0;
        virtual bool isAck(Ieee80211Frame *frame) const = 0;

        virtual void transmitContentionFrame(int txIndex, Ieee80211Frame *frame, simtime_t ifs, simtime_t eifs, int cwMin, int cwMax, simtime_t slotTime, int retryCount, ITxCallback *completionCallback) const = 0;
        virtual void transmitImmediateFrame(Ieee80211Frame *frame, simtime_t ifs, ITxCallback *completionCallback) const = 0;
};

} // namespace ieee80211
} // namespace inet

#endif

