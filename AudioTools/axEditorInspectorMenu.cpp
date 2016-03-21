#include "PyoComponent.h"
#include "atEditorInspectorMenu.h"
#include "atCommon.h"
#include "atMenuAttribute.h"

namespace at {
namespace editor {
	MenuSeparator::MenuSeparator(const ax::Rect& rect, const std::string& name)
		: _name(name)
		, _font("fonts/FreeSansBold.ttf")
	{
		win = ax::Window::Create(rect);
		win->event.OnPaint = ax::WBind<ax::GC>(this, &MenuSeparator::OnPaint);
	}

	void MenuSeparator::OnPaint(ax::GC gc)
	{
		ax::Rect rect(win->dimension.GetDrawingRect());

		gc.SetColor(ax::Color(0.94));
		gc.DrawRectangle(rect);

		gc.SetColor(ax::Color(0.3));
		gc.DrawString(_font, _name, ax::Point(10, 2));

		gc.SetColor(ax::Color(0.94));
		gc.DrawRectangleContour(rect);
	}

	/*
	 * InspectorMenu.
	 */
	InspectorMenu::InspectorMenu(const ax::Rect& rect)
		: _selected_handle(nullptr)
		, _font("fonts/Lato.ttf")
	{
		// Create window.
		win = ax::Window::Create(rect);
		win->event.OnPaint = ax::WBind<ax::GC>(this, &InspectorMenu::OnPaint);
	}

	void InspectorMenu::SetWidgetHandle(ax::Window* handle)
	{
		// Clear old content.
		RemoveHandle();
		_selected_handle = handle;

		if (_selected_handle) {
			ax::Rect rect(win->dimension.GetRect());

			ax::Size separator_size(rect.size.x, 20);

			// Add widget separator.
			win->node.Add(ax::shared<MenuSeparator>(ax::Rect(ax::Point(0, 30), separator_size), "Widget"));

			ax::widget::Component::Ptr widget
				= _selected_handle->component.Get<ax::widget::Component>("Widget");
			ax::widget::Info* info = widget->GetInfo();

			ax::StringVector info_atts = info->GetParamNameList();
			ax::StringPairVector atts_pair = widget->GetBuilderAttributes();

			ax::Point att_pos(0, 50);
			ax::Size att_size(rect.size.x, 20);

			for (auto& n : atts_pair) {
				win->node.Add(ax::shared<at::inspector::MenuAttribute>(
					ax::Rect(att_pos, att_size), n.first, n.second, GetOnWidgetUpdate()));
				att_pos.y += att_size.y;
			}

			win->node.Add(ax::shared<MenuSeparator>(ax::Rect(att_pos, separator_size), "Info"));

			att_pos.y += separator_size.y;

			for (auto& n : info_atts) {
				std::string value = info->GetAttributeValue(n);
				win->node.Add(ax::shared<at::inspector::MenuAttribute>(
					ax::Rect(att_pos, att_size), n, value, GetOnInfoUpdate()));
				att_pos.y += att_size.y;
			}

			// pyo function.
			if (_selected_handle->component.Has("pyo")) {
				pyo::Component::Ptr pyo_comp = _selected_handle->component.Get<pyo::Component>("pyo");

				win->node.Add(ax::shared<MenuSeparator>(ax::Rect(att_pos, separator_size), "Pyo"));

				att_pos.y += separator_size.y;

				std::string fct_name = pyo_comp->GetFunctionName();
				
				auto menu = ax::shared<at::inspector::MenuAttribute>(
					ax::Rect(att_pos, att_size), "callback", fct_name, GetOnPyoCallback());
				win->node.Add(menu);

				att_pos.y += att_size.y;
			}
		}
		win->Update();
	}

	void InspectorMenu::RemoveHandle()
	{
		if (_selected_handle != nullptr) {
			win->node.GetChildren().clear();
		}
		_selected_handle = nullptr;
	}

	void InspectorMenu::OnPyoCallback(const ax::Event::SimpleMsg<ax::StringPair>& msg)
	{
		ax::Print("Pyocallback");
		if (_selected_handle == nullptr) {
			return;
		}

		if (msg.GetMsg().first == "callback") {
			if (!_selected_handle->component.Has("pyo")) {
				return;
			}

			/// @todo Check string before.
			pyo::Component::Ptr pyo_comp = _selected_handle->component.Get<pyo::Component>("pyo");
			pyo_comp->SetFunctionName(msg.GetMsg().second);
			return;
		}
	}

	void InspectorMenu::OnWidgetUpdate(const ax::Event::SimpleMsg<ax::StringPair>& msg)
	{
		ax::Print("Pyocallback");
		if (_selected_handle == nullptr) {
			return;
		}

		if (_selected_handle->component.Has("Widget")) {
			ax::widget::Component::Ptr widget
				= _selected_handle->component.Get<ax::widget::Component>("Widget");

			widget->SetBuilderAttributes(ax::StringPairVector{ msg.GetMsg() });
		}
	}

	void InspectorMenu::OnInfoUpdate(const ax::Event::SimpleMsg<ax::StringPair>& msg)
	{
		ax::Print("Pyocallback");
		if (_selected_handle == nullptr) {
			return;
		}
		ax::widget::Component::Ptr widget
		= _selected_handle->component.Get<ax::widget::Component>("Widget");
		
		widget->SetInfo(ax::StringPairVector{ msg.GetMsg() });
		widget->ReloadInfo();
		
	}

	void InspectorMenu::OnPaint(ax::GC gc)
	{
		ax::Rect rect(win->dimension.GetDrawingRect());

		gc.SetColor(ax::Color(255, 255, 255));
		gc.DrawRectangle(rect);

		gc.SetColor(ax::Color(0.7));
		gc.DrawRectangleContour(rect);
	}
}
}