/*
-----------------------------------------------------------------------------------------------
Copyright (C) 2013 Henry van Merode. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
-----------------------------------------------------------------------------------------------
*/

#include "ParticleUniversePCH.h"

#ifndef PARTICLE_UNIVERSE_EXPORTS
#define PARTICLE_UNIVERSE_EXPORTS
#endif

#include "ParticleRenderers/ParticleUniverseBillboardRenderer.h"
#include "ParticleRenderers/ParticleUniverseBillboard.h"

namespace ParticleUniverse
{
	// Constants
	const BillboardRenderer::BillboardType BillboardRenderer::DEFAULT_BILLBOARD_TYPE = BillboardRenderer::BBT_POINT;
	const bool BillboardRenderer::DEFAULT_ACCURATE_FACING = false;
	const Ogre::v1::BillboardOrigin BillboardRenderer::DEFAULT_ORIGIN = Ogre::v1::BBO_CENTER;
	const Ogre::v1::BillboardRotationType BillboardRenderer::DEFAULT_ROTATION_TYPE = Ogre::v1::BBR_TEXCOORD;
	const Vector3 BillboardRenderer::DEFAULT_COMMON_DIRECTION(0, 0, 1);
	const Vector3 BillboardRenderer::DEFAULT_COMMON_UP_VECTOR(0, 1, 0);
	const bool BillboardRenderer::DEFAULT_POINT_RENDERING = false;

	//-----------------------------------------------------------------------
	BillboardRenderer::BillboardRenderer() :
		ParticleRenderer(),
		mBillboardType(DEFAULT_BILLBOARD_TYPE),
		mBillboardOrigin(Ogre::v1::BBO_CENTER),
		mBillboardRotationType(Ogre::v1::BBR_TEXCOORD),
		mBillboardUpVector(Vector3::UNIT_Y),
		mBillboardDirection(Vector3::UNIT_Z),
		mPointRenderingEnabled(false),
		mUseAccurateFacing(false),
		mBillboardSet(0)
	{
		autoRotate = false;
	}
	//-----------------------------------------------------------------------
	BillboardRenderer::~BillboardRenderer(void)
	{
		if (mBillboardSet)
		{
			PU_DELETE mBillboardSet;
			mBillboardSet = 0;
		}
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_prepare(ParticleTechnique* technique)
	{
		// Use the given technique, although it should be the same as mParentTechnique (must be set already)
		if (!technique || mRendererInitialised)
			return;

		mBillboardSet = PU_NEW Ogre::v1::BillboardSet(Ogre::Id::generateNewId<ParticleRenderer>(),
											technique->getParentSystem()->getDummyObjectMemMgr(),
											technique->getParentSystem()->_getManager(),
											0, true);
		mBillboardSet->setBillboardsInWorldSpace(true);

		setBillboardType(mBillboardType);
		setUseAccurateFacing(mUseAccurateFacing);
		setBillboardOrigin(mBillboardOrigin);
		setBillboardRotationType(mBillboardRotationType);
		setCommonDirection(mBillboardDirection);
		setCommonUpVector(mBillboardUpVector);
		setPointRenderingEnabled(mPointRenderingEnabled);

		_notifyParticleQuota(technique->getVisualParticleQuota());

		// Notify attached, but only if the parentnode exists
		if (technique->getParentSystem()->getParentNode())
		{		
			_notifyAttached(technique->getParentSystem()->getParentNode());
		}
		_notifyDefaultDimensions(_mRendererScale.x * technique->getDefaultWidth(),
								_mRendererScale.y * technique->getDefaultHeight(),
								_mRendererScale.z * technique->getDefaultDepth());
		_setMaterialName(technique->getMaterialName());
		mBillboardSet->setRenderQueueGroup(mQueueId);
		
		// Set the texture coordinates (if used)
		if (mTextureCoordsSet)
		{
			mBillboardSet->setTextureCoords(*mUVList.begin(), static_cast<uint16>(mUVList.size()));
		}
		else if (mTextureCoordsRowsAndColumnsSet)
		{
			mBillboardSet->setTextureStacksAndSlices(mTextureCoordsRows, mTextureCoordsColumns);
		}
		
		mRendererInitialised = true;
	}

	/*void BillboardRenderer::_prepare(ParticleTechnique* technique)
	{
		Ogre::LogManager::getSingleton().stream() << "\n[Particle Universe] Preparing Particle Technique: " + technique->getName();
		 Use the given technique, although it should be the same as mParentTechnique (must be set already)
		if (!technique || mRendererInitialised)
			return;


		mBillboardSet = PU_NEW Ogre::v1::BillboardSet(Ogre::Id::generateNewId<ParticleRenderer>(),
			technique->getParentSystem()->getDummyObjectMemMgr(),
			technique->getParentSystem()->_getManager(),
			0, true);
		mBillboardSet->setBillboardsInWorldSpace(true);

		 New: Copy the settings from the template if it exists
		ParticleSystem* pTemplate = ParticleSystemManager::getSingletonPtr()->getParticleSystemTemplate(technique->getParentSystem()->getTemplateName());
		if (pTemplate)
		{
			for (int i = 0; i < pTemplate->getNumTechniques(); i++)
			{
				if (technique->getName() == pTemplate->getTechnique(i)->getName())
				{
					ParticleUniverse::String rendererType = pTemplate->getTechnique(i)->getRenderer()->getRendererType();
					if (rendererType == getRendererType())
					{
						pTemplate->getTechnique(i)->getRenderer()->copyAttributesTo(this);
					}
				}
			}
		}

		Now set the properties which may have been restored from the template.
		setBillboardType(mBillboardType);
		setUseAccurateFacing(mUseAccurateFacing);
		setBillboardOrigin(mBillboardOrigin);
		setBillboardRotationType(mBillboardRotationType);
		setCommonDirection(mBillboardDirection);
		setCommonUpVector(mBillboardUpVector);
		setPointRenderingEnabled(mPointRenderingEnabled);

		_notifyParticleQuota(technique->getVisualParticleQuota());

		 Notify attached, but only if the parentnode exists
		if (technique->getParentSystem()->getParentNode())
		{
			_notifyAttached(technique->getParentSystem()->getParentNode());
		}
		_notifyDefaultDimensions(_mRendererScale.x * technique->getDefaultWidth(),
			_mRendererScale.y * technique->getDefaultHeight(),
			_mRendererScale.z * technique->getDefaultDepth());
		_setMaterialName(technique->getMaterialName());
		mBillboardSet->setRenderQueueGroup(mQueueId);

		 Set the texture coordinates (if used)
		if (mTextureCoordsSet)
		{
			mBillboardSet->setTextureCoords(*mUVList.begin(), static_cast<uint16>(mUVList.size()));
		}
		else if (mTextureCoordsRowsAndColumnsSet)
		{
			mBillboardSet->setTextureStacksAndSlices(mTextureCoordsRows, mTextureCoordsColumns);
		}

		mRendererInitialised = true;
	}*/
	//-----------------------------------------------------------------------
	void BillboardRenderer::_unprepare(ParticleTechnique* technique)
	{
		_notifyAttached(0); // Bugfix V1.5: If detached from scenenode, do not use the pointer to it
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setBillboardType(BillboardType bbt)
	{
		mBillboardType = bbt;
		if (mBillboardSet)
		{
			// Because BBT_ORIENTED_SHAPE is unknown to Ogre, this switch is needed
			switch (bbt)
			{
				case BBT_POINT:
					mBillboardSet->setBillboardType(Ogre::v1::BBT_POINT);
					break;

				case BBT_ORIENTED_COMMON:
					mBillboardSet->setBillboardType(Ogre::v1::BBT_ORIENTED_COMMON);
					break;

				case BBT_ORIENTED_SELF:
					mBillboardSet->setBillboardType(Ogre::v1::BBT_ORIENTED_SELF);
					break;

				case BBT_ORIENTED_SHAPE:
					mBillboardSet->setBillboardType(Ogre::v1::BBT_ORIENTED_SELF);
					break;

				case BBT_PERPENDICULAR_COMMON:
					mBillboardSet->setBillboardType(Ogre::v1::BBT_PERPENDICULAR_COMMON);
					break;

				case BBT_PERPENDICULAR_SELF:
					mBillboardSet->setBillboardType(Ogre::v1::BBT_PERPENDICULAR_SELF);
					break;
			}
		}
	}
	//-----------------------------------------------------------------------
	BillboardRenderer::BillboardType BillboardRenderer::getBillboardType(void) const
	{
		return mBillboardType;
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setUseAccurateFacing(bool acc)
	{
		mUseAccurateFacing = acc;
		if (mBillboardSet)
		{
			mBillboardSet->setUseAccurateFacing(acc);
		}
	}
	//-----------------------------------------------------------------------
	bool BillboardRenderer::isUseAccurateFacing(void) const
	{
		return mUseAccurateFacing;
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setBillboardOrigin(Ogre::v1::BillboardOrigin origin)
	{
		mBillboardOrigin = origin;
		if (mBillboardSet)
		{
			mBillboardSet->setBillboardOrigin(origin);
		}
	}
	//-----------------------------------------------------------------------
	Ogre::v1::BillboardOrigin BillboardRenderer::getBillboardOrigin(void) const
	{
		return mBillboardOrigin;
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setBillboardRotationType(Ogre::v1::BillboardRotationType rotationType)
	{
		mBillboardRotationType = rotationType;
		if (mBillboardSet)
		{
			mBillboardSet->setBillboardRotationType(rotationType);
		}
	}
	//-----------------------------------------------------------------------
	Ogre::v1::BillboardRotationType BillboardRenderer::getBillboardRotationType(void) const
	{
		return mBillboardRotationType;
	}
    //-----------------------------------------------------------------------
	void BillboardRenderer::setCommonDirection(const Vector3& vec)
	{
		mBillboardDirection = vec;
		if (mBillboardSet)
		{
			mBillboardSet->setCommonDirection(vec);
		}
	}
	//-----------------------------------------------------------------------
	const Vector3& BillboardRenderer::getCommonDirection(void) const
	{
		return mBillboardDirection;
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setCommonUpVector(const Vector3& vec)
	{
		mBillboardUpVector = vec;
		if (mBillboardSet)
		{
			mBillboardSet->setCommonUpVector(vec);
		}
	}
	//-----------------------------------------------------------------------
	const Vector3& BillboardRenderer::getCommonUpVector(void) const
	{
		return mBillboardUpVector;
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setPointRenderingEnabled(bool enabled)
	{
		mPointRenderingEnabled = enabled;
		if (mBillboardSet)
		{
			mBillboardSet->setPointRenderingEnabled(enabled);
		}
	}
	//-----------------------------------------------------------------------
	bool BillboardRenderer::isPointRenderingEnabled(void) const
	{
		return mPointRenderingEnabled;
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_updateRenderQueue(Ogre::RenderQueue* queue, Ogre::Camera* camera, const Ogre::Camera* lodCamera, ParticlePool* pool)
	{
		// Always perform this one
		ParticleRenderer::_updateRenderQueue(queue, camera, lodCamera, pool);

		if (!mVisible)
			return;

		// Fast check to determine whether there are visual particles
		if (pool->isEmpty(Particle::PT_VISUAL))
			return;

		mBillboardSet->_notifyCurrentCamera(camera, lodCamera);

		mBillboardSet->setCullIndividually(mCullIndividual);

		mBillboardSet->beginBillboards(pool->getSize(Particle::PT_VISUAL));
		Billboard bb; // This is the Particle Universe Billboard and not the Ogre Billboard
		
		VisualParticle* particle = static_cast<VisualParticle*>(pool->getFirst(Particle::PT_VISUAL));
		while (!pool->end(Particle::PT_VISUAL))
		{
			if (particle)
			{
				bb.mPosition = particle->position;

				if (mBillboardType == BBT_ORIENTED_SELF || mBillboardType == BBT_PERPENDICULAR_SELF)
				{
					// Normalise direction vector
					bb.mDirection = particle->direction;
					bb.mDirection.normalise();
				}
				else if (mBillboardType == BBT_ORIENTED_SHAPE)
				{
					// Normalise orientation vector
					bb.mDirection = Vector3(particle->orientation.x, particle->orientation.y, particle->orientation.z);
					bb.mDirection.normalise();
				}
					
				bb.mColour = particle->colour;
				bb.mRotation = particle->zRotation; // Use rotation around the Z-axis (2D rotation)

				if (bb.mOwnDimensions = particle->ownDimensions)
				{
					bb.mOwnDimensions = true;
					bb.mWidth = particle->width;
					bb.mHeight = particle->height;
				}

				// PU 1.4: No validation on max. texture coordinate because of performance reasons.
				bb.setTexcoordIndex(particle->textureCoordsCurrent);
					
				mBillboardSet->injectBillboard(bb, camera);
			}
			
			particle = static_cast<VisualParticle*>(pool->getNext(Particle::PT_VISUAL));
		}

        mBillboardSet->endBillboards();

		//add this billboard set to renderables so Ogre renders it
		this->getParentTechnique()->getParentSystem()->mRenderables.push_back(mBillboardSet);
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_notifyAttached(Ogre::Node* parent)
	{
		mBillboardSet->_notifyAttached(parent);
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_setMaterialName(const String& materialName)
	{
		if (mBillboardSet)
		{
			mBillboardSet->setDatablockOrMaterialName(materialName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
		}
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_notifyCurrentCamera(Camera* cam, const Ogre::Camera* lodCamera)
	{
		mBillboardSet->_notifyCurrentCamera(cam, lodCamera);
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_notifyParticleQuota(size_t quota)
	{
		mBillboardSet->setPoolSize(static_cast<unsigned int>(quota));
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_notifyDefaultDimensions(Real width, Real height, Real depth)
	{
		if (mBillboardSet)
		{
			// Ignore the depth, because the billboard is flat.
			mBillboardSet->setDefaultDimensions(width, height);
		}
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_notifyParticleResized(void)
	{
		mBillboardSet->_notifyBillboardResized();
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::_notifyParticleZRotated(void)
	{
		mBillboardSet->_notifyBillboardRotated();
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setRenderQueueGroup(uint8 queueId)
	{
		mQueueId = queueId;
		if (mBillboardSet)
		{
			mBillboardSet->setRenderQueueGroup(mQueueId);
		}
	}
	//-----------------------------------------------------------------------
	SortMode BillboardRenderer::_getSortMode(void) const
	{
		return mBillboardSet->_getSortMode();
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::setVisible(bool visible)
	{
		ParticleRenderer::setVisible(visible);
	}
	//-----------------------------------------------------------------------
	void BillboardRenderer::copyAttributesTo (ParticleRenderer* renderer)
	{
		// First copy parent attributes
		ParticleRenderer::copyAttributesTo(renderer);

		// First cast to BillboardRenderer
		BillboardRenderer* billboardRenderer = static_cast<BillboardRenderer*>(renderer);

		// Copy attributes in case there is a billboardset (which should be available)
		if (!billboardRenderer->getBillboardSet())
			return;

		billboardRenderer->setBillboardType(getBillboardType());
		billboardRenderer->setBillboardOrigin(getBillboardOrigin());
		billboardRenderer->setBillboardRotationType(getBillboardRotationType());
		billboardRenderer->setCommonDirection(getCommonDirection());
		billboardRenderer->setCommonUpVector(getCommonUpVector());
		billboardRenderer->setPointRenderingEnabled(isPointRenderingEnabled());
		billboardRenderer->setUseAccurateFacing(isUseAccurateFacing());
	}

}
