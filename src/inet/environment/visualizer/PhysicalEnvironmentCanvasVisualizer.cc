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
#include "inet/common/geometry/shape/Cuboid.h"
#include "inet/common/geometry/shape/Sphere.h"
#include "inet/common/geometry/shape/Prism.h"
#include "inet/common/geometry/shape/polyhedron/Polyhedron.h"
#include "inet/environment/visualizer/PhysicalEnvironmentCanvasVisualizer.h"
#include <algorithm>

namespace inet {

namespace physicalenvironment {

Define_Module(PhysicalEnvironmentCanvasVisualizer);

void PhysicalEnvironmentCanvasVisualizer::initialize(int stage)
{
    PhysicalEnvironmentVisualizerBase::initialize(stage);
    if (!hasGUI()) return;
    if (stage == INITSTAGE_LOCAL) {
        objectsLayer = new cGroupFigure();
        cCanvas *canvas = visualizerTargetModule->getCanvas();
        canvas->addFigureBelow(objectsLayer, canvas->getSubmodulesLayer());
        canvasProjection.setRotation(Rotation(computeViewAngle(par("viewAngle"))));
        canvasProjection.setTranslation(computeViewTranslation(par("viewTranslation")));
        CanvasProjection::setCanvasProjection(canvas, &canvasProjection);
    }
    else if (stage == INITSTAGE_LAST)
        updateCanvas();
}

void PhysicalEnvironmentCanvasVisualizer::handleParameterChange(const char* name)
{
    if (name && !strcmp(name, "viewAngle")) {
        canvasProjection.setRotation(Rotation(computeViewAngle(par("viewAngle"))));
        updateCanvas();
    }
    else if (name && !strcmp(name, "viewTranslation")) {
        canvasProjection.setTranslation(computeViewTranslation(par("viewTranslation")));
        updateCanvas();
    }
}

void PhysicalEnvironmentCanvasVisualizer::updateCanvas()
{
    while (objectsLayer->getNumFigures())
        delete objectsLayer->removeFigure(0);
    // KLUDGE: TODO: sorting objects with their rotated position's z coordinate to draw them in a "better" order
    std::vector<const PhysicalObject *> objectsCopy;
    for (int i = 0; i < physicalEnvironment->getNumObjects(); i++)
        objectsCopy.push_back(physicalEnvironment->getObject(i));
    std::stable_sort(objectsCopy.begin(), objectsCopy.end(), ObjectPositionComparator(canvasProjection.getRotation()));
    for (auto object : objectsCopy) {
        const ShapeBase *shape = object->getShape();
        const Coord& position = object->getPosition();
        const EulerAngles& orientation = object->getOrientation();
        const Rotation rotation(orientation);
        // cuboid
        const Cuboid *cuboid = dynamic_cast<const Cuboid *>(shape);
        if (cuboid) {
            std::vector<std::vector<Coord> > faces;
            cuboid->computeVisibleFaces(faces, rotation, canvasProjection.getRotation());
            computeFacePoints(object, faces, rotation);
        }
        // sphere
        const Sphere *sphere = dynamic_cast<const Sphere *>(shape);
        if (sphere) {
            double radius = sphere->getRadius();
            cOvalFigure *figure = new cOvalFigure();
            figure->setFilled(true);
            cFigure::Point center = canvasProjection.computeCanvasPoint(position);
            figure->setBounds(cFigure::Rectangle(center.x - radius, center.y - radius, radius * 2, radius * 2));
            figure->setLineWidth(object->getLineWidth());
            figure->setLineColor(object->getLineColor());
            figure->setFillColor(object->getFillColor());
#if OMNETPP_CANVAS_VERSION >= 0x20140908
            figure->setLineOpacity(object->getOpacity());
            figure->setFillOpacity(object->getOpacity());
            figure->setZoomLineWidth(false);
#endif
            std::string tags("physical_object ");
            if (object->getTags())
                tags += object->getTags();
            figure->setTags(tags.c_str());
            objectsLayer->addFigure(figure);
        }
        // prism
        const Prism *prism = dynamic_cast<const Prism *>(shape);
        if (prism) {
            std::vector<std::vector<Coord> > faces;
            prism->computeVisibleFaces(faces, rotation, canvasProjection.getRotation());
            computeFacePoints(object, faces, rotation);
        }
        // polyhedron
        const Polyhedron *polyhedron = dynamic_cast<const Polyhedron *>(shape);
        if (polyhedron) {
            std::vector<std::vector<Coord> > faces;
            polyhedron->computeVisibleFaces(faces, rotation, canvasProjection.getRotation());
            computeFacePoints(object, faces, rotation);
        }
        // add name to the end
        const char *name = object->getName();
        if (name) {
#if OMNETPP_CANVAS_VERSION >= 0x20140908
            cLabelFigure *nameFigure = new cLabelFigure();
            nameFigure->setPosition(canvasProjection.computeCanvasPoint(position));
#else
            cTextFigure *nameFigure = new cTextFigure();
            nameFigure->setLocation(canvasProjection.computeCanvasPoint(position));
#endif
            nameFigure->setTags("physical_object object_name label");
            nameFigure->setText(name);
            objectsLayer->addFigure(nameFigure);
        }
    }
    if (!_isNaN(axisLength)) {
        cLineFigure *xAxis = new cLineFigure();
        cLineFigure *yAxis = new cLineFigure();
        cLineFigure *zAxis = new cLineFigure();
        xAxis->setTags("axis");
        yAxis->setTags("axis");
        zAxis->setTags("axis");
        xAxis->setLineWidth(1);
        yAxis->setLineWidth(1);
        zAxis->setLineWidth(1);
        xAxis->setEndArrowHead(cFigure::ARROW_BARBED);
        yAxis->setEndArrowHead(cFigure::ARROW_BARBED);
        zAxis->setEndArrowHead(cFigure::ARROW_BARBED);
#if OMNETPP_CANVAS_VERSION >= 0x20140908
        xAxis->setZoomLineWidth(false);
        yAxis->setZoomLineWidth(false);
        zAxis->setZoomLineWidth(false);
#endif
        xAxis->setStart(canvasProjection.computeCanvasPoint(Coord::ZERO));
        yAxis->setStart(canvasProjection.computeCanvasPoint(Coord::ZERO));
        zAxis->setStart(canvasProjection.computeCanvasPoint(Coord::ZERO));
        xAxis->setEnd(canvasProjection.computeCanvasPoint(Coord(axisLength, 0, 0)));
        yAxis->setEnd(canvasProjection.computeCanvasPoint(Coord(0, axisLength, 0)));
        zAxis->setEnd(canvasProjection.computeCanvasPoint(Coord(0, 0, axisLength)));
        objectsLayer->addFigure(xAxis);
        objectsLayer->addFigure(yAxis);
        objectsLayer->addFigure(zAxis);
#if OMNETPP_CANVAS_VERSION >= 0x20140908
        cLabelFigure *xLabel = new cLabelFigure();
        cLabelFigure *yLabel = new cLabelFigure();
        cLabelFigure *zLabel = new cLabelFigure();
#else
        cTextFigure *xLabel = new cTextFigure();
        cTextFigure *yLabel = new cTextFigure();
        cTextFigure *zLabel = new cTextFigure();
#endif
        xLabel->setTags("axis label");
        yLabel->setTags("axis label");
        zLabel->setTags("axis label");
        xLabel->setText("X");
        yLabel->setText("Y");
        zLabel->setText("Z");
#if OMNETPP_CANVAS_VERSION >= 0x20140908
        xLabel->setPosition(canvasProjection.computeCanvasPoint(Coord(axisLength, 0, 0)));
        yLabel->setPosition(canvasProjection.computeCanvasPoint(Coord(0, axisLength, 0)));
        zLabel->setPosition(canvasProjection.computeCanvasPoint(Coord(0, 0, axisLength)));
#else
        xLabel->setLocation(canvasProjection.computeCanvasPoint(Coord(axisLength, 0, 0)));
        yLabel->setLocation(canvasProjection.computeCanvasPoint(Coord(0, axisLength, 0)));
        zLabel->setLocation(canvasProjection.computeCanvasPoint(Coord(0, 0, axisLength)));
#endif
        objectsLayer->addFigure(xLabel);
        objectsLayer->addFigure(yLabel);
        objectsLayer->addFigure(zLabel);
    }
}

EulerAngles PhysicalEnvironmentCanvasVisualizer::computeViewAngle(const char* viewAngle)
{
    double x, y, z;
    if (!strcmp(viewAngle, "x"))
    {
        x = 0;
        y = M_PI / 2;
        z = M_PI / -2;
    }
    else if (!strcmp(viewAngle, "y"))
    {
        x = M_PI / 2;
        y = 0;
        z = 0;
    }
    else if (!strcmp(viewAngle, "z"))
    {
        x = y = z = 0;
    }
    else if (!strncmp(viewAngle, "isometric", 9))
    {
        int v;
        int l = strlen(viewAngle);
        switch (l) {
            case 9: v = 0; break;
            case 10: v = viewAngle[9] - '0'; break;
            case 11: v = (viewAngle[9] - '0') * 10 + viewAngle[10] - '0'; break;
            default: throw cRuntimeError("Invalid isometric viewAngle parameter");
        }
        // 1st axis can point on the 2d plane in 6 directions
        // 2nd axis can point on the 2d plane in 4 directions (the opposite direction is forbidden)
        // 3rd axis can point on the 2d plane in 2 directions
        // this results in 6 * 4 * 2 = 48 different configurations
        x = math::deg2rad(45 + v % 4 * 90);
        y = math::deg2rad(v / 24 % 2 ? 35.27 : -35.27);
        z = math::deg2rad(30 + v / 4 % 6 * 60);
    }
    else if (sscanf(viewAngle, "%lf %lf %lf", &x, &y, &z) == 3)
    {
        x = math::deg2rad(x);
        y = math::deg2rad(y);
        z = math::deg2rad(z);
    }
    else
        throw cRuntimeError("The viewAngle parameter must be a predefined string or a triplet representing three degrees");
    return EulerAngles(x, y, z);
}

cFigure::Point PhysicalEnvironmentCanvasVisualizer::computeViewTranslation(const char* viewTranslation)
{
    double x, y;
    if (sscanf(viewTranslation, "%lf %lf", &x, &y) == 2)
    {
        x = math::deg2rad(x);
        y = math::deg2rad(y);
    }
    else
        throw cRuntimeError("The viewTranslation parameter must be a pair of doubles");
    return cFigure::Point(x, y);
}

void PhysicalEnvironmentCanvasVisualizer::computeFacePoints(const PhysicalObject *object, std::vector<std::vector<Coord> >& faces, const Rotation& rotation)
{
    const Coord& position = object->getPosition();
    for (std::vector<std::vector<Coord> >::const_iterator it = faces.begin(); it != faces.end(); it++)
    {
        std::vector<cFigure::Point> canvasPoints;
        const std::vector<Coord>& facePoints = *it;
        for (const auto & facePoint : facePoints)
        {
            cFigure::Point canvPoint = canvasProjection.computeCanvasPoint(rotation.rotateVectorClockwise(facePoint) + position);
            canvasPoints.push_back(canvPoint);
        }
        cPolygonFigure *figure = new cPolygonFigure();
        figure->setFilled(true);
        figure->setPoints(canvasPoints);
        figure->setLineWidth(object->getLineWidth());
        figure->setLineColor(object->getLineColor());
        figure->setFillColor(object->getFillColor());
#if OMNETPP_CANVAS_VERSION >= 0x20140908
        figure->setLineOpacity(object->getOpacity());
        figure->setFillOpacity(object->getOpacity());
        figure->setZoomLineWidth(false);
#endif
        std::string tags("physical_object ");
        if (object->getTags())
            tags += object->getTags();
        figure->setTags(tags.c_str());
        objectsLayer->addFigure(figure);
    }
}

} // namespace physicalenvironment

} // namespace inet

