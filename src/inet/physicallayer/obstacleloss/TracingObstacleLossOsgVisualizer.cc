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
#include "inet/physicallayer/obstacleloss/TracingObstacleLossOsgVisualizer.h"

namespace inet {

namespace physicallayer {

Define_Module(TracingObstacleLossOsgVisualizer);

void TracingObstacleLossOsgVisualizer::obstaclePenetrated(const IPhysicalObject *object, const Coord& intersection1, const Coord& intersection2, const Coord& normal1, const Coord& normal2)
{
    Enter_Method_Silent();
    if (!hasGUI()) return;
    if (leaveIntersectionTrail || leaveFaceNormalVectorTrail) {
        const Rotation rotation(object->getOrientation());
        const Coord& position = object->getPosition();
        const Coord rotatedIntersection1 = rotation.rotateVectorClockwise(intersection1);
        const Coord rotatedIntersection2 = rotation.rotateVectorClockwise(intersection2);
        double intersectionDistance = intersection2.distance(intersection1);
        if (leaveIntersectionTrail) {
            // TODO:
        }
        if (leaveFaceNormalVectorTrail) {
            // TODO:
        }
    }
}

} // namespace physicallayer

} // namespace inet

#endif // WITH_OSG

