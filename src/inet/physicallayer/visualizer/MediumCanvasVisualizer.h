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

#ifndef __INET_MEDIUMCANVASVISUALIZER_H
#define __INET_MEDIUMCANVASVISUALIZER_H

#include "inet/common/figures/HeatMapFigure.h"
#include "inet/common/figures/TrailFigure.h"
#include "inet/common/geometry/common/CanvasProjection.h"
#include "inet/common/visualizer/VisualizerBase.h"
#include "inet/physicallayer/base/packetlevel/PhysicalLayerDefs.h"
#include "inet/physicallayer/base/packetlevel/MediumVisualizerBase.h"
#include "inet/physicallayer/contract/packetlevel/IRadioFrame.h"
#include "inet/physicallayer/contract/packetlevel/IReceptionDecision.h"
#include "inet/physicallayer/contract/packetlevel/ITransmission.h"

namespace inet {

namespace physicallayer {

/**
 * This class provides the visualization of the communication on the radio medium.
 */
class INET_API MediumCanvasVisualizer : public MediumVisualizerBase
{
  protected:
    /** @name Parameters */
    //@{
    /**
     * The 2D projection used on the canvas.
     */
    const CanvasProjection *canvasProjection = nullptr;
    /**
     * Determines whether the visualizer displays the ongoing communications or not.
     */
    bool displayCommunication = false;
    /**
     * Displays a circle around the host submodule representing the communication range.
     */
    bool displayCommunicationRange = false;
    /**
     * Displays a circle around the host submodule representing the interference range.
     */
    bool displayInterferenceRange = false;
    /**
     * Determines how the ongoing communications are visualized: 3D spheres or
     * 2D circles on the X-Y plane.
     */
    bool drawCommunication2D = false;
    /**
     * Specifies whether successful communication between radios leave a trail or not.
     */
    bool leaveCommunicationTrail = false;
#if OMNETPP_CANVAS_VERSION >= 0x20140908
    /**
     * Specifies whether successful communication between radios leave a heat map or not.
     */
    bool leaveCommunicationHeat = false;
#endif
    /**
     * Specifies the size of the communication heat map.
     */
    int communicationHeatMapSize = 100;
    //@}

    /** @name State */
    //@{
    /**
     * The list of ongoing transmissions.
     */
    std::vector<const ITransmission *> transmissions;
    /**
     * The list of ongoing transmission figures.
     */
    std::map<const ITransmission *, cFigure *> transmissionFigures;
    //@}

    /** @name Timer */
    //@{
    /**
     * The message that is used to update the canvas when ongoing communications exist.
     */
    cMessage *updateCanvasTimer = nullptr;
    //@}

    /** @name Figures */
    //@{
    /**
     * The layer figure that contains the figures representing the ongoing communications.
     */
    cGroupFigure *communicationLayer = nullptr;
    /**
     * The trail figure that contains figures representing the recent successful communications.
     */
    TrailFigure *communicationTrail = nullptr;
#if OMNETPP_CANVAS_VERSION >= 0x20140908
    /**
     * The heat map figure that shows the recent successful communications.
     */
    HeatMapFigure *communicationHeat = nullptr;
#endif
    //@}

  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *message) override;

    virtual cFigure *getCachedFigure(const ITransmission *transmission) const;
    virtual void setCachedFigure(const ITransmission *transmission, cFigure *figure);
    virtual void removeCachedFigure(const ITransmission *transmission);

    virtual void updateCanvas() const;
    virtual void scheduleUpdateCanvasTimer();

  public:
    virtual ~MediumCanvasVisualizer();

    virtual void mediumChanged() override;
    virtual void radioAdded(const IRadio *radio) override;
    virtual void radioRemoved(const IRadio *radio) override;
    virtual void transmissionAdded(const ITransmission *transmission) override;
    virtual void transmissionRemoved(const ITransmission *transmission) override;
    virtual void packetReceived(const IReceptionDecision *decision) override;

    static void setInterferenceRange(const IRadio *radio);
    static void setCommunicationRange(const IRadio *radio);
};

} // namespace physicallayer

} // namespace inet

#endif // ifndef __INET_MEDIUMCANVASVISUALIZER_H

