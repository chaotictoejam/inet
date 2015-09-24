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

package inet.environment.visualizer;

import inet.environment.base.PhysicalEnvironmentVisualizerBase;

simple PhysicalEnvironmentCanvasVisualizer extends PhysicalEnvironmentVisualizerBase
{
    parameters:
        @class(PhysicalEnvironmentCanvasVisualizer);
        string viewAngle = default("z"); // the angle of view is either x, y, z, isometricN (where 0 <= N < 48 defaults to 0) or three rotation angles separated by spaces (it uses orthographic projection that is the projection is parallel to the normal of the viewing surface)
        string viewTranslation = default("0 0"); // the translation is applied on the 2 dimensional projection
}
