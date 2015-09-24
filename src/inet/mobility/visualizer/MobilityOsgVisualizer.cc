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

#include "inet/common/ModuleAccess.h"
#include "inet/common/OSGUtils.h"
#include "inet/mobility/visualizer/MobilityOsgVisualizer.h"
#include <osg/AutoTransform>
#include <osg/Material>
#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <osgText/Text>

namespace inet {

Define_Module(MobilityOsgVisualizer);

MobilityOsgVisualizer::CacheEntry::CacheEntry(osg::PositionAttitudeTransform *positionAttitudeTransform, osg::Geode *trail) :
    positionAttitudeTransform(positionAttitudeTransform),
    trail(trail)
{
}

MobilityOsgVisualizer::CacheEntry *MobilityOsgVisualizer::getCacheEntry(const IMobility *mobility) const
{
    auto it = cacheEntries.find(mobility);
    if (it == cacheEntries.end())
        return nullptr;
    else
        return it->second;
}

void MobilityOsgVisualizer::setCacheEntry(const IMobility *mobility, CacheEntry *entry)
{
    cacheEntries[mobility] = entry;
}

void MobilityOsgVisualizer::removeCacheEntry(const IMobility *mobility)
{
    cacheEntries.erase(mobility);
}

MobilityOsgVisualizer::CacheEntry* MobilityOsgVisualizer::ensureCacheEntry(const IMobility *mobility)
{
    auto cacheEntry = getCacheEntry(mobility);
    if (cacheEntry == nullptr) {
        auto module = check_and_cast<const cModule *>(mobility);
        auto trail = new osg::Geode();
#if OMNETPP_CANVAS_VERSION >= 0x20140908
        cFigure::Color color = cFigure::GOOD_DARK_COLORS[module->getId() % (sizeof(cFigure::GOOD_DARK_COLORS) / sizeof(cFigure::Color))];
#else
        cFigure::Color color = cFigure::BLACK;
#endif
        trail->setStateSet(inet::osg::createStateSet(color, 1.0));
        auto positionAttitudeTransform = createOsgNode(mobility);
        auto scene = inet::osg::getScene(visualizerTargetModule);
        scene->addChild(positionAttitudeTransform);
        scene->addChild(trail);
        cacheEntry = new CacheEntry(positionAttitudeTransform, trail);
        setCacheEntry(mobility, cacheEntry);
    }
    return cacheEntry;
}

osg::PositionAttitudeTransform* MobilityOsgVisualizer::createOsgNode(const IMobility *mobility)
{
    auto module = const_cast<cModule *>(check_and_cast<const cModule *>(mobility));
    auto networkNode = getModuleFromPar<cModule>(module->par("visualizerTargetModule"), module);
    osg::Node *osgNode = nullptr;
    if (networkNode->hasPar("visualization") && strlen(networkNode->par("visualization")) != 0) {
        const char *visualization = networkNode->par("visualization");
        osgNode = osgDB::readNodeFile(networkNode->par("visualization"));
        if (osgNode == nullptr)
            throw cRuntimeError("Model file \"%s\" not found", visualization);
    }
    else {
        // TODO: get real images path
        std::string path("/home/levy/workspace/omnetpp/images/");
        path += networkNode->getDisplayString().getTagArg("i", 0);
        path += ".png";
        auto image = osgDB::readImageFile(path.c_str());
        auto texture = new osg::Texture2D();
        texture->setImage(image);
        auto geometry = osg::createTexturedQuadGeometry(osg::Vec3(0.0, 0.0, 0.0), osg::Vec3(image->s(), 0.0, 0.0), osg::Vec3(0.0, image->t(), 0.0), 0.0, 0.0, 1.0, 1.0);
        geometry->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture);
        geometry->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
        geometry->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        geometry->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        auto text = new osgText::Text();
        text->setColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
        text->setCharacterSize(18);
        text->setText(networkNode->getFullName());
        auto geode = new osg::Geode();
        geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        geode->addDrawable(geometry);
        geode->addDrawable(text);
        auto autoTransform = new osg::AutoTransform();
        autoTransform->setPivotPoint(osg::Vec3d(0.5, 0.5, 0.0));
        autoTransform->setAutoScaleToScreen(true);
        autoTransform->setAutoRotateMode(osg::AutoTransform::ROTATE_TO_SCREEN);
        autoTransform->addChild(geode);
        autoTransform->setPosition(osg::Vec3d(0.0, 0.0, 0.0));
        osgNode = autoTransform;
    }
    auto objectNode = cOsgCanvas::createOmnetppObjectNode(module);
    objectNode->addChild(osgNode);
    auto positionAttitudeTransform = new osg::PositionAttitudeTransform();
    positionAttitudeTransform->addChild(objectNode);
    return positionAttitudeTransform;
}

void MobilityOsgVisualizer::extendMovementTrail(osg::Geode *trail, const Coord& position)
{
    if (trail->getNumDrawables() == 0)
        trail->addDrawable(inet::osg::createLineGeometry(position, position));
    else {
        auto drawable = static_cast<osg::Geometry *>(trail->getDrawable(0));
        auto vertices = static_cast<osg::Vec3Array *>(drawable->getVertexArray());
        auto lastPosition = vertices->at(vertices->size() - 1);
        auto dx = lastPosition.x() - position.x;
        auto dy = lastPosition.y() - position.y;
        auto dz = lastPosition.z() - position.z;
        // TODO: 1?
        if (dx * dx + dy * dy + dz * dz > 1) {
            vertices->push_back(osg::Vec3d(position.x, position.y, position.z));
            auto drawArrays = static_cast<osg::DrawArrays *>(drawable->getPrimitiveSet(0));
            drawArrays->setFirst(0);
            drawArrays->setCount(vertices->size());
            drawable->dirtyBound();
            drawable->dirtyDisplayList();
        }
    }
}

void MobilityOsgVisualizer::setPosition(osg::PositionAttitudeTransform *positionAttitudeTransform, const Coord& position)
{
    positionAttitudeTransform->setPosition(osg::Vec3d(position.x, position.y, position.z));
}

void MobilityOsgVisualizer::receiveSignal(cComponent *source, simsignal_t signal, cObject *object)
{
    Enter_Method_Silent();
    if (!hasGUI()) return;
    if (signal == IMobility::mobilityStateChangedSignal) {
        auto mobility = dynamic_cast<IMobility *>(object);
        auto position = mobility->getCurrentPosition();
        auto cacheEntry = ensureCacheEntry(mobility);
        setPosition(cacheEntry->positionAttitudeTransform, position);
        extendMovementTrail(cacheEntry->trail, position);
    }
}

} // namespace inet

#endif // WITH_OSG

