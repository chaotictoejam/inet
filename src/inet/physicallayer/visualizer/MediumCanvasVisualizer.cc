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

#include "inet/common/ModuleAccess.h"
#include "inet/environment/visualizer/PhysicalEnvironmentCanvasVisualizer.h"
#include "inet/physicallayer/common/packetlevel/RadioMedium.h"
#include "inet/physicallayer/visualizer/MediumCanvasVisualizer.h"

namespace inet {

namespace physicallayer {

Define_Module(MediumCanvasVisualizer);

MediumCanvasVisualizer::~MediumCanvasVisualizer()
{
    cancelAndDelete(updateCanvasTimer);
}

void MediumCanvasVisualizer::initialize(int stage)
{
    MediumVisualizerBase::initialize(stage);
    if (!hasGUI()) return;
    if (stage == INITSTAGE_LOCAL) {
        displayCommunication = par("displayCommunication");
        displayInterferenceRange = par("displayInterferenceRange");
        displayCommunicationRange = par("displayCommunicationRange");
        drawCommunication2D = par("drawCommunication2D");
        cCanvas *canvas = visualizerTargetModule->getCanvas();
        if (displayCommunication) {
            communicationLayer = new cGroupFigure("communication");
            canvas->addFigureBelow(communicationLayer, canvas->getSubmodulesLayer());
        }
        leaveCommunicationTrail = par("leaveCommunicationTrail");
        if (leaveCommunicationTrail) {
            communicationTrail = new TrailFigure(100, true, "communication trail");
            canvas->addFigureBelow(communicationTrail, canvas->getSubmodulesLayer());
        }
#if OMNETPP_CANVAS_VERSION >= 0x20140908
        leaveCommunicationHeat = par("leaveCommunicationHeat");
        if (leaveCommunicationHeat) {
            communicationHeat = new HeatMapFigure(communicationHeatMapSize, "communication heat");
            communicationHeat->setTags("successful_reception heat");
            canvas->addFigure(communicationHeat, 0);
        }
#endif
        updateInterval = par("updateCanvasInterval");
        updateCanvasTimer = new cMessage("updateCanvas");
    }
    else if (stage == INITSTAGE_LAST) {
        canvasProjection = CanvasProjection::getCanvasProjection(visualizerTargetModule->getCanvas());
#if OMNETPP_CANVAS_VERSION >= 0x20140908
        if (communicationHeat != nullptr) {
            const IMediumLimitCache *mediumLimitCache = radioMedium->getMediumLimitCache();
            Coord min = mediumLimitCache->getMinConstraintArea();
            Coord max = mediumLimitCache->getMaxConstraintArea();
            cFigure::Point o = canvasProjection->computeCanvasPoint(Coord::ZERO);
            cFigure::Point x = canvasProjection->computeCanvasPoint(Coord(1, 0, 0));
            cFigure::Point y = canvasProjection->computeCanvasPoint(Coord(0, 1, 0));
            double t1 = o.x;
            double t2 = o.y;
            double a = x.x - t1;
            double b = x.y - t2;
            double c = y.x - t1;
            double d = y.y - t2;
            communicationHeat->setTransform(cFigure::Transform(a, b, c, d, t1, t2));
            communicationHeat->setPosition(cFigure::Point((min.x + max.x) / 2, (min.y + max.y) / 2));
            communicationHeat->setWidth(max.x - min.x);
            communicationHeat->setHeight(max.y - min.y);
        }
#endif
    }
}

void MediumCanvasVisualizer::handleMessage(cMessage *message)
{
    if (message == updateCanvasTimer) {
        updateCanvas();
        scheduleUpdateCanvasTimer();
    }
    else
        throw cRuntimeError("Unknown message");
}

cFigure *MediumCanvasVisualizer::getCachedFigure(const ITransmission *transmission) const
{
    auto it = transmissionFigures.find(transmission);
    if (it == transmissionFigures.end())
        return nullptr;
    else
        return it->second;
}

void MediumCanvasVisualizer::setCachedFigure(const ITransmission *transmission, cFigure *figure)
{
    transmissionFigures[transmission] = figure;
}

void MediumCanvasVisualizer::removeCachedFigure(const ITransmission *transmission)
{
    transmissionFigures.erase(transmission);
}

void MediumCanvasVisualizer::mediumChanged()
{
    Enter_Method_Silent();
    if (displayCommunication)
        updateCanvas();
}

void MediumCanvasVisualizer::radioAdded(const IRadio *radio)
{
    Enter_Method_Silent();
    if (!hasGUI()) return;
    if (displayInterferenceRange)
        setInterferenceRange(radio);
    if (displayCommunicationRange)
        setCommunicationRange(radio);
}

void MediumCanvasVisualizer::radioRemoved(const IRadio *radio)
{
    Enter_Method_Silent();
}

void MediumCanvasVisualizer::transmissionAdded(const ITransmission *transmission)
{
    Enter_Method_Silent();
    if (!hasGUI()) return;
    if (displayCommunication) {
        transmissions.push_back(transmission);
        cFigure::Point position = canvasProjection->computeCanvasPoint(transmission->getStartPosition());
        cGroupFigure *groupFigure = new cGroupFigure();
#if OMNETPP_CANVAS_VERSION >= 0x20140908
        cFigure::Color color = cFigure::GOOD_DARK_COLORS[transmission->getId() % (sizeof(cFigure::GOOD_DARK_COLORS) / sizeof(cFigure::Color))];
        cRingFigure *communicationFigure = new cRingFigure();
#else
        cFigure::Color color(64 + rand() % 64, 64 + rand() % 64, 64 + rand() % 64);
        cOvalFigure *communicationFigure = new cOvalFigure();
#endif
        communicationFigure->setTags("ongoing_transmission");
        communicationFigure->setBounds(cFigure::Rectangle(position.x, position.y, 0, 0));
        communicationFigure->setFillColor(color);
        communicationFigure->setLineWidth(1);
        communicationFigure->setLineColor(cFigure::BLACK);
        groupFigure->addFigure(communicationFigure);
#if OMNETPP_CANVAS_VERSION >= 0x20140908
        communicationFigure->setFilled(true);
        communicationFigure->setFillOpacity(0.5);
        communicationFigure->setLineOpacity(0.5);
        communicationFigure->setZoomLineWidth(false);
        cLabelFigure *nameFigure = new cLabelFigure();
        nameFigure->setPosition(position);
#else
        cTextFigure *nameFigure = new cTextFigure();
        nameFigure->setLocation(position);
#endif
        nameFigure->setTags("ongoing_transmission packet_name label");
        nameFigure->setText(transmission->getMacFrame()->getName());
        nameFigure->setColor(color);
        groupFigure->addFigure(nameFigure);
        communicationLayer->addFigure(groupFigure);
        setCachedFigure(transmission, groupFigure);
        updateCanvas();
        if (updateInterval > 0)
            scheduleUpdateCanvasTimer();
    }
}

void MediumCanvasVisualizer::transmissionRemoved(const ITransmission *transmission)
{
    Enter_Method_Silent();
    if (!hasGUI()) return;
    if (displayCommunication) {
        transmissions.erase(std::remove(transmissions.begin(), transmissions.end(), transmission));
        cFigure *figure = getCachedFigure(transmission);
        removeCachedFigure(transmission);
        if (figure != nullptr)
            delete communicationLayer->removeFigure(figure);
    }
}

void MediumCanvasVisualizer::packetReceived(const IReceptionDecision *decision)
{
    Enter_Method_Silent();
    if (!hasGUI()) return;
    if (decision->isReceptionSuccessful()) {
        const ITransmission *transmission = decision->getReception()->getTransmission();
        const IReception *reception = decision->getReception();
        if (leaveCommunicationTrail) {
            cLineFigure *communicationFigure = new cLineFigure();
            communicationFigure->setTags("successful_reception recent_history");
            cFigure::Point start = canvasProjection->computeCanvasPoint(transmission->getStartPosition());
            cFigure::Point end = canvasProjection->computeCanvasPoint(reception->getStartPosition());
            communicationFigure->setStart(start);
            communicationFigure->setEnd(end);
            communicationFigure->setLineColor(cFigure::BLUE);
            communicationFigure->setEndArrowHead(cFigure::ARROW_BARBED);
            communicationFigure->setLineWidth(1);
#if OMNETPP_CANVAS_VERSION >= 0x20140908
            communicationFigure->setZoomLineWidth(false);
#endif
            communicationTrail->addFigure(communicationFigure);
        }
#if OMNETPP_CANVAS_VERSION >= 0x20140908
        if (leaveCommunicationHeat) {
            const IMediumLimitCache *mediumLimitCache = radioMedium->getMediumLimitCache();
            Coord min = mediumLimitCache->getMinConstraintArea();
            Coord max = mediumLimitCache->getMaxConstraintArea();
            Coord delta = max - min;
            int x1 = std::round((communicationHeatMapSize - 1) * ((transmission->getStartPosition().x - min.x) / delta.x));
            int y1 = std::round((communicationHeatMapSize - 1) * ((transmission->getStartPosition().y - min.x) / delta.y));
            int x2 = std::round((communicationHeatMapSize - 1) * ((reception->getStartPosition().x - min.x) / delta.x));
            int y2 = std::round((communicationHeatMapSize - 1) * ((reception->getStartPosition().y - min.y) / delta.y));
            communicationHeat->heatLine(x1, y1, x2, y2);
        }
#endif
    }
    if (displayCommunication)
        updateCanvas();
}

void MediumCanvasVisualizer::updateCanvas() const
{
    const IPropagation *propagation = radioMedium->getPropagation();
#if OMNETPP_CANVAS_VERSION >= 0x20140908
    if (communicationHeat != nullptr)
        communicationHeat->coolDown();
#endif
    for (const auto transmission : transmissions) {
        cFigure *groupFigure = getCachedFigure(transmission);
        double startRadius = propagation->getPropagationSpeed().get() * (simTime() - transmission->getStartTime()).dbl();
        double endRadius = std::max(0.0, propagation->getPropagationSpeed().get() * (simTime() - transmission->getEndTime()).dbl());
        if (groupFigure) {
#if OMNETPP_CANVAS_VERSION >= 0x20140908
            cRingFigure *communicationFigure = (cRingFigure *)groupFigure->getFigure(0);
#else
            cOvalFigure *communicationFigure = (cOvalFigure *)groupFigure->getFigure(0);
#endif
            const Coord transmissionStart = transmission->getStartPosition();
            // KLUDGE: to workaround overflow bugs in drawing
            if (startRadius > 10000)
                startRadius = 10000;
            if (endRadius > 10000)
                endRadius = 10000;
#if OMNETPP_CANVAS_VERSION >= 0x20140908
            if (drawCommunication2D) {
                // determine the rotated 2D canvas points by computing the 2D affine trasnformation from the 3D transformation of the environment
                cFigure::Point o = canvasProjection->computeCanvasPoint(transmissionStart);
                cFigure::Point x = canvasProjection->computeCanvasPoint(transmissionStart + Coord(1, 0, 0));
                cFigure::Point y = canvasProjection->computeCanvasPoint(transmissionStart + Coord(0, 1, 0));
                double t1 = o.x;
                double t2 = o.y;
                double a = x.x - t1;
                double b = x.y - t2;
                double c = y.x - t1;
                double d = y.y - t2;
                communicationFigure->setTransform(cFigure::Transform(a, b, c, d, t1, t2));
                communicationFigure->setBounds(cFigure::Rectangle(-startRadius, -startRadius, startRadius * 2, startRadius * 2));
                communicationFigure->setInnerRx(endRadius);
                communicationFigure->setInnerRy(endRadius);
            }
            else {
#else
            {
#endif
                // a sphere looks like a circle from any view angle
                cFigure::Point center = canvasProjection->computeCanvasPoint(transmissionStart);
                communicationFigure->setBounds(cFigure::Rectangle(center.x - startRadius, center.y - startRadius, 2 * startRadius, 2 * startRadius));
#if OMNETPP_CANVAS_VERSION >= 0x20140908
                communicationFigure->setInnerRx(endRadius);
                communicationFigure->setInnerRy(endRadius);
#endif
            }
        }
    }
}

void MediumCanvasVisualizer::scheduleUpdateCanvasTimer()
{
    if (updateCanvasTimer->isScheduled())
        cancelEvent(updateCanvasTimer);
    simtime_t nextUpdateTime = SimTime::getMaxTime();
    for (auto transmission : transmissions) {
        simtime_t transmissionNextUpdateTime = getTransmissionNextUpdateTime(transmission);
        if (transmissionNextUpdateTime < nextUpdateTime)
            nextUpdateTime = transmissionNextUpdateTime;
    }
    if (nextUpdateTime != SimTime::getMaxTime()) {
        scheduleAt(nextUpdateTime, updateCanvasTimer);
    }
}

void MediumCanvasVisualizer::setInterferenceRange(const IRadio *radio)
{
    auto module = check_and_cast<const cModule *>(radio);
    auto node = findContainingNode(const_cast<cModule *>(module));
    cDisplayString& displayString = node->getDisplayString();
    m maxInterferenceRage = check_and_cast<const RadioMedium *>(radio->getMedium())->getMediumLimitCache()->getMaxInterferenceRange(radio);
    const char *tag = "r1";
    displayString.removeTag(tag);
    displayString.insertTag(tag);
    displayString.setTagArg(tag, 0, maxInterferenceRage.get());
    displayString.setTagArg(tag, 2, "gray");
}

void MediumCanvasVisualizer::setCommunicationRange(const IRadio *radio)
{
    auto module = check_and_cast<const cModule *>(radio);
    auto node = findContainingNode(const_cast<cModule *>(module));
    cDisplayString& displayString = node->getDisplayString();
    m maxCommunicationRange = check_and_cast<const RadioMedium *>(radio->getMedium())->getMediumLimitCache()->getMaxCommunicationRange(radio);
    const char *tag = "r2";
    displayString.removeTag(tag);
    displayString.insertTag(tag);
    displayString.setTagArg(tag, 0, maxCommunicationRange.get());
    displayString.setTagArg(tag, 2, "blue");
}

} // namespace physicallayer

} // namespace inet

