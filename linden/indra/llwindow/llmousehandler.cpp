/**
 * @file llmousehandler.cpp
 * @brief LLMouseHandler class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2001-2007, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; version 2.1 of the License only.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA"
 * $/LicenseInfo$
 */

#include "llmousehandler.h"

//virtual
bool LLMouseHandler::handleAnyMouseClick(S32 x, S32 y, MASK mask,
										 EClickType clicktype, bool down)
{
	bool handled = false;

	if (down)
	{
		switch (clicktype)
		{
			case CLICK_LEFT:
				handled = handleMouseDown(x, y, mask);
				break;

			case CLICK_RIGHT:
				handled = handleRightMouseDown(x, y, mask);
				break;

			case CLICK_MIDDLE:
				handled = handleMiddleMouseDown(x, y, mask);
				break;

			case CLICK_DOUBLELEFT:
				handled = handleDoubleClick(x, y, mask);
		}
	}
	else
	{
		switch (clicktype)
		{
			case CLICK_LEFT:
				handled = handleMouseUp(x, y, mask);
				break;

			case CLICK_RIGHT:
				handled = handleRightMouseUp(x, y, mask);
				break;

			case CLICK_MIDDLE:
				handled = handleMiddleMouseUp(x, y, mask);
				break;

			case CLICK_DOUBLELEFT:
				handled = handleDoubleClick(x, y, mask);
		}
	}

	return handled;
}
