/*
 * Copyright (c) 2016 AudioTools - All Rights Reserved
 *
 * This Software may not be distributed in parts or its entirety
 * without prior written agreement by AudioTools.
 *
 * Neither the name of the AudioTools nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY AUDIOTOOLS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AUDIOTOOLS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Written by Alexandre Arsenault <alx.arsenault@gmail.com>
 */

#include "editor/atEditorLoader.hpp"
#include "PyoAudio.h"
#include "atSkin.hpp"
#include "atUniqueNameComponent.h"
#include "editor/atEditor.hpp"
#include "editor/atEditorMainWindow.hpp"
#include "python/PyoComponent.hpp"

#include <axlib/Button.hpp>
#include <axlib/Core.hpp>
#include <axlib/DropMenu.hpp>
#include <axlib/Knob.hpp>
#include <axlib/Label.hpp>
#include <axlib/Panel.hpp>
#include <axlib/Slider.hpp>
#include <axlib/Toggle.hpp>
#include <axlib/WidgetLoader.hpp>
#include <axlib/WindowManager.hpp>

namespace at {
namespace editor {
	Loader::Loader(ax::Window* win)
		: _win(win)
	{
	}

	std::string Loader::OpenLayoutFromXml(ax::Xml& xml)
	{
		ax::Xml::Node top_node = xml.GetNode("Layout");

		if (!top_node.IsValid()) {
			ax::console::Error("Loader not layout node.");
			return "";
		}

		std::string script_path;

		try {
			script_path = top_node.GetAttribute("script");
		}
		catch (ax::Xml::Exception& err) {
			//						ax::console::Error("No pyo node.", err.what());
		}

		ax::Xml::Node node = top_node.GetFirstNode();
		ax::widget::Loader* loader = ax::widget::Loader::GetInstance();

		auto panel_builder = loader->GetBuilder("Panel");
		panel_builder->SetCreateCallback([&](ax::Window* win, ax::Xml::Node& node) {
			std::string builder_name = node.GetAttribute("builder");
			std::string pyo_fct_name;

			ax::Xml::Node pyo_node = node.GetNode("pyo");

			if (pyo_node.IsValid()) {
				pyo_fct_name = pyo_node.GetValue();
			}

			std::string unique_name;
			ax::Xml::Node unique_name_node = node.GetNode("unique_name");

			if (unique_name_node.IsValid()) {
				unique_name = unique_name_node.GetValue();
			}

			SetupExistingWidget(win, builder_name, pyo_fct_name, unique_name);
		});

		try {
			while (node.IsValid()) {
				std::string node_name = node.GetName();
				//				ax::console::Print("Node name :", node_name);

				if (node_name == "Widget") {
					std::string buider_name = node.GetAttribute("builder");
					std::string pyo_fct_name;

					ax::Xml::Node pyo_node = node.GetNode("pyo");

					if (pyo_node.IsValid()) {
						pyo_fct_name = pyo_node.GetValue();
					}

					std::string unique_name;
					ax::Xml::Node unique_name_node = node.GetNode("unique_name");

					if (unique_name_node.IsValid()) {
						unique_name = unique_name_node.GetValue();
					}

					ax::widget::Builder* builder = loader->GetBuilder(buider_name);

					if (builder == nullptr) {
						ax::console::Error("Builder", buider_name, "doesn't exist.");
						node = node.GetNextSibling();
						continue;
					}

					auto obj(builder->Create(node));
					_win->node.Add(obj);
					SetupExistingWidget(obj->GetWindow(), buider_name, pyo_fct_name, unique_name);
				}

				node = node.GetNextSibling();
			}
		}
#warning("Catch this.")
		//		catch (rapidxml::parse_error& err) {
		//			ax::console::Error("Widget menu xml", err.what());
		//		}
		catch (ax::Xml::Exception& err) {
			ax::console::Error("Widget menu xml", err.what());
		}

		return script_path;
	}

	std::string Loader::OpenLayoutContent(const std::string& content, bool clear)
	{
		if (content.empty()) {
			return "";
		}

		if (clear) {
			_win->node.GetChildren().clear();
		}

		ax::Xml xml;

		if (!xml.Parse(content)) {
			ax::console::Error("parsing widget menu.");
			return "";
		}

		return OpenLayoutFromXml(xml);
	}

	std::string Loader::OpenLayout(const std::string& path, bool clear)
	{
		if (path.empty()) {
			return "";
		}

		if (clear) {
			_win->node.GetChildren().clear();
		}

		ax::Xml xml(path.c_str());

		if (!xml.Parse()) {
			ax::console::Error("parsing widget menu.");
			return "";
		}

		return OpenLayoutFromXml(xml);
	}

	void Loader::SetupExistingWidget(ax::Window* widget, const std::string& builder_name,
		const std::string& pyo_fct, const std::string& unique_name)
	{
		if (builder_name == "Button") {
			widget->property.AddProperty("Resizable");
			SetupEditWidget(widget);
			SetupPyoComponent(widget, pyo_fct);
			SetupButtonPyoEvent(widget);
			SetupUniqueNameComponent(widget, unique_name);
		}
		else if (builder_name == "Toggle") {
			widget->property.AddProperty("Resizable");
			SetupEditWidget(widget);
			SetupPyoComponent(widget, pyo_fct);
			SetupTogglePyoEvent(widget);
			SetupUniqueNameComponent(widget, unique_name);
		}
		else if (builder_name == "Panel") {
			widget->property.AddProperty("Resizable");
			SetupEditWidget(widget);

			// Add MainWindow property.
			ax::Panel* panel = static_cast<ax::Panel*>(widget->backbone.get());
			if (panel->GetName() == "MainWindow") {
				widget->property.AddProperty("MainWindow");
			}

			widget->property.AddProperty("BlockDrawing");
			SetupUniqueNameComponent(widget, unique_name);
		}
		else if (builder_name == "Knob") {
			widget->property.AddProperty("Resizable");
			SetupEditWidget(widget);
			SetupPyoComponent(widget, pyo_fct);
			SetupKnobPyoEvent(widget);
			SetupUniqueNameComponent(widget, unique_name);
		}
		else if (builder_name == "Slider") {
			widget->property.AddProperty("Resizable");
			SetupEditWidget(widget);
			SetupPyoComponent(widget, pyo_fct);
			SetupSliderPyoEvent(widget);
			SetupUniqueNameComponent(widget, unique_name);
		}
		else if (builder_name == "Label") {
			widget->property.AddProperty("Resizable");
			SetupEditWidget(widget);
			SetupUniqueNameComponent(widget, unique_name);
		}
	}

	void Loader::SetupEditWidget(ax::Window* win)
	{
		ax::Window* gwin = _win;

		auto m_down_fct = win->event.OnMouseLeftDown.GetFunction();
		win->event.OnMouseLeftDown = ax::WFunc<ax::Point>([gwin, win, m_down_fct](const ax::Point& pos) {

			ax::Point c_delta(pos - win->dimension.GetAbsoluteRect().position);

			bool cmd_down = ax::App::GetInstance().GetWindowManager()->IsCmdDown();

			if (cmd_down) {
				win->resource.Add("click_delta", c_delta);
				win->event.GrabMouse();
				win->property.AddProperty("edit_click");

				gwin->PushEvent(
					at::editor::GridWindow::SELECT_WIDGET, new ax::event::SimpleMsg<ax::Window*>(win));

				return;
			}

			if (win->property.HasProperty("current_editing_widget")
				&& win->property.HasProperty("Resizable")) {

				bool top = c_delta.y < 4;
				bool bottom = c_delta.y > win->dimension.GetShownRect().size.h - 4;
				bool right = c_delta.x > win->dimension.GetShownRect().size.w - 4;
				bool left = c_delta.x < 4;

				if (right && bottom) {
					win->property.AddProperty("ResizeBottomRight");
				}
				else if (right && top) {
					win->property.AddProperty("ResizeTopRight");
				}
				else if (left && top) {
					win->property.AddProperty("ResizeTopLeft");
				}
				else if (left && bottom) {
					win->property.AddProperty("ResizeBottomLeft");
				}
				else if (right) {
					win->property.AddProperty("ResizeRight");
				}
				else if (bottom) {
					win->property.AddProperty("ResizeBottom");
				}
				else if (left) {
					win->property.AddProperty("ResizeLeft");
				}
				else if (top) {
					win->property.AddProperty("ResizeTop");
				}

				win->resource.Add("click_delta", c_delta);
				win->event.GrabMouse();
				win->property.AddProperty("edit_click");
				gwin->PushEvent(
					at::editor::GridWindow::SELECT_WIDGET, new ax::event::SimpleMsg<ax::Window*>(win));
			}
			else {
				if (m_down_fct) {
					m_down_fct(pos);
				}
			}
		});

		auto m_drag_fct = win->event.OnMouseLeftDragging.GetFunction();
		win->event.OnMouseLeftDragging = ax::WFunc<ax::Point>([gwin, win, m_drag_fct](const ax::Point& pos) {

			// Editing.
			if (win->property.HasProperty("edit_click")) {
				if (win->event.IsGrabbed()) {

					ax::Point c_delta = win->resource.GetResource("click_delta");

					// Right resize.
					if (win->property.HasProperty("ResizeRight")) {
						int size_y = win->dimension.GetSize().h;
						int size_x = pos.x - win->dimension.GetAbsoluteRect().position.x;
						win->dimension.SetSize(ax::Size(size_x, size_y));
					}
					else if (win->property.HasProperty("ResizeBottomRight")) {
						int size_y = pos.y - win->dimension.GetAbsoluteRect().position.y;
						int size_x = pos.x - win->dimension.GetAbsoluteRect().position.x;
						win->dimension.SetSize(ax::Size(size_x, size_y));
					}
					else if (win->property.HasProperty("ResizeTopRight")) {
						ax::Rect abs_rect(win->dimension.GetAbsoluteRect());
						int size_x = pos.x - win->dimension.GetAbsoluteRect().position.x;
						int size_y = abs_rect.position.y + abs_rect.size.h - pos.y;
						int pos_y = pos.y - win->node.GetParent()->dimension.GetAbsoluteRect().position.y;
						int pos_x = win->dimension.GetRect().position.x;
						win->dimension.SetRect(ax::Rect(pos_x, pos_y, size_x, size_y));

						ax::Point dd(0, abs_rect.position.y - pos.y);
						std::vector<std::shared_ptr<ax::Window>>& children = win->node.GetChildren();
						for (auto& n : children) {
							ax::Point w_pos = n->dimension.GetRect().position;
							n->dimension.SetPosition(w_pos + dd);
						}
					}
					// Bottom resize.
					else if (win->property.HasProperty("ResizeBottom")) {
						int size_x = win->dimension.GetSize().w;
						int size_y = pos.y - win->dimension.GetAbsoluteRect().position.y;
						win->dimension.SetSize(ax::Size(size_x, size_y));
					}
					// Left resize.
					else if (win->property.HasProperty("ResizeLeft")) {
						ax::Rect abs_rect(win->dimension.GetAbsoluteRect());
						int size_x = abs_rect.position.x + abs_rect.size.w - pos.x;
						int size_y = abs_rect.size.h;
						int pos_y = win->dimension.GetRect().position.y;
						int pos_x = pos.x - win->node.GetParent()->dimension.GetAbsoluteRect().position.x;
						win->dimension.SetRect(ax::Rect(pos_x, pos_y, size_x, size_y));

						ax::Point dd(abs_rect.position.x - pos.x, 0);
						std::vector<std::shared_ptr<ax::Window>>& children = win->node.GetChildren();
						for (auto& n : children) {
							ax::Point w_pos = n->dimension.GetRect().position;
							n->dimension.SetPosition(w_pos + dd);
						}
					}
					else if (win->property.HasProperty("ResizeBottomLeft")) {
						ax::Rect abs_rect(win->dimension.GetAbsoluteRect());
						int size_x = abs_rect.position.x + abs_rect.size.w - pos.x;
						int size_y = pos.y - win->dimension.GetAbsoluteRect().position.y;
						int pos_y = win->dimension.GetRect().position.y;
						int pos_x = pos.x - win->node.GetParent()->dimension.GetAbsoluteRect().position.x;
						win->dimension.SetRect(ax::Rect(pos_x, pos_y, size_x, size_y));

						ax::Point dd(abs_rect.position.x - pos.x, 0);
						std::vector<std::shared_ptr<ax::Window>>& children = win->node.GetChildren();
						for (auto& n : children) {
							ax::Point w_pos = n->dimension.GetRect().position;
							n->dimension.SetPosition(w_pos + dd);
						}
					}
					// Top resize.
					else if (win->property.HasProperty("ResizeTop")) {
						ax::Rect abs_rect(win->dimension.GetAbsoluteRect());
						int size_x = abs_rect.size.w;
						int size_y = abs_rect.position.y + abs_rect.size.h - pos.y;
						int pos_y = pos.y - win->node.GetParent()->dimension.GetAbsoluteRect().position.y;
						int pos_x = win->dimension.GetRect().position.x;
						win->dimension.SetRect(ax::Rect(pos_x, pos_y, size_x, size_y));

						ax::Point dd(0, abs_rect.position.y - pos.y);
						std::vector<std::shared_ptr<ax::Window>>& children = win->node.GetChildren();
						for (auto& n : children) {
							ax::Point w_pos = n->dimension.GetRect().position;
							n->dimension.SetPosition(w_pos + dd);
						}
					}
					else if (win->property.HasProperty("ResizeTopLeft")) {
						ax::Rect abs_rect(win->dimension.GetAbsoluteRect());
						int size_x = abs_rect.position.x + abs_rect.size.w - pos.x;
						int size_y = abs_rect.position.y + abs_rect.size.h - pos.y;
						int pos_y = pos.y - win->node.GetParent()->dimension.GetAbsoluteRect().position.y;
						int pos_x = pos.x - win->node.GetParent()->dimension.GetAbsoluteRect().position.x;
						win->dimension.SetRect(ax::Rect(pos_x, pos_y, size_x, size_y));

						ax::Point dd(abs_rect.position.x - pos.x, abs_rect.position.y - pos.y);
						std::vector<std::shared_ptr<ax::Window>>& children = win->node.GetChildren();
						for (auto& n : children) {
							ax::Point w_pos = n->dimension.GetRect().position;
							n->dimension.SetPosition(w_pos + dd);
						}
					}
					// Moving widget.
					else {
						win->dimension.SetPosition(
							pos - win->node.GetParent()->dimension.GetAbsoluteRect().position - c_delta);
					}

					/// @todo Don't send this at every mouse move.
					gwin->PushEvent(at::editor::GridWindow::BEGIN_DRAGGING_WIDGET, new ax::event::EmptyMsg());
				}
			}

			// Call widget callback.
			else {
				if (m_drag_fct) {
					m_drag_fct(pos);
				}
			}
		});

		auto m_up_fct = win->event.OnMouseLeftUp.GetFunction();
		win->event.OnMouseLeftUp = ax::WFunc<ax::Point>([gwin, win, m_up_fct](const ax::Point& pos) {

			// Editing.
			if (win->property.HasProperty("edit_click")) {

				win->property.RemoveProperty("edit_click");
				win->property.RemoveProperty("ResizeLeft");
				win->property.RemoveProperty("ResizeRight");
				win->property.RemoveProperty("ResizeBottom");
				win->property.RemoveProperty("ResizeTop");

				win->property.RemoveProperty("ResizeTopLeft");
				win->property.RemoveProperty("ResizeTopRight");
				win->property.RemoveProperty("ResizeBottomLeft");
				win->property.RemoveProperty("ResizeBottomRight");

				if (win->event.IsGrabbed()) {
					win->event.UnGrabMouse();
				}

				gwin->PushEvent(at::editor::GridWindow::DONE_DRAGGING_WIDGET, new ax::event::EmptyMsg());
			}

			// Call widget callback.
			else {
				if (m_up_fct) {
					m_up_fct(pos);
				}
			}
		});

		auto m_right_down = win->event.OnMouseRightDown.GetFunction();
		win->event.OnMouseRightDown = ax::WFunc<ax::Point>([gwin, win, m_right_down](const ax::Point& pos) {

			if (win->property.HasProperty("current_editing_widget")) {
				win->property.AddProperty("edit_click");

				ax::App::GetInstance().GetCore()->SetCursor(ax::core::Core::Cursor::NORMAL);

				gwin->PushEvent(at::editor::GridWindow::DROP_WIDGET_MENU,
					new ax::event::SimpleMsg<std::pair<ax::Point, ax::Window*>>(
						std::pair<ax::Point, ax::Window*>(pos, win)));

				return;
			}

			// Call widget callback.
			if (m_right_down) {
				m_right_down(pos);
			}
		});

		// Mouse motion.
		auto m_motion = win->event.OnMouseMotion.GetFunction();
		win->event.OnMouseMotion = ax::WFunc<ax::Point>([gwin, win, m_motion](const ax::Point& pos) {

			if (win->property.HasProperty("current_editing_widget")) {
				ax::Point c_delta(pos - win->dimension.GetAbsoluteRect().position);

				bool cmd_down = ax::App::GetInstance().GetWindowManager()->IsCmdDown();

				if (cmd_down) {
					ax::App::GetInstance().GetCore()->SetCursor(ax::core::Core::Cursor::NORMAL);
					return;
				}

				if (win->property.HasProperty("Resizable")) {

					bool top = c_delta.y < 4;
					bool bottom = c_delta.y > win->dimension.GetShownRect().size.h - 4;
					bool right = c_delta.x > win->dimension.GetShownRect().size.w - 4;
					bool left = c_delta.x < 4;

					if ((right && bottom) || (top && left)) {
						ax::App::GetInstance().GetCore()->SetCursor(
							ax::core::Core::Cursor::RESIZE_TOP_LEFT_DOWN_RIGHT);
					}
					else if ((bottom && left) || (top && right)) {
						ax::App::GetInstance().GetCore()->SetCursor(
							ax::core::Core::Cursor::RESIZE_BOTTOM_LEFT_TOP_RIGHT);
					}
					else if (right || left) {
						ax::App::GetInstance().GetCore()->SetCursor(
							ax::core::Core::Cursor::RESIZE_LEFT_RIGHT);
					}
					else if (bottom || top) {
						ax::App::GetInstance().GetCore()->SetCursor(ax::core::Core::Cursor::RESIZE_UP_DOWN);
					}
					else {
						ax::App::GetInstance().GetCore()->SetCursor(ax::core::Core::Cursor::MOVE);

						//						if (m_motion) {
						//							m_motion(pos);
						//						}
					}
				}
			}
			else {
				if (m_motion) {
					m_motion(pos);
				}
			}
		});

		// OnMouseLeave event.
		auto m_leave = win->event.OnMouseLeave.GetFunction();
		win->event.OnMouseLeave = ax::WFunc<ax::Point>([gwin, win, m_leave](const ax::Point& pos) {

			//			ax::console::Print("Mouse leave");

			if (win->property.HasProperty("edit_click")) {
				//				ax::console::Print("Mouse leave has -> edit click");
			}
			else {
				// Set normal cursor.
				ax::App::GetInstance().GetCore()->SetCursor(ax::core::Core::Cursor::NORMAL);

				if (m_leave) {
					m_leave(pos);
				}
			}
		});

		//		auto m_l_arrow = win->event.OnLeftArrowDown.GetFunction();
		//		win->event.OnLeftArrowDown = ax::WFunc<char>([gwin, win, m_l_arrow](const char& c) {
		//
		//			if (win->property.HasProperty("current_editing_widget")) {
		//				const ax::Rect& w_rect = win->dimension.GetRect();
		//				win->dimension.SetPosition(w_rect.position - ax::Point(1, 0));
		//			}
		//			else {
		//				if(m_l_arrow) {
		//					m_l_arrow(c);
		//				}
		//			}
		//		});

		// OnPaintOverFrameBuffer event.
		win->event.OnPaintOverChildren = ax::WFunc<ax::GC>([win](ax::GC gc) {
			if (win->property.HasProperty("current_editing_widget")) {

				ax::Rect rect(win->dimension.GetDrawingRect());
				//				rect.position.x -= 1;
				//				rect.position.y -= 1;
				rect.position -= ax::Point(2, 2);
				rect.size += ax::Size(4, 4);

				ax::Color color(at::Skin::GetInstance()->data.common_at_yellow);
				//				gc.SetColor(color);
				//				gc.SetColor(color, 0.1);
				//				gc.DrawRectangleContour(rect);

				//				gc.SetColor(color, 0.5);
				//				gc.DrawRectangleContour(rect.GetInteriorRect(ax::Point(0, 0)));
				//
				gc.SetColor(color, 0.7);
				gc.DrawRectangleContour(rect);
				//
				gc.SetColor(color, 1.0);
				gc.DrawRectangleContour(rect.GetInteriorRect(ax::Point(1, 1)));
			}
		});
	}

	void PythonCallEmpty(const std::string& fct_name)
	{
		std::string fct_call = fct_name + "();\n";
		PyoAudio::GetInstance()->ProcessString(fct_call);
	}

	void PythonCallReal(const std::string& fct_name, double value)
	{
		std::string fct_call = fct_name + "(";
		fct_call += std::to_string(value) + ");\n";
		PyoAudio::GetInstance()->ProcessString(fct_call);
	}

	void Loader::SetupPyoComponent(ax::Window* win, const std::string& fct_name)
	{
		auto comp = pyo::Component::Ptr(new pyo::Component(win));
		comp->SetFunctionName(fct_name);
		win->component.Add("pyo", comp);
	}

	void Loader::SetupUniqueNameComponent(ax::Window* win, const std::string& name)
	{
		auto comp = at::UniqueNameComponent::Ptr(new at::UniqueNameComponent(win));
		comp->SetName(name);
		win->component.Add("unique_name", comp);
	}

	void Loader::SetupButtonPyoEvent(ax::Window* win)
	{
		win->AddConnection(ax::Button::Events::BUTTON_CLICK, ax::event::Function([win](ax::event::Msg* msg) {
			if (win->component.Has("pyo")) {
				pyo::Component::Ptr comp = win->component.Get<pyo::Component>("pyo");
				const std::string fct_name = comp->GetFunctionName();

				if (!fct_name.empty()) {
					PythonCallEmpty(fct_name);
				}
			}
		}));
	}

	void Loader::SetupTogglePyoEvent(ax::Window* win)
	{
		win->AddConnection(ax::Toggle::Events::BUTTON_CLICK, ax::event::Function([win](ax::event::Msg* msg) {
			if (win->component.Has("pyo")) {
				pyo::Component::Ptr comp = win->component.Get<pyo::Component>("pyo");
				const std::string fct_name = comp->GetFunctionName();

				if (!fct_name.empty()) {
					PythonCallEmpty(fct_name);
				}
			}
		}));
	}

	void Loader::SetupKnobPyoEvent(ax::Window* win)
	{
		win->AddConnection(0, ax::event::Function([win](ax::event::Msg* msg) {
			if (win->component.Has("pyo")) {
				pyo::Component::Ptr comp = win->component.Get<pyo::Component>("pyo");
				const std::string fct_name = comp->GetFunctionName();

				if (!fct_name.empty()) {
					ax::Knob::Msg* kmsg = static_cast<ax::Knob::Msg*>(msg);
					PythonCallReal(fct_name, kmsg->GetValue());
				}
			}
		}));
	}

	void Loader::SetupSliderPyoEvent(ax::Window* win)
	{
		win->AddConnection(0, ax::event::Function([win](ax::event::Msg* msg) {
			if (win->component.Has("pyo")) {
				pyo::Component::Ptr comp = win->component.Get<pyo::Component>("pyo");
				const std::string fct_name = comp->GetFunctionName();

				if (!fct_name.empty()) {
					ax::Slider::Msg* kmsg = static_cast<ax::Slider::Msg*>(msg);
					PythonCallReal(fct_name, 1.0 - kmsg->GetValue());
				}
			}
		}));
	}
}
}