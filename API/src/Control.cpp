#include <string.h>
#include "Control.h"
#include "Mask.h"
#include "Data/OtterData.h"
#include "Utilities/Utilities.h"
#include "Graphics/Graphics.h"
#include "Memory/Memory.h"
#include <assert.h>
#include <limits>

using namespace VectorMath;

namespace Otter
{
	/* Constructor
	 */
	Control::Control(Scene* pScene, Control* pParent, const ControlData* pControlData)
	{
		mParent = pParent;
		mControlData = pControlData;

		mScene = pScene;

		mEnabled = true;
		mUpdateTransform = true;

		mUserCreated = false;
		mTouchEnabled = true;

		mMaskControl = NULL;
	}

	/* Virtual Destructor
	 */
	Control::~Control(void)
	{
		SetParentControl(NULL);
	}

	/* Retrieves the control's name
	 */
	const char* Control::GetName()
	{
		if(!mControlData)
			return "";

		return mControlData->mName;
	}
	
	/* Retrieves the control's ID
	 */
	uint32 Control::GetID()
	{
		if(!mControlData)
			return 0xFFFFFFFF;

		return mControlData->mID;
	}	

	/* Retrieves the control's position
	 */
	const Vector2& Control::GetPosition()
	{
		Vector2& v = *reinterpret_cast<Vector2*>((float*)mControlData->mPosition);
		return v;
	}


	/* Retrieves the control's absolute position
	 */
	const Vector2 Control::GetAbsolutePosition()
	{
		Vector2 v = *reinterpret_cast<Vector2*>((float*)mControlData->mPosition);
		
		Control* parent = GetParentControl();

		if( parent )
		{
			Vector2 parentv = parent->GetAbsolutePosition();
			v.x += parentv.x;
			v.y += parentv.y;
		}

		return v;
	}

	/* Sets the control's position
	 */
	void Control::SetPosition(const Vector2& position)
	{
		ControlData* pControlData = const_cast<ControlData*>(mControlData);
		pControlData->mPosition[0] = position.x;
		pControlData->mPosition[1] = position.y;

		mUpdateTransform = true;
	}		

	/* Retrieves the control's center
	 */
	const Vector2& Control::GetCenter()
	{
		Vector2& v = *reinterpret_cast<Vector2*>((float*)mControlData->mCenter);
		return v;
	}

	/* Sets the control's center
	 */
	void Control::SetCenter(const Vector2& center)
	{
		ControlData* pControlData = const_cast<ControlData*>(mControlData);
		pControlData->mCenter[0] = center.x;
		pControlData->mCenter[1] = center.y;

		mUpdateTransform = true;
	}

	/* Retrieves the control's size
	 */
	const Vector2& Control::GetSize()
	{
		Vector2& v = *reinterpret_cast<Vector2*>((float*)mControlData->mSize);
		return v;
	}

	/* Sets the control's size
	 */
	void Control::SetSize(const Vector2& size)
	{
		ControlData* pControlData = const_cast<ControlData*>(mControlData);
		pControlData->mSize[0] = size.x;
		pControlData->mSize[1] = size.y;

		mUpdateTransform = true;
	}

	/* Gets the control's rotation
	 */
	float Control::GetRotation()
	{
		return mControlData->mRotation;
	}

	/* Sets the control's rotation
	 */
	void Control::SetRotation(float rotation)
	{
		ControlData* pControlData = const_cast<ControlData*>(mControlData);
		pControlData->mRotation = rotation;

		mUpdateTransform = true;
	}

	/* Gets the control's mask ID
	 */
	int Control::GetMask()
	{
		return mControlData->mMaskID;
	}

	/* Gets the control's mask
	 */
	Mask* Control::GetMaskControl()
	{
		return mMaskControl;
	}

	/* Sets the control's mask ID
	 */
	void Control::SetMask(int maskID)
	{
		ControlData* pControlData = const_cast<ControlData*>(mControlData);

		Control* parentView = GetParentView();
		if (parentView)
		{
			pControlData->mMaskID = maskID;

			if (maskID == -1)
				mMaskControl = NULL;
			else
				mMaskControl = (Mask*)parentView->GetControl(maskID);
		}
		else
		{
			//this is a view; you can't set a mask on a view
			pControlData->mMaskID = -1;
			mMaskControl = NULL;
		}
	}

	/* Calculates the local transform for this control.
	 */
	const Matrix4& Control::GetTransform()
	{	
		if(mUpdateTransform)
		{
			const Vector2& center = GetCenter();
			const Vector2& pos = GetPosition();
			float rot = GetRotation();

			Matrix4 offsetTrans		= Matrix4::Translation(-center.x, -center.y, 0.0f);
			Matrix4 rotation		= Matrix4::RotationZ(rot * Constants::DEG_TO_RAD);
			Matrix4 locTrans		= Matrix4::Translation(pos.x, pos.y, 0.0f);

			mTransform = mBaseTransform * (locTrans * (rotation * offsetTrans));
			mUpdateTransform = false;
		}

		return mTransform;
	}

	/* Calculates the full transform for this control.
	 */
	const Matrix4 Control::GetFullTransform()
	{	
		Matrix4 fullTransform = Matrix4::IDENTITY;
		Control* pControl = this;
		while(pControl)
		{
			fullTransform = pControl->GetTransform() * fullTransform;
			pControl = pControl->GetParentControl();
		}

		return fullTransform;
	}



	/**
		* Sets the base transform for this control
		*/
	void Control::SetBaseTransform(const VectorMath::Matrix4& baseTransform)
	{
		mBaseTransform = baseTransform;
		mUpdateTransform = true;
	}

	/* Retrieves a control in the hierachy by id
	 */
	Control* Control::GetControl(uint32 id)
	{
		if(id == 0xFFFFFFFF)
			return NULL;

		if(GetID() == id)
			return this;

		for(uint32 i = 0; i < mControls.size(); i++)
		{
			Control* pControl = mControls[i];
			pControl = pControl->GetControl(id);

			if(pControl)
				return pControl;
		}

		return NULL;
	}
	
	/* Retrieves a control by point.
	 */
	Control* Control::GetControl(const Point& point, Point* localPoint, bool touchablesOnly)
	{
		if(!mEnabled || touchablesOnly && !TouchesEnabled())
			return NULL;
		for(sint32 i = (sint32)mControls.size() - 1; i >= 0; --i)
			if( Control * c=mControls[i]->GetControl(point, localPoint, touchablesOnly) )
				return c;
		Point lp;
		ScreenToLocal(point, lp);		

		const Vector2& dims = GetSize();
		if (lp.x >= 0.0f && lp.x <= dims.x && lp.y >= 0.0 && lp.y <= dims.y)
		{
			if(localPoint)
			{
				localPoint->x = lp.x;
				localPoint->y = lp.y;
			}

			return this;
		}

		return NULL;
	}

	/* Adds a child control
		*/
	Result Control::AddControl(Control* pControl)
	{		
		if(pControl == NULL)
			return kResult_InvalidParameter;

		if(pControl->GetParentControl() != NULL)
			pControl->GetParentControl()->RemoveControl(pControl);

		mControls.push_back(pControl);
		pControl->SetParentControl(this);

		return kResult_OK;
	}

	/* Removes a child control
	 */
	Result Control::RemoveControl(Control* pControl)
	{
		if(pControl == NULL)
			return kResult_InvalidParameter;

		for(uint32 i = 0; i < mControls.size(); i++)
		{
			if(mControls[i] == pControl)
			{
				mControls.erase(i);
		
				return kResult_OK;
			}
		}

		return kResult_InvalidParameter;
	}

	/* Retrieves a control by name
	 */
	Control* Control::GetControl(const char* szControlName)
	{
		if(strcmp(GetName(), szControlName) == 0)
			return this;

		for(uint32 i = 0; i < mControls.size(); i++)
		{
			Control* pControl = mControls[i];
			pControl = pControl->GetControl(szControlName);

			if(pControl)
				return pControl;
		}

		return NULL;
	}

	/* Retrieves a control by index
	 */
	Control* Control::GetControlAtIndex(uint32 index)
	{
		if(index >= mControls.size())
			return NULL;

		return mControls[index];
	}

	/* Retrieves the number of direct child controls
	 */
	uint32 Control::GetNumControls()
	{
		return mControls.size();
	}

	/* Converts the screen point to a local
	 * local point, relative to the control
	 */
	void Control::ScreenToLocal(const Point& point, Point& localPoint)
	{
		Matrix4 inverse = GetFullTransform().Inverse();
		Vector4 v(point.x, point.y, 0.0f, 1.0f);
		v = inverse * v;

		localPoint.x = v.x;
		localPoint.y = v.y;
	}

	/* Sets the control's parent
		*/
	Result Control::SetParentControl(Control* pParent)
	{
		mParent = pParent;
		mScene = (pParent == NULL) ? NULL : pParent->GetScene();

		mUpdateTransform = true;

		return kResult_OK;
	}

	/* Returns the parent of this control
	 */
	Control* Control::GetParentControl()
	{
		return mParent;
	}

	/* Returns the containing view of this control
	 * The top-most parent will always be a View, unless
	 * 'this' is a view, in which case mParent is NULL and
	 * GetView returns NULL.
	 */
	View* Control::GetParentView()
	{		
		Control* pParent = mParent;
		while(pParent != NULL && pParent->GetParentControl() != NULL)
			pParent = pParent->GetParentControl();

		return (View*)pParent;
	}

	void Control::RemapMasks( View * parentView, Control * cloneRoot )
	{
		assert(parentView!=0);
		assert(cloneRoot!=0);
		if( mControlData->mMaskID!=-1 )
		{
			mMaskControl = static_cast<Mask *>(cloneRoot->GetControl(parentView->GetControl(mControlData->mMaskID)->GetName()));
			const_cast<ControlData *>(mControlData)->mMaskID = mMaskControl->GetID();
		}
		for (uint32 i = 0; i < mControls.size(); ++i)
			mControls[i]->RemapMasks(parentView,cloneRoot);
	}

	/* Brings the specified control the front, ie drawn on top of everything else	 
	 */
	void Control::BringToFront(Control* pControl)
	{
		if(!pControl)
			return;

		for(uint32 i = 0; i < mControls.size(); i++)
		{			
			if(mControls[i]->GetID() == pControl->GetID())
			{
				mControls.erase(i);
				mControls.push_back(pControl);
				break;
			}
		}
	}

	/* Sends a control to the back, ie drawn behind everything else
	 */
	void Control::SendToBack(Control* pControl)
	{
		if(!pControl)
			return;

		for(uint32 i = 0; i < mControls.size(); i++)
		{			
			if(mControls[i]->GetID() == pControl->GetID())
			{
				mControls.erase(i);
				mControls.push_front(pControl);
				break;
			}
		}
	}	
	/* Points (touches/mouse/etc) were pressed down
	 */
	bool Control::OnPointsDown(const Point* points, sint32 numPoints)
	{
		assert(points!=0);
		assert(numPoints>=0);
		return false;
	}
		
	/* Points (touches/mouse/etc) were released
	 */
	bool Control::OnPointsUp(const Point* points, sint32 numPoints)
	{
		assert(points!=0);
		assert(numPoints>=0);
		return false;
	}
		
	/* Points (touches/mouse/etc) were moved.
	 */
	bool Control::OnPointsMove(const Point* points, sint32 numPoints)
	{
		assert(points!=0);
		assert(numPoints>=0);
		return false;
	}
		
	/**
	* Points (touches/mouse/etc) were released but the control was unreachable (other control in the way)
	* Returns a reference to the control that handled the points
	*/
	bool Control::OnPointsUpCancel(const Point* points, sint32 numPoints)
	{
		assert(points!=0);
		assert(numPoints>=0);
		return false;
	}

	/* Applies the interpolation of two keyframes to the control.
	 * pEndFrame can be NULL.
	 */
	void Control::ApplyKeyFrame(const KeyFrameData* pStartFrame, const KeyFrameData* pEndFrame, float factor)
	{
		if (pStartFrame == NULL && pEndFrame == NULL)
            return;

        Rectangle parentRect;
        if (mParent != NULL)
		{
			const Vector2& size = mParent->GetSize();
			parentRect.w = size.x;
			parentRect.h = size.y;
        }

		const ControlLayout* pStartLayout	= pStartFrame->GetLayout();
		const ControlLayout* pEndLayout		= pEndFrame ? pEndFrame->GetLayout() : NULL;

        float startLeft		= pStartLayout->mX - pStartLayout->mCenterX;
        float startRight	= startLeft + pStartLayout->mWidth;
        float startTop		= pStartLayout->mY - pStartLayout->mCenterY;
        float startBottom	= startTop + pStartLayout->mHeight;

        Rectangle startRect(startLeft, startTop, (startRight - startLeft), (startBottom - startTop));
		Point startCenter(pStartLayout->mCenterX, pStartLayout->mCenterY);

        float leftValue     = (mControlData->mAnchorFlags & LeftRelative) == 0 ? pStartFrame->mLeftAbs : pStartFrame->mLeftRel;
        float rightValue    = (mControlData->mAnchorFlags & RightRelative) == 0 ? pStartFrame->mRightAbs : pStartFrame->mRightRel;
        float topValue      = (mControlData->mAnchorFlags & TopRelative) == 0 ? pStartFrame->mTopAbs : pStartFrame->mTopRel;
        float bottomValue   = (mControlData->mAnchorFlags & BottomRelative) == 0 ? pStartFrame->mBottomAbs : pStartFrame->mBottomRel;

		Otter::ComputeAnchoredRectangle( startRect, startCenter, parentRect, 
                                         (AnchorFlags)mControlData->mAnchorFlags, 
                                         leftValue, rightValue, topValue, bottomValue);

        Rectangle endRect;
        Point endCenter;

        if(pEndFrame != NULL)
        {

            float endLeft = pEndLayout->mX - pEndLayout->mCenterX;
            float endRight = endLeft + pEndLayout->mWidth;
            float endTop = pEndLayout->mY - pEndLayout->mCenterY;
            float endBottom = endTop + pEndLayout->mHeight;

            endRect = Rectangle(endLeft, endTop, (endRight - endLeft), (endBottom - endTop));
			endCenter = Point(pEndLayout->mCenterX, pEndLayout->mCenterY);

            leftValue   = (mControlData->mAnchorFlags & LeftRelative) == 0 ? pEndFrame->mLeftAbs : pEndFrame->mLeftRel;
            rightValue  = (mControlData->mAnchorFlags & RightRelative) == 0 ? pEndFrame->mRightAbs : pEndFrame->mRightRel;
            topValue    = (mControlData->mAnchorFlags & TopRelative) == 0 ? pEndFrame->mTopAbs : pEndFrame->mTopRel;
            bottomValue = (mControlData->mAnchorFlags & BottomRelative) == 0 ? pEndFrame->mBottomAbs : pEndFrame->mBottomRel;

			Otter::ComputeAnchoredRectangle( endRect, endCenter, parentRect, 
											 (AnchorFlags)mControlData->mAnchorFlags,
                                             leftValue, rightValue, topValue, bottomValue);
        }   

        float newX = 0;
        float newY = 0;
        float newW = 0;
        float newH = 0;
        float newCX = 0;
        float newCY = 0;

        float newR = 0.0f;

        if (pEndFrame == NULL)
        {
            newCX = startCenter.x;
            newCY = startCenter.y;

            newX = startRect.x;
            newY = startRect.y;

            newW = startRect.w;
            newH = startRect.h;

            newR = pStartLayout->mRotation;
        }
        else
        {
            newCX = startCenter.x * (1.0f - factor) + endCenter.x * factor;
            newCY = startCenter.y * (1.0f - factor) + endCenter.y * factor;

            newX = startRect.x * (1.0f - factor) + endRect.x * factor;
            newY = startRect.y * (1.0f - factor) + endRect.y * factor;

            newW = (startRect.w) * (1.0f - factor) + (endRect.w) * factor;
            newH = (startRect.h) * (1.0f - factor) + (endRect.h) * factor;

            newR = pStartLayout->mRotation * (1.0f - factor) + pEndLayout->mRotation * factor;
        }

        SetCenter(Vector2(newCX, newCY));
		SetPosition(Vector2(newX + newCX, newY + newCY));
		SetSize(Vector2(newW, newH));
        SetRotation(newR);
	}

	/* Draws the control.
		*/
	void Control::Draw(Graphics* pGraphics)
	{
		if(!mEnabled)
			return;		
		if( int const ncontrols=mControls.size() )
		{
			const VectorMath::Vector2& size = GetSize();
			if(size.x == 0.0f || size.y == 0.0f)
				return;
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

	/**
	 * Stores a pointer to this control's mask, and recurses on child controls
	 */
	void Control::SetMaskPointers()
	{
		SetMask(GetMask());

		for (uint32 i = 0; i < mControls.size(); ++i)
			mControls[i]->SetMaskPointers();
	}

}
