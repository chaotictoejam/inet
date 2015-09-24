//
// Copyright (C) 2013 OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifdef WITH_OSG

#ifndef __INET_MOBILITYOSGVISUALIZER_H
#define __INET_MOBILITYOSGVISUALIZER_H

#include "inet/mobility/base/MobilityVisualizerBase.h"
#include "inet/mobility/contract/IMobility.h"

namespace inet {

class INET_API MobilityOsgVisualizer : public MobilityVisualizerBase
{
  protected:
    class INET_API CacheEntry {
      public:
        osg::PositionAttitudeTransform *positionAttitudeTransform;
        osg::Geode *trail;

      public:
        CacheEntry(osg::PositionAttitudeTransform *positionAttitudeTransform, osg::Geode *trail);
    };

  protected:
    std::map<const IMobility *, CacheEntry *> cacheEntries;

  protected:
    virtual CacheEntry *getCacheEntry(const IMobility *mobility) const;
    virtual void setCacheEntry(const IMobility *mobility, CacheEntry *entry);
    virtual void removeCacheEntry(const IMobility *mobility);
    virtual CacheEntry* ensureCacheEntry(const IMobility *mobility);

    virtual void extendMovementTrail(osg::Geode *trail, const Coord& position);

  public:
    virtual void receiveSignal(cComponent *source, simsignal_t signal, cObject *object) override;

    static osg::PositionAttitudeTransform *createOsgNode(const IMobility *mobility);
    static void setPosition(osg::PositionAttitudeTransform *positionAttitudeTransform, const Coord& position);
};

} // namespace inet

#endif // ifndef __INET_MOBILITYOSGVISUALIZER_H

#endif // WITH_OSG

