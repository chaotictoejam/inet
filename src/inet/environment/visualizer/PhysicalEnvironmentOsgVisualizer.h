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

#ifndef __INET_PHYSICALENVIRONMENTOSGVISUALIZER_H
#define __INET_PHYSICALENVIRONMENTOSGVISUALIZER_H

#include "inet/environment/base/PhysicalEnvironmentVisualizerBase.h"
#include "inet/environment/common/PhysicalEnvironment.h"

namespace inet {

namespace physicalenvironment {

class INET_API PhysicalEnvironmentOsgVisualizer : public PhysicalEnvironmentVisualizerBase
{
  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;

    virtual osg::Geode *createPlayground();
    virtual void updateScene();

  public:
    PhysicalEnvironmentOsgVisualizer() {}
    virtual ~PhysicalEnvironmentOsgVisualizer() {}
};

} // namespace physicalenvironment

} // namespace inet

#endif // ifndef __INET_PHYSICALENVIRONMENTOSGVISUALIZER_H

#endif // WITH_OSG

