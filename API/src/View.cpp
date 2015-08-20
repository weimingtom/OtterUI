#include <string.h>
#include <assert.h>
#include <algorithm>

#include "View.h"
#include "Sprite.h"
#include "Label.h"
#include "Button.h"
#include "Group.h"
#include "Scene.h"
#include "Table.h"
#include "Toggle.h"
#include "Slider.h"
#include "Mask.h"
#include "Common/Log.h"
#include "Interfaces/ISoundSystem.h"
#include "Data/OtterData.h"
#include "Utilities/Utilities.h"
#include "Memory/Memory.h"

namespace Otter
{
	/* Constructor - builds the view from the provided view data
	 */
	View::View(Scene* pScene, const ViewData* pViewData) : Control(pScene, NULL, pViewData)
	{
		mAnimCounter = 2;				// Starts past ANIM_ACTIVATE and ANIM_DEACTIVATE
		mNextControlID = 10000000;		// Hopefully no one will ever have over 1,000,000 controls.. 
		mFinishedAnims.reserve(50);

		LoadFromData(pViewData);
	}

	/* Destructor
	 */
	View::~View(void)
	{
		Unload();
	}

	/* Loads the view from the provided view data
	 */
	bool View::LoadFromData(const ViewData* pViewData)
	{
		Unload();

		if(pViewData == NULL || pViewData->mFourCC != FOURCC_GGVW)
			return false;

		for(uint32 i = 0; i < pViewData->mNumControls; i++)
		{
			const ControlData* pControlData = pViewData->GetControlData(i);

			Control* pControl = CreateControl(pControlData, this);
			if(pControl)
				mControls.push_back(pControl);
		}

		//having created the controls, now point any control with a mask to its mask control
		SetMaskPointers();

		return true;
	}

	/* Unloads and frees the view's internal objects.
	 */
	void View::Unload()
	{		
		while(mControls.size() > 0)
		{
			Otter::Control* pControl = mControls[0];
			RemoveControl(pControl);

			DestroyControl(pControl);
		}

		mControls.clear();
	}

	void View::DestroyControl(Otter::Control* pControl)
	{
		if(!pControl)
			return;

		// First delete children
		while(pControl->GetNumControls() > 0)
		{
			Otter::Control* pChildControl = pControl->GetControlAtIndex(0);
			pControl->RemoveControl(pChildControl);

			DestroyControl(pChildControl);
		}
  
		// Now get rid of the control itself
		if(MemoryManager::isValid(pControl))
		{
			OTTER_DELETE(pControl);
		}
		else
		{
			const Array<IPlugin*>& plugins = mScene->GetSystem()->GetPlugins();
			int cnt = plugins.size();
			for(int j = 0; j < cnt; j++)
			{
				if(plugins[j]->DestroyControl(pControl))
					break;
			}
		}
	}

	/* Creates a control from the provided control data
	 */
	Control* View::CreateControl(const ControlData* pControlData, Control* pParent)
	{
		if(!pControlData)
			return NULL;

		Control* pControl = NULL;

		switch(pControlData->mFourCC)
		{
		// Sprite Control
		case FOURCC_GSPR:
			{
				const SpriteData* pSpriteData = (const SpriteData*)pControlData;
				pControl = OTTER_NEW(Sprite, (mScene, pParent, pSpriteData)); 

				Sprite* pSpriteControl = (Sprite*)pControl;
				for(uint32 i = 0; i < pSpriteData->mNumControls; i++)
				{
					Control* pChildControl = CreateControl(pSpriteData->GetControlData(i), pSpriteControl);
					pSpriteControl->AddControl(pChildControl);
				}
				break;
			}
		// Label Control 
		case FOURCC_GLBL:
			{
				const LabelData* pLabelData = (const LabelData*)pControlData;
				pControl = OTTER_NEW(Label, (mScene, pParent, pLabelData));
				break;
			}
		// Button Control
		case FOURCC_GBTT:
			{
				const ButtonData* pButtonData = (const ButtonData*)pControlData;
				pControl = OTTER_NEW(Button, (mScene, pParent, pButtonData));
				break;
			}
		// Group Control
		case FOURCC_GGRP:
			{
				const GroupData* pGroupData = (const GroupData*)pControlData;
				pControl = OTTER_NEW(Group, (mScene, pParent, pGroupData));

				Group* pGroupControl = (Group*)pControl;
				for(uint32 i = 0; i < pGroupData->mNumControls; i++)
				{
					Control* pChildControl = CreateControl(pGroupData->GetControlData(i), pGroupControl);
					pGroupControl->AddControl(pChildControl);
				}
				break;
			}
		// Table Control
		case FOURCC_GTBL:
			{
				const TableData* pTableData = (const TableData*)pControlData;
				pControl = OTTER_NEW(Table, (mScene, pParent, pTableData));
				break;
			}
		// Toggle Control
		case FOURCC_GTGL:
			{
				const ToggleData* pToggleData = (const ToggleData*)pControlData;
				pControl = OTTER_NEW(Toggle, (mScene, pParent, pToggleData));
				break;
			}
		// Slider Control
		case FOURCC_GSLD:
			{
				const SliderData* pSliderData = (const SliderData*)pControlData;
				pControl = OTTER_NEW(Slider, (mScene, pParent, pSliderData));
				break;
			}
		// Mask Control
		case FOURCC_GMSK:
			{
				const MaskData* pMaskData = (const MaskData*)pControlData;
				pControl = OTTER_NEW(Mask, (mScene, pParent, pMaskData));
				break;
			}
		// Unknown Control.  Defer to any plugins to load the control.
		default:
			{
				const Array<IPlugin*>& plugins = mScene->GetSystem()->GetPlugins();
				int cnt = plugins.size();
				for(int i = 0; i < cnt; i++)
				{
					pControl = plugins[i]->CreateControl(mScene, pControlData, pParent);
					if(pControl)
						break;
				}
				break;
			}
		}

		return pControl;
	}

	/**
	 * Retrieves a control by name.  If animInstanceID is non-zero,
	 * this function will search only against controls within a specific active animation
	 */
	Control* View::GetControlInAnimation(uint32 animInstanceID, const char* szControlName)
	{
		ActiveAnimation* pActiveAnim = GetActiveAnimation(animInstanceID);
		if(!pActiveAnim || pActiveAnim->mControlRemap.size() == 0)
			return NULL;

		const ViewData* pData = static_cast<const ViewData*>(GetData());
		if(!pData)
			return NULL;

		const AnimationListData* pAnimListData = pData->GetAnimationListData();
		if(!pAnimListData)
			return NULL;
		
		const AnimationData* pAnimData = pAnimListData->GetAnimation(pActiveAnim->mAnimationIndex);
		if(!pAnimData)
			return NULL;

		for(uint32 i = 0; i < pAnimData->mNumAnimationChannels; i++)
			if( Control * pControl = GetControl((uint32)pActiveAnim->mControlRemap[i])->GetControl(szControlName) )
				return pControl;

		return NULL;
	}

	/* Called whenever this view is activated
	 */
	void View::OnActivate()
	{
		for(uint32 i = 0; i < mControls.size(); i++)
		{
			Control* pControl = mControls[i];
			pControl->OnActivate();
		}
		
		mActiveAnimations.clear();
		mOnActivate(this, NULL);
	}

	/* Called whenever this view is deactivated
	 */
	void View::OnDeactivate()
	{
		while( mActiveAnimations.size() )
			StopAnimation(mActiveAnimations[0].mID);
		for(uint32 i = 0; i < mControls.size(); i++)
		{
			Control* pControl = mControls[i];
			pControl->OnDeactivate();
		}
		mOnDeactivate(this, NULL);
	}

	/** 
	 * Creates an animation instance.  We assume the ActiveAnimation struct
	 * has already been filled with valid data, and we need to duplicate controls
	 * and set up the control id remaps.
	 */
	void View::CreateAnimationInstance(ActiveAnimation& activeAnimation)
	{
		const ViewData* pData = static_cast<const ViewData*>(GetData());
		if(!pData)
			return;

		const AnimationListData* pAnimListData = pData->GetAnimationListData();
		if(!pAnimListData)
			return;
		
		const AnimationData* pAnimData = pAnimListData->GetAnimation(activeAnimation.mAnimationIndex);
		if(!pAnimData)
			return;

		assert(activeAnimation.mControlRemap.size()==0);
		activeAnimation.mControlRemap.resize(pAnimData->mNumAnimationChannels);
		if(LogIsEnabled() && !activeAnimation.mControlRemap.size())
			Otter::LogInfo("Scene %d, View '%s' : Animation [ID: %u] -- ZERO animation channels", GetScene()->GetID(), GetName(), activeAnimation.mID);
		Array<Control *> & roots = activeAnimation.mClonedRoots;
		assert(roots.size()==0);
		for( int i=0,n=pAnimData->mNumAnimationChannels; i!=n; ++i )
		{
			AnimationChannelData const * ch = pAnimData->GetAnimationChannel(i);
			Control * r=0;
			for( Control * c=GetControl((uint32)ch->mControlID); c!=this; c=c->GetParentControl() )
				r=c;
			assert(r!=0);
			if( std::find(&roots[0],&roots[0]+roots.size(),r)==&roots[0]+roots.size() )
				roots.push_back(r);
		}

		for( Control * * i=&roots[0],* * e=i+roots.size(); i!=e; ++i )
			*i = (*i)->Clone();

		for( int i=0,n=pAnimData->mNumAnimationChannels; i!=n; ++i )
		{
			char const * const name=GetControl(pAnimData->GetAnimationChannel(i)->mControlID)->GetName();
			for( Control * * j=&roots[0],* * e=j+roots.size(); j!=e; ++j )
				if( Control * c=(*j)->GetControl(name) )
					activeAnimation.mControlRemap[i] = c->GetID();
		}

		for( Control* * i=&roots[0],* * e=i+roots.size(); i!=e; ++i )
			(*i)->RemapMasks(this,*i);
	}


/*
	void View::CreateAnimationInstance(ActiveAnimation& activeAnimation)
	{
		const ViewData* pData = static_cast<const ViewData*>(GetData());
		if(!pData)
			return;

		const AnimationListData* pAnimListData = pData->GetAnimationListData();
		if(!pAnimListData)
			return;
		
		const AnimationData* pAnimData = pAnimListData->GetAnimation(activeAnimation.mAnimationIndex);
		if(!pAnimData)
			return;

		assert(activeAnimation.mControlRemap==0);
		activeAnimation.mControlRemap = (sint32*)OTTER_ALLOC(sizeof(sint32) * pAnimData->mNumAnimationChannels);
#ifdef _DEBUG
		std::fill(activeAnimation.mControlRemap,activeAnimation.mControlRemap+pAnimData->mNumAnimationChannels,-1);
#endif

		for(uint32 j = 0; j < pAnimData->mNumAnimationChannels; j++)
		{
			const AnimationChannelData* pChannel = pAnimData->GetAnimationChannel(j);

			Control* pControl = GetControl((uint32)pChannel->mControlID);

			Control* pParent = pControl->GetParentControl();
			int ancestorChannel = -1;
			bool bDone = false;
			while(pParent && !bDone)
			{
				for(uint32 k = 0; k < pAnimData->mNumAnimationChannels; k++)
				{
					const AnimationChannelData* pChl = pAnimData->GetAnimationChannel(k);
					Control* pTmpControl = GetControl((uint32)pChl->mControlID);

					if(pTmpControl == pParent)
					{
						ancestorChannel = k;
						bDone = true;
						break;
					}
				}

				if(!bDone)
					pParent = pParent->GetParentControl();
			}

			// If this control has a parent in the current animation, just find the clone
			// and be done with it.
			if(ancestorChannel != -1)
			{				
				assert(activeAnimation.mControlRemap[ancestorChannel]!=-1);
				Control* pClonedAncestor = this->GetControl(activeAnimation.mControlRemap[ancestorChannel]);
				Control* pMyClone = pClonedAncestor->GetControl(pControl->GetName());
				if( pMyClone )
					{
					assert(pMyClone!=0);
					assert(activeAnimation.mControlRemap[j]==-1);
					activeAnimation.mControlRemap[j] = pMyClone->GetID();
					}
			}
			else
			{				
				assert(mNextControlID!=-1);
//				assert(!GetControl(mNextControlID));
				activeAnimation.mControlRemap[j] = mNextControlID;

				Control* pNewControl = pControl->Clone();
				((ControlData*)pNewControl->GetData())->mID = mNextControlID;

				++mNextControlID;
			}
		}
	}
*/


	/** 
	 * Destroys an animation instance
	 */
	void View::DestroyAnimationInstance(ActiveAnimation& activeAnimation)
	{
		const ViewData* pData = static_cast<const ViewData*>(GetData());
		if(!pData)
			return;

		const AnimationListData* pAnimListData = pData->GetAnimationListData();
		if(!pAnimListData)
			return;
		
		const AnimationData* pAnimData = pAnimListData->GetAnimation(activeAnimation.mAnimationIndex);
		if(!pAnimData)
			return;

		Array<Control *> & roots = activeAnimation.mClonedRoots;
		for( Control * * i=&roots[0],* * e=i+roots.size(); i!=e; ++i )
			{
			Result r=RemoveControl(*i);
			assert(r==kResult_OK);
			OTTER_DELETE(*i);
			}
		roots.clear(true);
		activeAnimation.mControlRemap.clear(true);
	}

	/* Retrieves an active animation by ID.  Returns NULL if not found
	 */
	ActiveAnimation* View::GetActiveAnimation(uint32 animID)
	{
		if((animID & 0xF0000000) != 0xF0000000)
			return NULL;
	
		for(uint32 i = 0; i < mActiveAnimations.size(); i++)
		{
			ActiveAnimation& anim = mActiveAnimations[i];
			if(anim.mID == animID)
				return &anim;
		}
		
		return NULL;
	}

	AnimationData const * View::GetAnimationData( char const * name ) const
	{
		assert(name&&*name);
		const ViewData* pData = static_cast<const ViewData*>(const_cast<View *>(this)->GetData());
		if(!pData)
			return 0;

		const AnimationListData* pAnimListData = pData->GetAnimationListData();
		if(!pAnimListData)
			return 0;
		
		for(uint32 i = 0; i < pAnimListData->mNumAnimations; i++)
		{
			const AnimationData* pAnimData = pAnimListData->GetAnimation(i);
			if(!pAnimData)
				continue;			
			if(strcmp((const char*)pAnimData->mName, name) == 0 && pAnimData->mNumFrames > 0)
				return pAnimData;
		}

		return 0;
	}

	bool IsLoopingAnimation( AnimationData const & ad )
	{
		return ad.mRepeatStart!=-1 && ad.mRepeatEnd!=-1;
	}
	
	/* Plays an animation by name
	 */
	uint32 View::PlayAnimation(const char* szName, uint32 startFrame, uint32 endFrame, bool bReverse, VectorMath::Matrix4 const & transform, bool bMakeInstance)
	{
		const ViewData* pData = static_cast<const ViewData*>(GetData());
		if(!pData)
			return 0;

		const AnimationListData* pAnimListData = pData->GetAnimationListData();
		if(!pAnimListData)
			return 0;
		
		for(uint32 i = 0; i < pAnimListData->mNumAnimations; i++)
		{
			const AnimationData* pAnimData = pAnimListData->GetAnimation(i);
			if(!pAnimData)
				continue;			
			
			if(strcmp((const char*)pAnimData->mName, szName) == 0 && pAnimData->mNumFrames > 0)
			{
				return PlayAnimation(i, startFrame, endFrame, bReverse, transform, bMakeInstance);
			}
		}

		return 0;
	}

	/* Plays an animation by index
	 */
	uint32 View::PlayAnimation(uint32 index, uint32 startFrame, uint32 endFrame, bool bReverse, VectorMath::Matrix4 const & transform, bool bMakeInstance)
	{
		const ViewData* pData = static_cast<const ViewData*>(GetData());
		if(!pData)
			return 0;

		const AnimationListData* pAnimListData = pData->GetAnimationListData();
		if(!pAnimListData)
			return 0;

		const AnimationData* pAnimData = pAnimListData->GetAnimation(index);
		if(!pAnimData)
			return 0;

		// Go from 1 to 0-based
		startFrame -= 1;
		endFrame -= 1;

		float startTime = bReverse ? ((float)pAnimData->mNumFrames - 1.0f) : 0.0f;
		float endTime = bReverse ? 0.0f : ((float)pAnimData->mNumFrames - 1.0f);
		bool bKeyOff = false;

		if(startFrame != 0xFFFFFFFF)
		{
			startTime = (float)startFrame;
			startTime = Clamp(startTime, 0.0f, (float)pAnimData->mNumFrames - 1.0f);

			if(!bReverse && (uint32)pAnimData->mRepeatEnd != 0xFFFFFFFF && startFrame > (uint32)pAnimData->mRepeatEnd ||
				bReverse && (uint32)pAnimData->mRepeatStart != 0xFFFFFFFF && startFrame < (uint32)pAnimData->mRepeatStart)
			{
				bKeyOff = true;
			}
		}

		if(endFrame != 0xFFFFFFFF)
		{
			endTime = (float)(endFrame);

			if(bReverse)
				endTime = Clamp(endTime, 0.0f, startTime);
			else
				endTime = Clamp(endTime, startTime, ((float)pAnimData->mNumFrames - 1.0f));
		}

		ActiveAnimation activeAnimation;
		activeAnimation.mAnimationIndex = index;
		activeAnimation.mFrameTime = startTime;
		activeAnimation.mStartFrameTime = startTime;
		activeAnimation.mEndFrameTime = endTime;
		activeAnimation.mKeyOff = bKeyOff;
		activeAnimation.mID = 0xF0000000 | (++mAnimCounter);
		activeAnimation.mReverse = bReverse;
		activeAnimation.mTransform = transform;

		if(strcmp((const char*)pAnimData->mName, "OnActivate") == 0)
		{
			activeAnimation.mID = ANIM_ONACTIVATE;
		}
		else if(strcmp((const char*)pAnimData->mName, "OnDeactivate") == 0)
		{
			activeAnimation.mID = ANIM_ONDEACTIVATE;
		}

		if(bMakeInstance)
		{
			CreateAnimationInstance(activeAnimation);
		}

		AnimateControls(pAnimData, activeAnimation);

		mActiveAnimations.push_back(activeAnimation);	
		mOnAnimationStarted(this, activeAnimation.mID);

		if(LogIsEnabled())
		{
			Otter::LogInfo("View '%s' : Animation '%s' [ID: %u] -- STARTED", GetName(), GetActiveAnimationName(activeAnimation.mID), activeAnimation.mID);
		}
				
		return activeAnimation.mID;
	}

	/* Stops an active animation by name
	 */
	Result View::StopAnimation(uint32 animID)
	{
		for(uint32 i = 0; i < mActiveAnimations.size(); i++)
		{
			if(mActiveAnimations[i].mID == animID)
			{
				if(LogIsEnabled())
				{
					Otter::LogInfo("View '%s' : Animation '%s' [ID: %u] -- ENDED (manual stop)", GetName(), GetActiveAnimationName(mActiveAnimations[i].mID), mActiveAnimations[i].mID);
				}

				DestroyAnimationInstance(mActiveAnimations[i]);

				mActiveAnimations.erase(i);
				mOnAnimationEnded(this, animID);
				return kResult_OK;
			}
		}

		return kResult_AnimationNotFound;
	}

	/* Flags an animation to run past the looping section.
	 */
	Result View::KeyOffAnimation(uint32 animID)
	{ 
		ActiveAnimation* pActiveAnim = GetActiveAnimation(animID);
		if(!pActiveAnim)
			return kResult_AnimationNotFound;
			
		pActiveAnim->mKeyOff = true;

		return kResult_OK;
	}

	/** 
	 * Retrieves an active animation's name by its unique animation ID
	 */
	const char* View::GetActiveAnimationName(uint32 animID)
	{
		const ViewData* pData = static_cast<const ViewData*>(GetData());
		if(!pData)
			return NULL;
		
		const AnimationListData* pAnimListData = pData->GetAnimationListData();
		if(!pAnimListData)
			return NULL;

		uint32 animIndex = 0xFFFFFFFF;

		for(uint32 i = 0; i < mActiveAnimations.size(); i++)
		{
			if(mActiveAnimations[i].mID == animID)
			{
				animIndex = mActiveAnimations[i].mAnimationIndex;
				break;
			}
		}

		if(animIndex != 0xFFFFFFFF)
		{
			const AnimationData* pAnimation = pAnimListData->GetAnimation(animIndex);
			if(!pAnimation)
				return NULL;

			return (const char*)pAnimation->mName;
		}

		return NULL;
	}
	
	/* Retrieves the list of active/playing animations by name
	 */
	Array<uint32> View::GetActiveAnimations(const char* szName)
	{
		Array<uint32> anims;

		const ViewData* pData = static_cast<const ViewData*>(GetData());
		if(!pData)
			return anims;
		
		const AnimationListData* pAnimListData = pData->GetAnimationListData();
		if(!pAnimListData)
			return anims;
		
		uint32 animIndex = 0;
		for(uint32 i = 0; i < pAnimListData->mNumAnimations; i++)
		{
			const AnimationData* pAnimData = pAnimListData->GetAnimation(i);
			if(!pAnimData)
				continue;
			
			if(strcmp((const char*)pAnimData->mName, szName) == 0)
			{
				animIndex = i;
				break;
			}
		}
		
		if(animIndex)
		{
			for(uint32 i = 0; i < mActiveAnimations.size(); i++)
			{
				if(mActiveAnimations[i].mAnimationIndex == animIndex)
					anims.push_back(mActiveAnimations[i].mID);
			}
		}
		
		return anims;
	}	

	bool View::SetActiveAnimationTransform( uint32 animID, VectorMath::Matrix4 const & transform )
	{
		if( ActiveAnimation * a=GetActiveAnimation(animID) )
		{
			a->mTransform=transform;
			return true;
		}
		else
			return false;
	}

	/** 
	 * Retrieves the 1-based frame index of a named main channel frame of the
	 * specified animation
	 */
	uint32 View::GetFrameIndex(const char* szAnimationName, const char* szFrameName)
	{
		const ViewData* pData = static_cast<const ViewData*>(GetData());
		if(!pData)
			return 0;

		const AnimationListData* pAnimListData = pData->GetAnimationListData();
		if(!pAnimListData)
			return 0;
		
		for(uint32 i = 0; i < pAnimListData->mNumAnimations; i++)
		{
			const AnimationData* pAnimData = pAnimListData->GetAnimation(i);
			if(!pAnimData)
				continue;			
			
			if(strcmp((const char*)pAnimData->mName, szAnimationName) == 0)
			{
				for(uint32 j = 0; j < pAnimData->mNumFrames; j++)
				{
					const MainChannelFrameData* pMainChannelFrameData = pAnimData->GetMainChannelFrameData(j);
					if(pMainChannelFrameData && strcmp(pMainChannelFrameData->mName, szFrameName) == 0)
					{
						return (pMainChannelFrameData->mFrame + 1);
					}
				}
			}
		}

		return 0;
	}
	
	/* Points (touches/mouse/etc) were pressed down
	 */
	bool View::OnPointsDown(const Point* points, sint32 numPoints)
	{
		assert(numPoints>=0);
		if(!mTouchEnabled)
			return false;
		if(!mEnabled)
			return false;
		bool handled = false;
		for(int i = 0; i!=numPoints; ++i)
		{
			Point localPoint;
			if( Control* pControl = GetControl(points[i], &localPoint, true) )
				if( pControl!=this && pControl->OnPointsDown(&localPoint, 1) )
				{
					handled = true;
					mTouchedControls.push_back(pControl);
				}
		}
		return handled;
	}
	
	/* Points (touches/mouse/etc) were released
	 * Only controls that were previously "touched" get the up 
	 * notification
	 */
	bool View::OnPointsUp(const Point* points, sint32 numPoints)
	{
		assert(numPoints>=0);
		if(!mTouchEnabled)
			return false;
		if(!mEnabled)
			return false;
		ControlArray tmp;
		mTouchedControls.Swap(tmp);
		bool handled = false;
		for(int i = 0; i!=numPoints; ++i)
		{
			Point localPoint;
			if( Control* pControl = GetControl(points[i], &localPoint, true) )
				for( int j=0,n=tmp.size(); j!=n; ++j )
					if( pControl==tmp[j] )
					{
						tmp[j]=0;
						handled = handled || pControl->OnPointsUp(&localPoint, 1);
					}
		}
		for(int i = 0; i!=numPoints; ++i)
			for( int j=0,n=tmp.size(); j!=n; ++j )
				if( Control * c=tmp[j] )
				{
					Point localPoint;
					c->ScreenToLocal(points[i], localPoint);
					(void) c->OnPointsUpCancel(&localPoint,1);
				}
		return handled;
	}
	
	/* Points (touches/mouse/etc) were moved.
	 * Only controls that were previously "touched" get the move
	 * notification
	 */
	bool View::OnPointsMove(const Point* points, sint32 numPoints)
	{
		assert(numPoints>=0);
		if(!mTouchEnabled)
			return false;
		if(!mEnabled)
			return false;
		bool handled = false;
		for(uint32 c = 0; c < mTouchedControls.size(); c++)
		{
			Control* pControl = mTouchedControls[c];
			for(int i = 0; i!=numPoints; ++i)
			{
				Point localPoint;			
				pControl->ScreenToLocal(points[i], localPoint);
				handled = handled || pControl->OnPointsMove(&localPoint, 1);
			}
		}														  

		return handled;
	}

	/* Processes an action
	 */
	void View::ProcessAction(Control* pSender, const ActionData* pAction)
	{
		switch(pAction->mFourCC)
		{
		case FOURCC_MSGA:
			{
				const MessageActionData* messageAction = (const MessageActionData*)pAction;
				const MessageData* messageData = GetScene()->GetData()->GetMessageData(messageAction->mMessageID);

				if(!messageData)
					return;

				MessageArgs messageArgs;
				messageArgs.mMessageID = messageData->mID;

				strncpy_s(messageArgs.mText, 64, messageData->mText, 64);
				messageArgs.mText[63] = 0;

				mOnMessage(pSender, messageArgs);
				System & s=*GetScene()->GetSystem();
				s.mOnMessage(&s,messageArgs);

				break;
			}
		case FOURCC_SNDA:
			{
				if(mScene->GetSoundSystem() == NULL)
					break;

				const SoundActionData* soundAction = (const SoundActionData*)pAction;
				const SoundData* soundData = GetScene()->GetData()->GetSound(soundAction->mSoundID);

				if(!soundData)
					return;

				mScene->GetSoundSystem()->OnPlaySound(soundData->mSoundID, soundAction->mVolume);							
				break;
			}
		}
	}

	/* Draws the view.  Note that we draw from the back of the list to the front.
	 * That is because the front of the controls list is actually the topmost control. 
	 */
	void View::Draw(Graphics* pGraphics)
	{		
		if(!mEnabled)
			return;		
		if( int const ncontrols=mControls.size() )
		{
			pGraphics->PushMatrix(GetTransform());
			for(int i = 0; i !=ncontrols; i++)
			{
				Control* pControl = mControls[i];
				if( Mask* pMask = pControl->GetMaskControl() )
					pMask->DrawMask(pGraphics);
				pControl->Draw(pGraphics);
			}
			pGraphics->PopMatrix();
		}
	}	
		
	/* Processes and fires the events between the start and end frame times
	 */
	void View::ProcessEvents(float startFrameTime, float endFrameTime, const ActiveAnimation* pActiveAnimation, const AnimationData* pAnimation)
	{
		if(!pAnimation)
			return;

		// Cycle through all elapsed frames and fire off the appropriate events
		sint32 curr = (sint32)startFrameTime;
		sint32 end =  (sint32)endFrameTime;

		do
		{
			const MainChannelFrameData* pMainChannelFrame = pAnimation->GetMainChannelFrameData(curr);
			if(pMainChannelFrame)
			{
				for(uint32 a = 0; a < pMainChannelFrame->mNumActions; a++)
				{
					const ActionData* pAction = pMainChannelFrame->GetAction(a);

					ProcessAction(this, pAction);
				}
			}

			curr = curr + (pActiveAnimation->mReverse ? -1 : 1);
			if(pAnimation->mRepeatStart != -1 && pAnimation->mRepeatEnd != -1)
			{
				if(!pActiveAnimation->mKeyOff)
				{
					if(pActiveAnimation->mReverse && (curr < pAnimation->mRepeatStart))
						curr = pAnimation->mRepeatEnd;
					else if(!pActiveAnimation->mReverse && (curr > pAnimation->mRepeatEnd))
						curr = pAnimation->mRepeatStart;					
				}
			}
		}
		while(pActiveAnimation->mReverse && (curr > end) || !pActiveAnimation->mReverse && (curr < end));
	}

	/**
	 * Animates the controls associated with an animation, at the specified frametime
	 */
	void View::AnimateControls(const AnimationData* pAnimation, ActiveAnimation& activeAnim)
	{
		// Now we need to iterate over the animations, and determine the new
		// values for the specified controls.
		for(uint32 i = 0; i < pAnimation->mNumAnimationChannels; i++)
		{
			const AnimationChannelData* pChannel = pAnimation->GetAnimationChannel(i);
			if(!pChannel)
				continue;

			Control* pControl = GetControl(activeAnim.mControlRemap.size()==0 ? pChannel->mControlID : activeAnim.mControlRemap[i]);
			if(!pControl)
				continue;

			// Find the bracketing keyframes
			const KeyFrameData* startKeyFrame = NULL;
			const KeyFrameData* endKeyFrame = NULL;

			for(uint32 k = 0; k < pChannel->mNumKeyFrames; k++)
			{
				const KeyFrameData* keyFrame = pChannel->GetKeyFrameData(k);

				if(!keyFrame || keyFrame->mFrame >= pAnimation->mNumFrames)
					continue;

				if(keyFrame->mFrame <= activeAnim.mFrameTime)
					startKeyFrame = keyFrame;
				else if(keyFrame->mFrame > activeAnim.mFrameTime && endKeyFrame == NULL)
					endKeyFrame = keyFrame;
			}

			if(startKeyFrame == NULL && endKeyFrame == NULL)
				continue;

			float factor = 0.0f;
		
			if(startKeyFrame && endKeyFrame)
				factor = (float)(activeAnim.mFrameTime - startKeyFrame->mFrame) / (float)(endKeyFrame->mFrame - startKeyFrame->mFrame);
		
			factor = factor < 0.0f ? 0.0f : (factor > 1.0f ? 1.0f : factor);
			factor = ((int)(factor * 1000.0f)) / 1000.0f;
			factor = CalculateEase(factor, startKeyFrame->mEaseType, startKeyFrame->mEaseAmount);

			pControl->ApplyKeyFrame(startKeyFrame, endKeyFrame, factor);
			if( pControl->GetParentControl()==this )
				pControl->SetBaseTransform(activeAnim.mTransform);
		}
	}

	/* Updates the view
	 * "frameDelta" indicates the number of frames that have passed since last update.
	 */
	void View::Update(float frameDelta)
	{			
		if(!mEnabled)
			return;
		const ViewData* pData = static_cast<const ViewData*>(GetData());
		const AnimationListData* pAnimListData = pData->GetAnimationListData();		
		if(!pAnimListData)
			return;

		uint32 index = 0;
		mFinishedAnims.clear();
		
		while(index < mActiveAnimations.size())
		{
			ActiveAnimation& activeAnim = mActiveAnimations[index];					
			const AnimationData* pAnimation = pAnimListData->GetAnimation(activeAnim.mAnimationIndex);	
			bool bFinished = false;

			if(pAnimation)
			{				
				float oldFrameTime = activeAnim.mFrameTime;
				activeAnim.mFrameTime += activeAnim.mReverse ? -frameDelta : frameDelta;	

				// Check for looping
				// We need to loop before we determine the actual frame time to prevent accidental animation beyond the loop range.
				if(pAnimation->mRepeatStart != -1 && pAnimation->mRepeatEnd != -1)
				{
					// Only loop if the KeyOff flag has NOT been set
					if(!activeAnim.mKeyOff)
					{
						bool looped=false;
						if(activeAnim.mReverse && activeAnim.mFrameTime < (float)pAnimation->mRepeatStart)
							looped=true, activeAnim.mFrameTime=(float)(pAnimation->mRepeatEnd);
						else if(!activeAnim.mReverse && activeAnim.mFrameTime > (float)pAnimation->mRepeatEnd)
							looped=true, activeAnim.mFrameTime=(float)(pAnimation->mRepeatStart);
						if( looped )
							mOnAnimationLooped(this,activeAnim.mID);
					}
				}	
				
				// Animation has ended
				if( activeAnim.mReverse && activeAnim.mFrameTime < activeAnim.mEndFrameTime ||
				   !activeAnim.mReverse && activeAnim.mFrameTime > activeAnim.mEndFrameTime)
				{
					bFinished = true;
				}	

				// Only clamp the time AFTER checking for loops and finished markers, to ensure that we play the last frame.
				if(activeAnim.mReverse)
					activeAnim.mFrameTime = Clamp(activeAnim.mFrameTime, activeAnim.mEndFrameTime, activeAnim.mStartFrameTime);
				else
					activeAnim.mFrameTime = Clamp(activeAnim.mFrameTime, activeAnim.mStartFrameTime, activeAnim.mEndFrameTime);

				ProcessEvents(oldFrameTime, activeAnim.mFrameTime, &activeAnim, pAnimation);
				AnimateControls(pAnimation, activeAnim);
			}

			if(bFinished || !pAnimation)
			{
				if(LogIsEnabled())
				{
					Otter::LogInfo("View '%s' : Animation '%s' [ID: %u] -- ENDED", GetName(), GetActiveAnimationName(activeAnim.mID), activeAnim.mID);
				}

				mFinishedAnims.push_back(activeAnim.mID);

				DestroyAnimationInstance(mActiveAnimations[index]);
				index = mActiveAnimations.erase(index);
			}
			else
			{
				index++;
			}
		}
		
		for(uint32 i = 0; i < mFinishedAnims.size(); i++)	
			mOnAnimationEnded(this, mFinishedAnims[i]);
	}

	/*
	 * Returns the length of an animation by name.  If the animation is not found,
	 * returns 0.
	 */
	uint32 View::GetAnimationLength(const char* szName)
	{
		const ViewData* pData = static_cast<const ViewData*>(GetData());
		if(!pData)
			return 0;

		const AnimationListData* pAnimListData = pData->GetAnimationListData();
		if(!pAnimListData)
			return 0;

		for(uint32 i = 0; i < pAnimListData->mNumAnimations; i++)
		{
			const AnimationData* pAnimData = pAnimListData->GetAnimation(i);
			if(!pAnimData)
				continue;			

			if(strcmp((const char*)pAnimData->mName, szName) == 0 && pAnimData->mNumFrames > 0)
			{
				return pAnimData->mNumFrames;
			}
		}

		return 0;
 	}

	/** 
	* Retrieves the end frame time of a given Animation. Returns -1 if the animation is not found.
	*/
	float View::GetAnimationEndFrameTime(uint32 animID)
	{
		ActiveAnimation* pAnim = GetActiveAnimation(animID);

		if(pAnim == NULL)
		{
			return -1;
		}

		return pAnim->mEndFrameTime;
	}

	/** 
	* Retrieves the current frame time of a given Animation. Returns -1 if the animation is not found.
	*/
	float View::GetAnimationFrameTime(uint32 animID)
	{
		ActiveAnimation* pAnim = GetActiveAnimation(animID);

		if(pAnim == NULL)
		{
			return -1;
		}

		return pAnim->mFrameTime;
	}

	uint32 GetMaxID(Control* pControl)
	{
		sint32 maxID = 0;

		if(pControl)
		{
			maxID = (sint32)pControl->GetID();

			int numControls = pControl->GetNumControls();
			for(int i = 0; i < numControls; i++)
			{
				sint32 childMaxID = (sint32)GetMaxID(pControl->GetControlAtIndex(i));
				if(childMaxID > maxID)
					maxID = childMaxID;
			}
		}

		return (uint32)maxID;
	}
	
	/*
	 * Returns a new (unused) control ID
	 */
	uint32 View::GenerateNewID()
	{
		return GetMaxID(this) + 1;
	}
}
