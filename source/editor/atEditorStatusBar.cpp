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

#include "editor/atEditorStatusBar.hpp"
#include "PyoAudio.h"
#include "atCommon.h"
#include "atHelpBar.h"
#include "atOpenDialog.hpp"
#include "atPreferenceDialog.h"
#include "atSaveDialog.hpp"
#include "atSkin.hpp"
#include "editor/atEditor.hpp"
#include "editor/atEditorMainWindow.hpp"

#include <axlib/Core.hpp>
#include <axlib/Toggle.hpp>
#include <axlib/WindowManager.hpp>

namespace at {
namespace editor {
	StatusBar::StatusBar(const ax::Rect& rect)
		: _font(0)
	{
		// Create window.
		win = ax::Window::Create(rect);
		win->event.OnPaint = ax::WBind<ax::GC>(this, &StatusBar::OnPaint);
		win->event.OnResize = ax::WBind<ax::Size>(this, &StatusBar::OnResize);

		// Transparent toggle with image.
		ax::Toggle::Info tog_info;
		tog_info.normal = ax::Color(0.0, 0.0);
		tog_info.hover = ax::Color(0.0, 0.0);
		tog_info.clicking = ax::Color(0.0, 0.0);
		tog_info.selected = ax::Color(0.0, 0.0);
		tog_info.font_color = ax::Color(0.0, 0.0);

		tog_info.selected = ax::Color(0.0, 0.0);
		tog_info.selected_hover = ax::Color(0.0, 0.0);
		tog_info.selected_clicking = ax::Color(0.0, 0.0);
		tog_info.selected = ax::Color(0.0, 0.0);
		tog_info.selected_font_color = ax::Color(0.0, 0.0);

		tog_info.contour = ax::Color(0.0, 0.0);
		tog_info.font_size = 12;
		tog_info.img = "resources/top_menu_toggle_left.png";
		tog_info.single_img = false;

		ax::Point pos(rect.size.w - 95, 2);

		// Volume meter left.
		ax::Rect volume_rect(rect.size.w - 165, 8, 50, 7);
		auto v_meter_l = ax::shared<at::VolumeMeter>(volume_rect);
		win->node.Add(v_meter_l);
		_volumeMeterLeft = v_meter_l.get();

		// Volume meter right.
		volume_rect.position = volume_rect.GetNextPosDown(0);
		auto v_meter_r = ax::shared<at::VolumeMeter>(volume_rect);
		win->node.Add(v_meter_r);
		_volumeMeterRight = v_meter_r.get();

		// Connect volume meter event.
		win->AddConnection(PyoAudio::Events::RMS_VALUE_CHANGE, GetOnAudioRmsValue());
		PyoAudio::GetInstance()->SetConnectedObject(win);

		//		auto midi_feedback
		//			= ax::shared<at::MidiFeedback>(ax::Rect(volume_rect.GetNextPosRight(5), ax::Size(12,
		// 12)));
		//		_midi_feedback = midi_feedback.get();
		//		win->node.Add(midi_feedback);

		const ax::Size tog_size(25, 25);

		// Left panel toggle.
		auto tog_left = ax::shared<ax::Toggle>(ax::Rect(pos, tog_size), GetOnToggleLeftPanel(), tog_info);
		AttachHelpInfo(tog_left->GetWindow(), "Show / Hide widget menu.");

		// Bottom panel toggle.
		pos = tog_left->GetWindow()->dimension.GetRect().GetNextPosRight(5);
		tog_info.img = "resources/top_menu_toggle_bottom.png";

		auto tog_middle = ax::shared<ax::Toggle>(ax::Rect(pos, tog_size), GetOnToggleBottomPanel(), tog_info);
		AttachHelpInfo(tog_middle->GetWindow(), "Show / Hide code editor.");

		// Right panel toggle.
		pos = tog_middle->GetWindow()->dimension.GetRect().GetNextPosRight(5);
		tog_info.img = "resources/top_menu_toggle_right.png";

		auto tog_right = ax::shared<ax::Toggle>(ax::Rect(pos, tog_size), GetOnToggleRightPanel(), tog_info);
		AttachHelpInfo(tog_right->GetWindow(), "Show / Hide inspector menu.");

		tog_left->SetSelected(true);
		tog_middle->SetSelected(true);
		tog_right->SetSelected(true);

		win->node.Add(tog_left);
		win->node.Add(tog_middle);
		win->node.Add(tog_right);

		_toggle_left = tog_left.get();
		_toggle_bottom = tog_middle.get();
		_toggle_right = tog_right.get();

		ax::Button::Info btn_info;
		btn_info.normal = ax::Color(0.30);
		btn_info.hover = ax::Color(0.34);
		btn_info.clicking = ax::Color(0.32);
		btn_info.selected = ax::Color(0.30);
		btn_info.contour = ax::Color(0.30);
		btn_info.font_color = ax::Color(1.0);

		// Open button.
		pos = ax::Point(5, 2);
		auto create_btn = ax::shared<ax::Button>(ax::Rect(pos, ax::Size(25, 25)), GetOnCreateNewLayout(),
			btn_info, "resources/create.png", "", ax::Button::Flags::SINGLE_IMG);
		win->node.Add(create_btn);
		AttachHelpInfo(create_btn->GetWindow(), "Create new layout file.");
		pos = create_btn->GetWindow()->dimension.GetRect().GetNextPosRight(5);

		auto open_menu = ax::shared<ax::Button>(ax::Rect(pos, ax::Size(25, 25)), GetOnOpenLayout(), btn_info,
			"resources/folder.png", "", ax::Button::Flags::SINGLE_IMG);
		win->node.Add(open_menu);

		AttachHelpInfo(open_menu->GetWindow(), "Open layout file.");

		// Save button.
		pos = open_menu->GetWindow()->dimension.GetRect().GetNextPosRight(5);
		auto save_btn = ax::shared<ax::Button>(ax::Rect(pos, ax::Size(25, 25)), GetOnSaveLayout(), btn_info,
			"resources/save.png", "", ax::Button::Flags::SINGLE_IMG);
		win->node.Add(save_btn);

		AttachHelpInfo(save_btn->GetWindow(), "Save current layout file.");

		// Save as button.
		pos = save_btn->GetWindow()->dimension.GetRect().GetNextPosRight(5);
		auto save_as_btn = ax::shared<ax::Button>(ax::Rect(pos, ax::Size(25, 25)), GetOnSaveAsLayout(),
			btn_info, "resources/save_as.png", "", ax::Button::Flags::SINGLE_IMG);
		win->node.Add(save_as_btn);

		AttachHelpInfo(save_as_btn->GetWindow(), "Save as current layout file.");

		// View button.
		pos = save_as_btn->GetWindow()->dimension.GetRect().GetNextPosRight(5);
		auto view_btn = ax::shared<ax::Button>(ax::Rect(pos, ax::Size(25, 25)), GetOnViewLayout(), btn_info,
			"resources/view.png", "", ax::Button::Flags::SINGLE_IMG);
		win->node.Add(view_btn);

		AttachHelpInfo(view_btn->GetWindow(), "Switch layout to view mode.");

		// Settings button.
		pos = view_btn->GetWindow()->dimension.GetRect().GetNextPosRight(5);
		auto settings_btn = ax::shared<ax::Button>(ax::Rect(pos, ax::Size(25, 25)), GetOnSettings(), btn_info,
			"resources/settings.png", "", ax::Button::Flags::SINGLE_IMG);
		win->node.Add(settings_btn);

		AttachHelpInfo(settings_btn->GetWindow(), "Open preference.");

		// Play / Refresh button.
		pos = settings_btn->GetWindow()->dimension.GetRect().GetNextPosRight(5);
		auto refresh_btn = ax::shared<ax::Button>(ax::Rect(pos, ax::Size(25, 25)), GetOnReload(), btn_info,
			"resources/play.png", "", ax::Button::Flags::SINGLE_IMG);
		win->node.Add(refresh_btn);

		AttachHelpInfo(refresh_btn->GetWindow(), "Start script interpreter.");

		// Stop button.
		pos = refresh_btn->GetWindow()->dimension.GetRect().GetNextPosRight(5);
		auto stop_btn = ax::shared<ax::Button>(ax::Rect(pos, ax::Size(25, 25)), GetOnStop(), btn_info,
			"resources/stop.png", "", ax::Button::Flags::SINGLE_IMG);
		win->node.Add(stop_btn);

		AttachHelpInfo(stop_btn->GetWindow(), "Stop script interpreter.");
	}

	void StatusBar::OnSaveLayout(const ax::Button::Msg& msg)
	{
		win->PushEvent(SAVE_LAYOUT, new ax::event::StringMsg(_layout_file_path));
	}

	void StatusBar::OnSaveAsLayout(const ax::Button::Msg& msg)
	{
		std::string filepath = ax::App::GetInstance().SaveFileDialog();
		ax::console::Print("filepath :", filepath);
		win->PushEvent(SAVE_AS_LAYOUT, new ax::event::StringMsg(filepath));
	}

	void StatusBar::OnOpenLayout(const ax::Button::Msg& msg)
	{
		std::string filepath = ax::App::GetInstance().OpenFileDialog();
		ax::console::Print("File :", filepath);

		win->PushEvent(OPEN_LAYOUT, new ax::event::StringMsg(filepath));
	}

	void StatusBar::OnCreateNewLayout(const ax::Button::Msg& msg)
	{
		std::string filepath = ax::App::GetInstance().SaveFileDialog();
		ax::console::Print("File :", filepath);

		win->PushEvent(CREATE_NEW_LAYOUT, new ax::event::StringMsg(filepath));
	}

	void StatusBar::OnViewLayout(const ax::Button::Msg& msg)
	{
		ax::console::Print("On view layout.");
		win->PushEvent(VIEW_LAYOUT, new ax::event::SimpleMsg<int>(0));
	}

	void StatusBar::OnReload(const ax::Button::Msg& msg)
	{
		ax::console::Print("On reload script.");
		win->PushEvent(RELOAD_SCRIPT, new ax::event::SimpleMsg<int>(0));
	}

	void StatusBar::OnStop(const ax::Button::Msg& msg)
	{
		ax::console::Print("On stop script.");
		win->PushEvent(STOP_SCRIPT, new ax::event::SimpleMsg<int>(0));
	}

	void StatusBar::OnSettings(const ax::Button::Msg& msg)
	{
		ax::App& app(ax::App::GetInstance());
		app.GetPopupManager()->Clear();

		const ax::Rect rect = msg.GetSender()->GetWindow()->dimension.GetAbsoluteRect();
		ax::Point pos(0, rect.position.y + rect.size.h);
		ax::Size size = app.GetFrameSize();
		size.h -= rect.size.h;

		auto pref_dialog = ax::shared<PreferenceDialog>(ax::Rect(pos, size));
		app.AddPopupTopLevel(pref_dialog);
		app.UpdateAll();

		//		ax::App::GetInstance().GetPopupManager()->GetWindowTree()->AddTopLevel(
		//			std::shared_ptr<ax::Window>(pref_dialog->GetWindow()));

		//		pref_dialog->GetWindow()->backbone = pref_dialog;
	}

	void StatusBar::OnToggleLeftPanel(const ax::Toggle::Msg& msg)
	{
		win->PushEvent(TOGGLE_LEFT_PANEL, new ax::Toggle::Msg(msg));
	}

	void StatusBar::OnToggleBottomPanel(const ax::Toggle::Msg& msg)
	{
		win->PushEvent(TOGGLE_BOTTOM_PANEL, new ax::Toggle::Msg(msg));
	}

	void StatusBar::OnToggleRightPanel(const ax::Toggle::Msg& msg)
	{
		win->PushEvent(TOGGLE_RIGHT_PANEL, new ax::Toggle::Msg(msg));
	}

	void StatusBar::OnSaveDialog(const ax::event::StringMsg& msg)
	{
		win->PushEvent(SAVE_LAYOUT, new ax::event::StringMsg(msg));
	}

	void StatusBar::OnOpenDialog(const ax::event::StringMsg& msg)
	{
		win->PushEvent(OPEN_LAYOUT, new ax::event::StringMsg(msg));
	}

	void StatusBar::OnCancelDialog(const ax::event::StringMsg& msg)
	{
	}

	void StatusBar::OnAudioRmsValue(const ax::event::SimpleMsg<StereoRmsValue>& msg)
	{
		_volumeMeterLeft->SetValue(msg.GetMsg().first);
		_volumeMeterRight->SetValue(msg.GetMsg().second);
	}

	void StatusBar::OnResize(const ax::Size& size)
	{
		// Repos left volume meter.
		_volumeMeterLeft->GetWindow()->dimension.SetPosition(ax::Point(size.w - 165, 8));

		// Repos right volume meter.
		_volumeMeterRight->GetWindow()->dimension.SetPosition(
			_volumeMeterLeft->GetWindow()->dimension.GetRect().GetNextPosDown(0));

		// Left toggle.
		ax::Point pos(size.w - 95, 2);
		_toggle_left->GetWindow()->dimension.SetPosition(pos);
		pos = _toggle_left->GetWindow()->dimension.GetRect().GetNextPosRight(5);

		// Middle toggle.
		_toggle_bottom->GetWindow()->dimension.SetPosition(pos);
		pos = _toggle_bottom->GetWindow()->dimension.GetRect().GetNextPosRight(5);

		// Right toggle.
		_toggle_right->GetWindow()->dimension.SetPosition(pos);
	}

	void StatusBar::OnPaint(ax::GC gc)
	{
		const ax::Rect rect(win->dimension.GetDrawingRect());

		gc.SetColor(at::Skin::GetInstance()->data.status_bar_bg);
		gc.DrawRectangle(rect);

		if (!_layout_file_path.empty()) {
			gc.SetColor(at::Skin::GetInstance()->data.status_bar_text);
			gc.DrawStringAlignedCenter(_font, _layout_file_path, rect);
		}

		gc.SetColor(at::Skin::GetInstance()->data.status_bar_bg);
		gc.DrawRectangleContour(rect);
	}
}
}