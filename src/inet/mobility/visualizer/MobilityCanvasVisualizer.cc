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
#include "inet/mobility/visualizer/MobilityCanvasVisualizer.h"

namespace inet {

Define_Module(MobilityCanvasVisualizer);

MobilityCanvasVisualizer::CacheEntry::CacheEntry(cModule *visualRepresentation, cImageFigure *mainFigure, TrailFigure *trailFigure) :
    visualRepresentation(visualRepresentation),
    mainFigure(mainFigure),
    trailFigure(trailFigure)
{
}

MobilityCanvasVisualizer::~MobilityCanvasVisualizer()
{
    for (auto cacheEntry : cacheEntries)
        delete cacheEntry.second;
}

void MobilityCanvasVisualizer::initialize(int stage)
{
    MobilityVisualizerBase::initialize(stage);
    if (!hasGUI()) return;
    if (stage == INITSTAGE_PHYSICAL_ENVIRONMENT)
        canvasProjection = CanvasProjection::getCanvasProjection(visualizerTargetModule->getCanvas());
}

cModule *MobilityCanvasVisualizer::findVisualRepresentation(cModule *module) const
{
    while (module != nullptr && module->getParentModule() != visualizerTargetModule)
        module = module->getParentModule();
    return module;
}

MobilityCanvasVisualizer::CacheEntry *MobilityCanvasVisualizer::getCacheEntry(const IMobility *mobility) const
{
    auto it = cacheEntries.find(mobility);
    if (it == cacheEntries.end())
        return nullptr;
    else
        return it->second;
}

void MobilityCanvasVisualizer::setCacheEntry(const IMobility *mobility, CacheEntry *entry)
{
    cacheEntries[mobility] = entry;
}

void MobilityCanvasVisualizer::removeCacheEntry(const IMobility *mobility)
{
    cacheEntries.erase(mobility);
}

MobilityCanvasVisualizer::CacheEntry* MobilityCanvasVisualizer::ensureCacheEntry(const IMobility *mobility)
{
    auto cacheEntry = getCacheEntry(mobility);
    if (cacheEntry == nullptr) {
        auto canvas = visualizerTargetModule->getCanvas();
        auto module = const_cast<cModule *>(dynamic_cast<const cModule *>(mobility));
        auto visualRepresentation = findVisualRepresentation(module);
        auto mainFigure = visualRepresentation == nullptr ? new cImageFigure() : nullptr;
        if (mainFigure != nullptr) {
            mainFigure->setImageName(module->getDisplayString().getTagArg("i", 0));
            canvas->addFigure(mainFigure);
        }
        auto trailFigure = leaveMovementTrail ? new TrailFigure(100, true, "movement trail") : nullptr;
        if (trailFigure != nullptr)
            canvas->addFigure(trailFigure);
        cacheEntry = new CacheEntry(visualRepresentation, mainFigure, trailFigure);
        setCacheEntry(mobility, cacheEntry);
    }
    return cacheEntry;
}

void MobilityCanvasVisualizer::extendMovementTrail(TrailFigure *trailFigure, cFigure::Point position)
{
    cFigure::Point startPosition;
    cFigure::Point endPosition = position;
    if (trailFigure->getNumFigures() == 0)
        startPosition = position;
    else
        startPosition = static_cast<cLineFigure*>(trailFigure->getFigure(trailFigure->getNumFigures() - 1))->getEnd();
    double dx = startPosition.x - endPosition.x;
    double dy = startPosition.y - endPosition.y;
    // TODO: 1?
    if (trailFigure->getNumFigures() == 0 || dx * dx + dy * dy > 1) {
        cLineFigure *movementLine = new cLineFigure();
        movementLine->setTags("movement_trail recent_history");
        movementLine->setStart(startPosition);
        movementLine->setEnd(endPosition);
        movementLine->setLineWidth(1);
#if OMNETPP_CANVAS_VERSION >= 0x20140908
        // TODO: module id
        cFigure::Color color = cFigure::GOOD_DARK_COLORS[getId() % (sizeof(cFigure::GOOD_DARK_COLORS) / sizeof(cFigure::Color))];
        movementLine->setLineColor(color);
        movementLine->setZoomLineWidth(false);
#else
        movementLine->setLineColor(cFigure::BLACK);
#endif
        trailFigure->addFigure(movementLine);
    }
}

void MobilityCanvasVisualizer::receiveSignal(cComponent *source, simsignal_t signal, cObject *object)
{
    Enter_Method_Silent();
    if (!hasGUI()) return;
    if (signal == IMobility::mobilityStateChangedSignal) {
        auto mobility = dynamic_cast<IMobility *>(object);
        auto position = canvasProjection->computeCanvasPoint(mobility->getCurrentPosition());
        auto entry = ensureCacheEntry(mobility);
        if (entry->mainFigure != nullptr)
            entry->mainFigure->setPosition(position);
        if (entry->visualRepresentation != nullptr)
            setPosition(entry->visualRepresentation, position);
        if (leaveMovementTrail)
            extendMovementTrail(entry->trailFigure, position);
    }
}

void MobilityCanvasVisualizer::setPosition(cModule* visualRepresentation, cFigure::Point position)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%lf", position.x);
    buf[sizeof(buf) - 1] = 0;
    visualRepresentation->getDisplayString().setTagArg("p", 0, buf);
    snprintf(buf, sizeof(buf), "%lf", position.y);
    buf[sizeof(buf) - 1] = 0;
    visualRepresentation->getDisplayString().setTagArg("p", 1, buf);
}

} // namespace inet

