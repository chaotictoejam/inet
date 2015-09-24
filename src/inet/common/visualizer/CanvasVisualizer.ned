//
// Copyright (C) 2006 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

package inet.common.visualizer;

import inet.environment.visualizer.PhysicalEnvironmentCanvasVisualizer;
import inet.mobility.visualizer.MobilityCanvasVisualizer;
import inet.physicallayer.visualizer.MediumCanvasVisualizer;

module CanvasVisualizer like IVisualizer
{
    parameters:
        @display("bgb=0,0;i=block/app");
    submodules:
        physicalEnvironmentVisualizer: PhysicalEnvironmentCanvasVisualizer {
            parameters:
                @display("p=200,50");
        }
        mobilityVisualizer: MobilityCanvasVisualizer {
            parameters:
                @display("p=200,150");
        }
        mediumVisualizer: MediumCanvasVisualizer {
            parameters:
                @display("p=200,250");
        }
}
