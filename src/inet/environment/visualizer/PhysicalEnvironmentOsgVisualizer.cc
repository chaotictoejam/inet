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
#include "inet/common/geometry/shape/Cuboid.h"
#include "inet/common/geometry/shape/Sphere.h"
#include "inet/common/geometry/shape/Prism.h"
#include "inet/common/geometry/shape/polyhedron/Polyhedron.h"
#include "inet/environment/visualizer/PhysicalEnvironmentOsgVisualizer.h"

namespace inet {

namespace physicalenvironment {

Define_Module(PhysicalEnvironmentOsgVisualizer);

void PhysicalEnvironmentOsgVisualizer::initialize(int stage)
{
    PhysicalEnvironmentVisualizerBase::initialize(stage);
    if (!hasGUI()) return;
    if (stage == INITSTAGE_LAST)
        updateScene();
}

osg::Geode *PhysicalEnvironmentOsgVisualizer::createPlayground()
{
    auto geometry = new osg::Geometry();
    auto vertices = new osg::Vec3Array();
    vertices->push_back(osg::Vec3d(physicalEnvironment->getSpaceMin().x, physicalEnvironment->getSpaceMin().y, physicalEnvironment->getSpaceMin().z));
    vertices->push_back(osg::Vec3d(physicalEnvironment->getSpaceMax().x, physicalEnvironment->getSpaceMin().y, physicalEnvironment->getSpaceMin().z));
    vertices->push_back(osg::Vec3d(physicalEnvironment->getSpaceMax().x, physicalEnvironment->getSpaceMax().y, physicalEnvironment->getSpaceMin().z));
    vertices->push_back(osg::Vec3d(physicalEnvironment->getSpaceMin().x, physicalEnvironment->getSpaceMax().y, physicalEnvironment->getSpaceMin().z));
    geometry->setVertexArray(vertices);
    geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));
    auto geode = new osg::Geode();
    geode->addDrawable(geometry);
    return geode;
}

void PhysicalEnvironmentOsgVisualizer::updateScene()
{
    auto scene = inet::osg::getScene(visualizerTargetModule);
// TODO: scene->addChild(createPlayground());
    for (int i = 0; i < physicalEnvironment->getNumObjects(); i++) {
        const PhysicalObject *object = physicalEnvironment->getObject(i);
        const ShapeBase *shape = object->getShape();
        const Coord& position = object->getPosition();
        const EulerAngles& orientation = object->getOrientation();
        const Rotation rotation(orientation);
        // cuboid
        const Cuboid *cuboid = dynamic_cast<const Cuboid *>(shape);
        if (cuboid != nullptr) {
            auto size = cuboid->getSize();
            auto drawable = new osg::ShapeDrawable(new osg::Box(osg::Vec3d(0, 0, 0), size.x, size.y, size.z));
            drawable->setStateSet(inet::osg::createStateSet(object->getFillColor(), object->getOpacity()));
            auto geode = new osg::Geode();
            geode->addDrawable(drawable);
            auto pat = inet::osg::createPositionAttitudeTransform(position, orientation);
            pat->addChild(geode);
            scene->addChild(pat);
        }
        // sphere
        const Sphere *sphere = dynamic_cast<const Sphere *>(shape);
        if (sphere != nullptr) {
            double radius = sphere->getRadius();
            auto drawable = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(position.x, position.y, position.z), radius));
            drawable->setStateSet(inet::osg::createStateSet(object->getFillColor(), object->getOpacity()));
            auto geode = new osg::Geode();
            geode->addDrawable(drawable);
            scene->addChild(geode);
        }
        // prism
        const Prism *prism = dynamic_cast<const Prism *>(shape);
        if (prism != nullptr) {
            auto geode = new osg::Geode();
            auto stateSet = inet::osg::createStateSet(object->getFillColor(), object->getOpacity());
            geode->addDrawable(inet::osg::createPolygonGeometry(prism->getBase().getPoints(), stateSet));
            geode->addDrawable(inet::osg::createPolygonGeometry(prism->getBase().getPoints(), stateSet, Coord(0, 0, prism->getHeight())));
            for (auto face : prism->getFaces())
                geode->addDrawable(inet::osg::createPolygonGeometry(face.getPoints(), stateSet));
            auto pat = inet::osg::createPositionAttitudeTransform(position, orientation);
            pat->addChild(geode);
            scene->addChild(pat);
        }
        // polyhedron
        const Polyhedron *polyhedron = dynamic_cast<const Polyhedron *>(shape);
        if (polyhedron != nullptr) {
            auto geode = new osg::Geode();
            auto stateSet = inet::osg::createStateSet(object->getFillColor(), object->getOpacity());
            for (auto face : polyhedron->getFaces()) {
                auto geometry = new osg::Geometry();
                auto vertices = new osg::Vec3Array();
                for (auto edge : face->getEdges())
                    vertices->push_back(osg::Vec3d(edge->getP1()->x, edge->getP1()->y, edge->getP1()->z));
                geometry->setVertexArray(vertices);
                geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON, 0, face->getEdges().size()));
                geometry->setStateSet(stateSet);
                geode->addDrawable(geometry);
            }
            auto pat = inet::osg::createPositionAttitudeTransform(position, orientation);
            pat->addChild(geode);
            scene->addChild(pat);
        }
    }
    if (!_isNaN(axisLength)) {
        osg::Geode* geode = new osg::Geode();
        geode->addDrawable(inet::osg::createLineGeometry(Coord::ZERO, Coord(axisLength, 0.0, 0.0)));
        geode->addDrawable(inet::osg::createLineGeometry(Coord::ZERO, Coord(0.0, axisLength, 0.0)));
        geode->addDrawable(inet::osg::createLineGeometry(Coord::ZERO, Coord(0.0, 0.0, axisLength)));
        auto stateSet = inet::osg::createStateSet(cFigure::BLACK, 1.0);
        geode->setStateSet(stateSet);
        scene->addChild(geode);
    }
}

} // namespace physicalenvironment

} // namespace inet

#endif // WITH_OSG

