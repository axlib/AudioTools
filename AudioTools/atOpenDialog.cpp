#include "atOpenDialog.hpp"

#include <OpenAX/Core.h>
#include <OpenAX/OSFileSystem.h>
#include <OpenAX/Toggle.h>

namespace at {
namespace editor {
	OpenDialog::OpenDialog(const ax::Rect& rect)
	{
		// Create window.
		win = ax::Window::Create(rect);
		win->event.OnPaint = ax::WBind<ax::GC>(this, &OpenDialog::OnPaint);
		win->event.OnGlobalClick = ax::WBind<ax::Window::Event::GlobalClick>(
			this, &OpenDialog::OnGlobalClick);
		win->event.OnMouseLeftDown
			= ax::WBind<ax::Point>(this, &OpenDialog::OnMouseLeftDown);

		ax::App::GetInstance().GetWindowManager()->AddGlobalClickListener(win);

		ax::DropMenu::Info menu_info;
		menu_info.normal = ax::Color(240, 240, 240);
		menu_info.hover = ax::Color(246, 246, 246);
		menu_info.font_color = ax::Color(0.0);
		menu_info.selected = ax::Color(41, 222, 255);
		menu_info.selected_hover = ax::Color(41, 226, 255);
		menu_info.selected_font_color = ax::Color(0.0);
		menu_info.contour = ax::Color(0.86);
		menu_info.separation = ax::Color(0.86);
		menu_info.up_down_arrow = ax::Color(0.35);
		menu_info.right_arrow = ax::Color(0.70);
		menu_info.item_height = 35;

		ax::os::Directory dir;
		dir.Goto("layouts/");

		std::vector<ax::os::File> files = dir.GetContent();

		ax::StringVector layout_files;

		for (auto& n : files) {
			//		ax::Print(n.name);
			if (n.ext == "xml") {
				layout_files.push_back(n.name);
			}
		}

		for (auto& n : layout_files) {
			ax::Print(n);
		}

		//	ax::Size fsize(ax::App::GetInstance().GetFrameSize());
		ax::Size size(150, 300);

		_menu = ax::shared<ax::DropMenu>(
			ax::Rect(ax::Point(rect.position.x, 0), size), GetOnMenuSelection(),
			menu_info, layout_files);

		win->node.Add(_menu);

		ax::Size menu_size(_menu->GetWindow()->dimension.GetSize());

		auto open = ax::shared<ax::Button>(
			ax::Rect(rect.position.x, menu_size.y, size.x * 0.5, 30),
			GetOnOpen(), ax::Button::Info(), "", "Open");

		auto cancel = ax::shared<ax::Button>(
			ax::Rect(ax::Point(size.x * 0.5, menu_size.y),
				ax::Size(size.x * 0.5, 30)),
			GetOnCancel(), ax::Button::Info(), "", "Cancel");

		win->node.Add(open);
		win->node.Add(cancel);

		win->dimension.SetPosition(ax::Point(0, rect.position.y));

		//	win->dimension.SetSize(ax::Size(menu_size.x, menu_size.y + 30));

		//
		//		std::string font_path;
		//		ax::TextBox::Info txt_info;
		//		txt_info.normal = ax::Color(0.85);
		//		txt_info.hover = ax::Color(0.85);
		//		txt_info.highlight = ax::Color(0.6, 0.2); // This needs to be
		// transparent (alpha < 1.0).
		//		txt_info.selected = ax::Color(0.85);
		//		txt_info.selected_shadow = ax::Color(0.85);
		//		txt_info.cursor = ax::Color(0.0);
		//		txt_info.contour = ax::Color(0.3);;
		//		txt_info.font_color = ax::Color(0.0);
		//
		//		_txtBox = ax::shared<ax::TextBox>(ax::Rect(10, 10, 200, 30),
		// ax::TextBox::Events(), txt_info, "",
		//"default.xml");
		//		win->node.Add(_txtBox);
		//
		//		auto save = ax::shared<ax::Button>(ax::Rect(10, 50, 60, 30),
		// GetOnSave(), ax::Button::Info(), "",
		//"Save");
		//		auto cancel = ax::shared<ax::Button>(ax::Rect(10, 120, 60, 30),
		// GetOnCancel(), ax::Button::Info(),
		//"", "Cancel");
		//
		//		win->node.Add(save);
		//		win->node.Add(cancel);
	}

	void OpenDialog::OnGlobalClick(const ax::Window::Event::GlobalClick& gclick)
	{
		DeleteDialog();
	}

	void OpenDialog::OnOpen(const ax::Button::Msg& msg)
	{
		//		std::string label = _txtBox->GetLabel();
		//		ax::Print(label);
		win->PushEvent(
			OPEN, new ax::Event::StringMsg(_menu->GetSelectedItem()));
		DeleteDialog();
	}

	void OpenDialog::OnCancel(const ax::Button::Msg& msg)
	{
		win->PushEvent(CANCEL, new ax::Event::StringMsg(""));
		DeleteDialog();
	}

	void OpenDialog::OnMenuSelection(const ax::DropMenu::Msg& msg)
	{
		win->PushEvent(OPEN, new ax::Event::StringMsg(msg.GetItem()));
		DeleteDialog();
	}

	void OpenDialog::DeleteDialog()
	{
		ax::App::GetInstance().GetWindowManager()->RemoveGlobalClickListener(
			win);
		ax::App::GetInstance().GetWindowManager()->SetPastWindow(nullptr);
		ax::App::GetInstance().GetWindowManager()->UnGrabKey();
		ax::App::GetInstance().GetWindowManager()->UnGrabMouse();

		win->event.UnGrabKey();
		win->event.UnGrabMouse();

		win->backbone = nullptr;
		
		ax::App::GetInstance()
			.GetPopupManager()
			->GetWindowTree()
			->GetNodeVector()
			.clear();
		
		ax::App::GetInstance().GetPopupManager()->SetPastWindow(nullptr);
		ax::Print("Delete window.");

		ax::App::GetInstance().UpdateAll();
	}

	void OpenDialog::OnMouseLeftDown(const ax::Point& pos)
	{
		//	win->PushEvent(CANCEL, new ax::Event::StringMsg(""));
		DeleteDialog();
	}

	void OpenDialog::OnPaint(ax::GC gc)
	{
		ax::Rect rect(win->dimension.GetDrawingRect());

		gc.SetColor(ax::Color(0.0, 0.6));
		gc.DrawRectangle(rect);
	}
}
}