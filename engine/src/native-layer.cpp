/* Copyright (C) 2015 LiveCode Ltd.
 
 This file is part of LiveCode.
 
 LiveCode is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License v3 as published by the Free
 Software Foundation.
 
 LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.
 
 You should have received a copy of the GNU General Public License
 along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
#include "util.h"
#include "mcerror.h"
#include "sellst.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "widget.h"
#include "param.h"
#include "osspec.h"
#include "cmds.h"
#include "scriptpt.h"
#include "hndlrlst.h"
#include "debug.h"
#include "redraw.h"
#include "font.h"
#include "chunk.h"
#include "graphicscontext.h"
#include "objptr.h"

#include "globals.h"
#include "context.h"

#include "native-layer.h"

////////////////////////////////////////////////////////////////////////////////

MCNativeLayer::MCNativeLayer() :
	m_object(nil), m_attached(false), m_can_render_to_context(true),
	m_defer_geometry_changes(false), m_visible(false), m_show_for_tool(false)
{
    ;
}

MCNativeLayer::~MCNativeLayer()
{
    ;
}

////////////////////////////////////////////////////////////////////////////////

void MCNativeLayer::OnAttach()
{
	m_attached = true;
	doAttach();
}

void MCNativeLayer::OnDetach()
{
	m_attached = false;
	doDetach();
}

bool MCNativeLayer::OnPaint(MCGContextRef p_context)
{
	return doPaint(p_context);
}

void MCNativeLayer::OnGeometryChanged(const MCRectangle &p_new_rect)
{
	if (!m_defer_geometry_changes)
		doSetGeometry(p_new_rect);
}

void MCNativeLayer::OnToolChanged(Tool p_new_tool)
{
	bool t_showing;
	t_showing = ShouldShowLayer();
	
	m_show_for_tool = p_new_tool == T_BROWSE || p_new_tool == T_HELP;

	if (t_showing != (ShouldShowLayer()))
		UpdateVisibility();

	m_object->Redraw();
}

void MCNativeLayer::OnVisibilityChanged(bool p_visible)
{
	bool t_showing;
	t_showing = ShouldShowLayer();
	
	m_visible = p_visible;
	
	if (t_showing != (ShouldShowLayer()))
		UpdateVisibility();
}

void MCNativeLayer::UpdateVisibility()
{
	if (ShouldShowLayer())
	{
		if (m_defer_geometry_changes)
			doSetGeometry(m_object->getrect());
		m_defer_geometry_changes = false;
	}
	else
	{
		if (!GetCanRenderToContext())
			m_defer_geometry_changes = true;
	}
	
	doSetVisible(ShouldShowLayer());
}

void MCNativeLayer::OnLayerChanged()
{
	doRelayer();
}

////////////////////////////////////////////////////////////////////////////////

bool MCNativeLayer::isAttached() const
{
    return m_attached;
}

bool MCNativeLayer::ShouldShowLayer()
{
	return m_show_for_tool && m_visible;
}

struct MCNativeLayerFindObjectVisitor: public MCObjectVisitor
{
	MCObject *current_object;
	MCObject *found_object;
	
	bool find_above;
	bool reached_current;
	
	bool OnObject(MCObject *p_object)
	{
		// We are only looking for objects with a native layer
		if (p_object->getNativeLayer() != nil)
			return OnNativeLayerObject(p_object);
		
		return true;
	}
	
	bool OnNativeLayerObject(MCObject *p_object)
	{
		if (find_above)
		{
			if (reached_current)
			{
				found_object = p_object;
				return false;
			}
			else if (p_object == current_object)
			{
				reached_current = true;
				return true;
			}
			return true;
		}
		else
		{
			if (p_object == current_object)
			{
				reached_current = true;
				return false;
			}
			else
			{
				found_object = p_object;
				return true;
			}
		}
	}
};


MCObject* MCNativeLayer::findNextLayerAbove(MCObject* p_object)
{
	MCNativeLayerFindObjectVisitor t_visitor;
	t_visitor.current_object = p_object;
	t_visitor.found_object = nil;
	t_visitor.reached_current = false;
	t_visitor.find_above = true;
	
	p_object->getparent()->visit_children(0, 0, &t_visitor);
	
	return t_visitor.found_object;
}

MCObject* MCNativeLayer::findNextLayerBelow(MCObject* p_object)
{
	MCNativeLayerFindObjectVisitor t_visitor;
	t_visitor.current_object = p_object;
	t_visitor.found_object = nil;
	t_visitor.reached_current = false;
	t_visitor.find_above = false;
	
	p_object->getparent()->visit_children(0, 0, &t_visitor);
	
	return t_visitor.found_object;
}

////////////////////////////////////////////////////////////////////////////////

void MCNativeLayer::SetCanRenderToContext(bool p_can_render)
{
	m_can_render_to_context = p_can_render;
}

bool MCNativeLayer::GetCanRenderToContext()
{
	return m_can_render_to_context;
}

////////////////////////////////////////////////////////////////////////////////
