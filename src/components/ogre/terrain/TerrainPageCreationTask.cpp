/*
 Copyright (C) 2009 Erik Hjortsberg <erik.hjortsberg@gmail.com>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "TerrainPageCreationTask.h"

#include "TerrainManager.h"
#include "TerrainPage.h"
#include "TerrainShader.h"
#include "TerrainPageGeometry.h"
#include "TerrainMaterialCompilationTask.h"
#include "HeightMapUpdateTask.h"
#include "components/ogre/EmberOgre.h"
#include "components/ogre/EmberEntity.h"
#include "components/ogre/EmberEntityFactory.h"
#include "components/ogre/WorldEmberEntity.h"
#include "components/ogre/environment/Environment.h"

#include "framework/tasks/TaskExecutionContext.h"

#include <OgreMaterial.h>

namespace EmberOgre
{

namespace Terrain
{

TerrainPageCreationTask::TerrainPageCreationTask(TerrainManager& TerrainManager, const TerrainPosition& pos, ITerrainPageBridge* bridge, HeightMapBufferProvider& heightMapBufferProvider, HeightMap& heightMap) :
	mTerrainManager(TerrainManager), mPage(0), mPos(pos), mBridge(bridge), mHeightMapBufferProvider(heightMapBufferProvider), mHeightMap(heightMap)
{

}

TerrainPageCreationTask::~TerrainPageCreationTask()
{

}

void TerrainPageCreationTask::executeTaskInBackgroundThread(Ember::Tasks::TaskExecutionContext& context)
{
	mPage = new TerrainPage(mPos, mTerrainManager);
	mPage->registerBridge(mBridge);

	//add the base shaders, this should probably be refactored into a server side thing in the future
	const std::list<TerrainShader*>& baseShaders = mTerrainManager.getBaseShaders();
	for (std::list<TerrainShader*>::const_iterator I = baseShaders.begin(); I != baseShaders.end(); ++I) {
		mPage->addShader(*I);
	}

	mPage->updateAllShaderTextures(true);

	mPage->createShadowData(EmberOgre::getSingleton().getEntityFactory()->getWorld()->getEnvironment()->getSun()->getSunDirection());

	// setup foliage
	if (mTerrainManager.isFoliageShown()) {
		mPage->showFoliage();
	}

	context.executeTask(new TerrainMaterialCompilationTask(mPage));
	std::vector<Mercator::Segment*> segments;
	const SegmentVector& segmentVector = mPage->getGeometry().getValidSegments();
	for (SegmentVector::const_iterator I = segmentVector.begin(); I != segmentVector.end(); ++I) {
		segments.push_back(I->segment);
	}
	context.executeTask(new HeightMapUpdateTask(mHeightMapBufferProvider, mHeightMap, segments));
}

void TerrainPageCreationTask::executeTaskInMainThread()
{
	if (mPage) {

		//		mPage->loadShadow();
		//		mPage->generateTerrainMaterials(false);

		mTerrainManager.addPage(mPage);

		mPage->notifyBridgePageReady();
	}
}

}

}