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

#ifndef __INET_MOBILITYCANVASVISUALIZER_H
#define __INET_MOBILITYCANVASVISUALIZER_H

#include "inet/common/figures/TrailFigure.h"
#include "inet/common/geometry/common/CanvasProjection.h"
#include "inet/mobility/base/MobilityVisualizerBase.h"

namespace inet {

class INET_API MobilityCanvasVisualizer : public MobilityVisualizerBase
{
  protected:
    class INET_API CacheEntry {
      public:
        cModule *visualRepresentation = nullptr;
        cImageFigure *mainFigure = nullptr;
        TrailFigure *trailFigure = nullptr;

      public:
        CacheEntry(cModule *visualRepresentation, cImageFigure *mainFigure, TrailFigure *trailFigure);
    };

  protected:
    const CanvasProjection *canvasProjection = nullptr;

    std::map<const IMobility *, CacheEntry *> cacheEntries;

  protected:
    virtual void initialize(int stage) override;

    virtual cModule *findVisualRepresentation(cModule *module) const;

    virtual CacheEntry *getCacheEntry(const IMobility *mobility) const;
    virtual void setCacheEntry(const IMobility *mobility, CacheEntry *entry);
    virtual void removeCacheEntry(const IMobility *mobility);
    virtual CacheEntry* ensureCacheEntry(const IMobility *mobility);
    virtual void extendMovementTrail(TrailFigure *trailFigure, cFigure::Point position);

  public:
    virtual ~MobilityCanvasVisualizer();

    virtual void receiveSignal(cComponent *source, simsignal_t signal, cObject *object) override;

    static void setPosition(cModule* visualRepresentation, cFigure::Point position);
};

} // namespace inet

#endif // ifndef __INET_MOBILITYCANVASVISUALIZER_H

