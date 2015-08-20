#include <string.h>

#include "Group.h"
#include "View.h"
#include "Mask.h"

#include "Math/VectorMath.h"
#include "Data/OtterData.h"
#include "Graphics/Graphics.h"
#include "Memory/Memory.h"

using namespace VectorMath;

namespace Otter
{
	/* Constructor
	 */
	Group::Group(Scene* pScene, Control* pParent, const GroupData* pGroupData) : Control(pScene, pParent, pGroupData)
	{
	}

	/* Constructor
	 */
	Group::Group() : Control(NULL, NULL, OTTER_NEW(GroupData, ()))
	{
		mUserCreated = true;
	}

	/* Virtual Destructor
	 */
	Group::~Group(void)
	{
		if(mUserCreated)
		{
			for(uint32 i=0,n=mControls.size(); i!=n ; ++i)
				OTTER_DELETE(mControls[i]);
				
			OTTER_DELETE(GetData());
		}
	}

	/* Draws the sprite to screen
	 */
	void Group::Draw(Graphics* pGraphics)
	{		
		Control::Draw(pGraphics);
	}

	/**
	 * Clones the control and returns the new instance
	 */
	Control* Group::Clone()
	{
		Group* pGroup = OTTER_NEW(Group, ());
		memcpy((uint8*)pGroup->GetData(), (uint8*)GetData(), sizeof(GroupData));
		((ControlData*)pGroup->GetData())->mID = GetParentView()->GenerateNewID();
		mParent->AddControl(pGroup);

		uint32 numControls = this->GetNumControls();
		for(uint32 i = 0; i < numControls; i++)
		{
			Control* pControl = this->GetControlAtIndex(i)->Clone();
			pGroup->AddControl(pControl);
		}

		return pGroup;
	}

	Control* Group::GetControl(const Point& point, Point* localPoint, bool touchablesOnly)
	{
		if(!mEnabled || touchablesOnly && !TouchesEnabled())
			return 0;
		for(sint32 i = (sint32)mControls.size() - 1; i >= 0; --i)
			if( Control * c=mControls[i]->GetControl(point, localPoint, touchablesOnly) )
				return c;
		return 0;
	}
}
