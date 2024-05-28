///|/ Copyright (c) Prusa Research 2018 - 2023 Oleksandra Iushchenko @YuSanka, Enrico Turri @enricoturri1966, Vojtěch Bubník @bubnikv, David Kocík @kocikdav, Lukáš Matěna @lukasmatena, Tomáš Mészáros @tamasmeszaros, Vojtěch Král @vojtechkral
///|/ Copyright (c) 2020 Benjamin Greiner
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "wxExtensions.hpp"

#include <stdexcept>
#include <cmath>

#include <wx/sizer.h>

#include <boost/algorithm/string/replace.hpp>

#include "BitmapCache.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "GUI_ObjectList.hpp"
#include "I18N.hpp"
#include "GUI_Utils.hpp"
#include "Plater.hpp"
#include "../Utils/MacDarkMode.hpp"
#include "BitmapComboBox.hpp"
#include "libslic3r/Utils.hpp"
#include "OG_CustomCtrl.hpp"
#include "format.hpp"

#include "libslic3r/Color.hpp"

#ifndef __linux__
// msw_menuitem_bitmaps is used for MSW and OSX
static std::map<int, std::string> msw_menuitem_bitmaps;
void sys_color_changed_menu(wxMenu* menu)
{
	struct update_icons {
		static void run(wxMenuItem* item) {
			const auto it = msw_menuitem_bitmaps.find(item->GetId());
			if (it != msw_menuitem_bitmaps.end()) {
				wxBitmapBundle* item_icon = get_bmp_bundle(it->second);
				if (item_icon->IsOk())
					item->SetBitmap(*item_icon);
			}
			if (item->IsSubMenu())
				for (wxMenuItem *sub_item : item->GetSubMenu()->GetMenuItems())
					update_icons::run(sub_item);
		}
	};

	for (wxMenuItem *item : menu->GetMenuItems())
		update_icons::run(item);
}
#endif /* no __linux__ */

void enable_menu_item(wxUpdateUIEvent& evt, std::function<bool()> const cb_condition, wxMenuItem* item, wxWindow* win)
{
    const bool enable = cb_condition();
    evt.Enable(enable);
}

wxMenuItem* append_menu_item(wxMenu* menu, int id, const wxString& string, const wxString& description,
    std::function<void(wxCommandEvent& event)> cb, wxBitmapBundle* icon, wxEvtHandler* event_handler,
    std::function<bool()> const cb_condition, wxWindow* parent, int insert_pos/* = wxNOT_FOUND*/)
{
    if (id == wxID_ANY)
        id = wxNewId();

    auto *item = new wxMenuItem(menu, id, string, description);
    if (icon && icon->IsOk()) {
        item->SetBitmap(*icon);
    }
    if (insert_pos == wxNOT_FOUND)
        menu->Append(item);
    else
        menu->Insert(insert_pos, item);

#ifdef __WXMSW__
    if (event_handler != nullptr && event_handler != menu)
        event_handler->Bind(wxEVT_MENU, cb, id);
    else
#endif // __WXMSW__
        menu->Bind(wxEVT_MENU, cb, id);

    if (parent) {
        parent->Bind(wxEVT_UPDATE_UI, [cb_condition, item, parent](wxUpdateUIEvent& evt) {
            enable_menu_item(evt, cb_condition, item, parent); }, id);
    }

    return item;
}

wxMenuItem* append_menu_item(wxMenu* menu, int id, const wxString& string, const wxString& description,
    std::function<void(wxCommandEvent& event)> cb, const std::string& icon, wxEvtHandler* event_handler,
    std::function<bool()> const cb_condition, wxWindow* parent, int insert_pos/* = wxNOT_FOUND*/)
{
    if (id == wxID_ANY)
        id = wxNewId();

    wxBitmapBundle* bmp = icon.empty() ? nullptr : get_bmp_bundle(icon);

#ifndef __linux__
    if (bmp && bmp->IsOk())
        msw_menuitem_bitmaps[id] = icon;
#endif /* no __linux__ */

    return append_menu_item(menu, id, string, description, cb, bmp, event_handler, cb_condition, parent, insert_pos);
}

wxMenuItem* append_submenu(wxMenu* menu, wxMenu* sub_menu, int id, const wxString& string, const wxString& description, const std::string& icon,
    std::function<bool()> const cb_condition, wxWindow* parent)
{
    if (id == wxID_ANY)
        id = wxNewId();

    wxMenuItem* item = new wxMenuItem(menu, id, string, description);
    if (!icon.empty()) {
        item->SetBitmap(*get_bmp_bundle(icon));

#ifndef __linux__
        msw_menuitem_bitmaps[id] = icon;
#endif // no __linux__
    }

    item->SetSubMenu(sub_menu);
    menu->Append(item);

    if (parent) {
        parent->Bind(wxEVT_UPDATE_UI, [cb_condition, item, parent](wxUpdateUIEvent& evt) {
            enable_menu_item(evt, cb_condition, item, parent); }, id);
    }

    return item;
}

wxMenuItem* append_menu_radio_item(wxMenu* menu, int id, const wxString& string, const wxString& description,
    std::function<void(wxCommandEvent& event)> cb, wxEvtHandler* event_handler)
{
    if (id == wxID_ANY)
        id = wxNewId();

    wxMenuItem* item = menu->AppendRadioItem(id, string, description);

#ifdef __WXMSW__
    if (event_handler != nullptr && event_handler != menu)
        event_handler->Bind(wxEVT_MENU, cb, id);
    else
#endif // __WXMSW__
        menu->Bind(wxEVT_MENU, cb, id);

    return item;
}

wxMenuItem* append_menu_check_item(wxMenu* menu, int id, const wxString& string, const wxString& description,
    std::function<void(wxCommandEvent & event)> cb, wxEvtHandler* event_handler,
    std::function<bool()> const enable_condition, std::function<bool()> const check_condition, wxWindow* parent)
{
    if (id == wxID_ANY)
        id = wxNewId();

    wxMenuItem* item = menu->AppendCheckItem(id, string, description);

#ifdef __WXMSW__
    if (event_handler != nullptr && event_handler != menu)
        event_handler->Bind(wxEVT_MENU, cb, id);
    else
#endif // __WXMSW__
        menu->Bind(wxEVT_MENU, cb, id);

    if (parent)
        parent->Bind(wxEVT_UPDATE_UI, [enable_condition, check_condition](wxUpdateUIEvent& evt)
            {
                evt.Enable(enable_condition());
                evt.Check(check_condition());
            }, id);

    return item;
}

const unsigned int wxCheckListBoxComboPopup::DefaultWidth = 200;
const unsigned int wxCheckListBoxComboPopup::DefaultHeight = 200;

bool wxCheckListBoxComboPopup::Create(wxWindow* parent)
{
    return wxCheckListBox::Create(parent, wxID_HIGHEST + 1, wxPoint(0, 0));
}

wxWindow* wxCheckListBoxComboPopup::GetControl()
{
    return this;
}

void wxCheckListBoxComboPopup::SetStringValue(const wxString& value)
{
    m_text = value;
}

wxString wxCheckListBoxComboPopup::GetStringValue() const
{
    return m_text;
}

wxSize wxCheckListBoxComboPopup::GetAdjustedSize(int minWidth, int prefHeight, int maxHeight)
{
    // set width dinamically in dependence of items text
    // and set height dinamically in dependence of items count

    wxComboCtrl* cmb = GetComboCtrl();
    if (cmb != nullptr) {
        wxSize size = GetComboCtrl()->GetSize();

        unsigned int count = GetCount();
        if (count > 0) {
            int max_width = size.x;
            for (unsigned int i = 0; i < count; ++i) {
                max_width = std::max(max_width, 60 + GetTextExtent(GetString(i)).x);
            }
            size.SetWidth(max_width);
            size.SetHeight(count * cmb->GetCharHeight());
        }
        else
            size.SetHeight(DefaultHeight);

        return size;
    }
    else
        return wxSize(DefaultWidth, DefaultHeight);
}

void wxCheckListBoxComboPopup::OnKeyEvent(wxKeyEvent& evt)
{
    // filters out all the keys which are not working properly
    switch (evt.GetKeyCode())
    {
    case WXK_LEFT:
    case WXK_UP:
    case WXK_RIGHT:
    case WXK_DOWN:
    case WXK_PAGEUP:
    case WXK_PAGEDOWN:
    case WXK_END:
    case WXK_HOME:
    case WXK_NUMPAD_LEFT:
    case WXK_NUMPAD_UP:
    case WXK_NUMPAD_RIGHT:
    case WXK_NUMPAD_DOWN:
    case WXK_NUMPAD_PAGEUP:
    case WXK_NUMPAD_PAGEDOWN:
    case WXK_NUMPAD_END:
    case WXK_NUMPAD_HOME:
    {
        break;
    }
    default:
    {
        evt.Skip();
        break;
    }
    }
}

void wxCheckListBoxComboPopup::OnCheckListBox(wxCommandEvent& evt)
{
    // forwards the checklistbox event to the owner wxComboCtrl

    if (m_check_box_events_status == OnCheckListBoxFunction::FreeToProceed )
    {
        wxComboCtrl* cmb = GetComboCtrl();
        if (cmb != nullptr) {
            wxCommandEvent event(wxEVT_CHECKLISTBOX, cmb->GetId());
            event.SetEventObject(cmb);
            cmb->ProcessWindowEvent(event);
        }
    }

    evt.Skip();

    #ifndef _WIN32  // events are sent differently on OSX+Linux vs Win (more description in header file)
        if ( m_check_box_events_status == OnCheckListBoxFunction::RefuseToProceed )
            // this happens if the event was resent by OnListBoxSelection - next call to OnListBoxSelection is due to user clicking the text, so the function should
            // explicitly change the state on the checkbox
            m_check_box_events_status = OnCheckListBoxFunction::WasRefusedLastTime;
        else
            // if the user clicked the checkbox square, this event was sent before OnListBoxSelection was called, so we don't want it to resend it
            m_check_box_events_status = OnCheckListBoxFunction::RefuseToProceed;
    #endif
}

void wxCheckListBoxComboPopup::OnListBoxSelection(wxCommandEvent& evt)
{
    // transforms list box item selection event into checklistbox item toggle event 

    int selId = GetSelection();
    if (selId != wxNOT_FOUND)
    {
        #ifndef _WIN32
            if (m_check_box_events_status == OnCheckListBoxFunction::RefuseToProceed)
        #endif
                Check((unsigned int)selId, !IsChecked((unsigned int)selId));

        m_check_box_events_status = OnCheckListBoxFunction::FreeToProceed; // so the checkbox reacts to square-click the next time

        SetSelection(wxNOT_FOUND);
        wxCommandEvent event(wxEVT_CHECKLISTBOX, GetId());
        event.SetInt(selId);
        event.SetEventObject(this);
        ProcessEvent(event);
    }
}


// ***  wxDataViewTreeCtrlComboPopup  ***

const unsigned int wxDataViewTreeCtrlComboPopup::DefaultWidth = 270;
const unsigned int wxDataViewTreeCtrlComboPopup::DefaultHeight = 200;
const unsigned int wxDataViewTreeCtrlComboPopup::DefaultItemHeight = 22;

bool wxDataViewTreeCtrlComboPopup::Create(wxWindow* parent)
{
	return wxDataViewTreeCtrl::Create(parent, wxID_ANY/*HIGHEST + 1*/, wxPoint(0, 0), wxDefaultSize/*wxSize(270, -1)*/, wxDV_NO_HEADER);
}
/*
wxSize wxDataViewTreeCtrlComboPopup::GetAdjustedSize(int minWidth, int prefHeight, int maxHeight)
{
	// matches owner wxComboCtrl's width
	// and sets height dinamically in dependence of contained items count
	wxComboCtrl* cmb = GetComboCtrl();
	if (cmb != nullptr)
	{
		wxSize size = GetComboCtrl()->GetSize();
		if (m_cnt_open_items > 0)
			size.SetHeight(m_cnt_open_items * DefaultItemHeight);
		else
			size.SetHeight(DefaultHeight);

		return size;
	}
	else
		return wxSize(DefaultWidth, DefaultHeight);
}
*/
void wxDataViewTreeCtrlComboPopup::OnKeyEvent(wxKeyEvent& evt)
{
	// filters out all the keys which are not working properly
	if (evt.GetKeyCode() == WXK_UP)
	{
		return;
	}
	else if (evt.GetKeyCode() == WXK_DOWN)
	{
		return;
	}
	else
	{
		evt.Skip();
		return;
	}
}

void wxDataViewTreeCtrlComboPopup::OnDataViewTreeCtrlSelection(wxCommandEvent& evt)
{
	wxComboCtrl* cmb = GetComboCtrl();
	auto selected = GetItemText(GetSelection());
	cmb->SetText(selected);
}

// edit tooltip : change Slic3r to SLIC3R_APP_KEY
// Temporary workaround for localization
void edit_tooltip(wxString& tooltip)
{
    tooltip.Replace("Slic3r", SLIC3R_APP_KEY, true);
}

/* Function for rescale of buttons in Dialog under MSW if dpi is changed.
 * btn_ids - vector of buttons identifiers
 */
void msw_buttons_rescale(wxDialog* dlg, const int em_unit, const std::vector<int>& btn_ids, double height_koef/* = 1.*/)
{
    const wxSize& btn_size = wxSize(-1, int(2.5 * em_unit * height_koef + 0.5f));

    for (int btn_id : btn_ids) {
        // There is a case [FirmwareDialog], when we have wxControl instead of wxButton
        // so let casting everything to the wxControl
        wxControl* btn = static_cast<wxControl*>(dlg->FindWindowById(btn_id, dlg));
        if (btn)
            btn->SetMinSize(btn_size);
    }
}

/* Function for getting of em_unit value from correct parent.
 * In most of cases it is m_em_unit value from GUI_App,
 * but for DPIDialogs it's its own value. 
 * This value will be used to correct rescale after moving between 
 * Displays with different HDPI */
int em_unit(wxWindow* win)
{
    if (win)
    {
        wxTopLevelWindow *toplevel = Slic3r::GUI::find_toplevel_parent(win);
        Slic3r::GUI::DPIDialog* dlg = dynamic_cast<Slic3r::GUI::DPIDialog*>(toplevel);
        if (dlg)
            return dlg->em_unit();
        Slic3r::GUI::DPIFrame* frame = dynamic_cast<Slic3r::GUI::DPIFrame*>(toplevel);
        if (frame)
            return frame->em_unit();
    }
    
    return Slic3r::GUI::wxGetApp().em_unit();
}

int mode_icon_px_size()
{
#ifdef __APPLE__
    return 10;
#else
    return 12;
#endif
}

#ifdef __WXGTK2__
static int scale() 
{
    return int(em_unit(nullptr) * 0.1f + 0.5f);
}
#endif // __WXGTK2__

wxBitmapBundle* get_bmp_bundle(const std::string& bmp_name_in, int width/* = 16*/, int height/* = -1*/, const std::string& new_color/* = std::string()*/)
{
#ifdef __WXGTK2__
    width *= scale();
    if (height > 0)
        height *= scale();
#endif // __WXGTK2__

    static Slic3r::GUI::BitmapCache cache;

    std::string bmp_name = bmp_name_in;
    boost::replace_last(bmp_name, ".png", "");

    if (height < 0)
        height = width;

    // Try loading an SVG first, then PNG if SVG is not found:
    wxBitmapBundle* bmp = cache.from_svg(bmp_name, width, height, Slic3r::GUI::wxGetApp().dark_mode(), new_color);
    if (bmp == nullptr) {
        bmp = cache.from_png(bmp_name, width, height);
        if (!bmp)
            // Neither SVG nor PNG has been found, raise error
            throw Slic3r::RuntimeError("Could not load bitmap: " + bmp_name);
    }
    return bmp;
}

wxBitmapBundle* get_empty_bmp_bundle(int width, int height)
{
    static Slic3r::GUI::BitmapCache cache;
#ifdef __WXGTK2__
    return cache.mkclear_bndl(width * scale(), height * scale());
#else
    return cache.mkclear_bndl(width, height);
#endif // __WXGTK2__
}

wxBitmapBundle* get_solid_bmp_bundle(int width, int height, const std::string& color )
{
    static Slic3r::GUI::BitmapCache cache;
#ifdef __WXGTK2__
    return cache.mksolid_bndl(width * scale(), height * scale(), color, 1, Slic3r::GUI::wxGetApp().dark_mode());
#else
    return cache.mksolid_bndl(width, height, color, 1, Slic3r::GUI::wxGetApp().dark_mode());
#endif // __WXGTK2__
}

std::vector<wxBitmapBundle*> get_extruder_color_icons(bool thin_icon/* = false*/)
{
    // Create the bitmap with color bars.
    std::vector<wxBitmapBundle*> bmps;
    std::vector<std::string> colors = Slic3r::GUI::wxGetApp().plater()->get_extruder_colors_from_plater_config();

    if (colors.empty())
        return bmps;

    for (const std::string& color : colors)
        bmps.emplace_back(get_solid_bmp_bundle(thin_icon ? 16 : 32, 16, color));

    return bmps;
}


void apply_extruder_selector(Slic3r::GUI::BitmapComboBox** ctrl, 
                             wxWindow* parent,
                             const std::string& first_item/* = ""*/, 
                             wxPoint pos/* = wxDefaultPosition*/,
                             wxSize size/* = wxDefaultSize*/,
                             bool use_thin_icon/* = false*/)
{
    std::vector<wxBitmapBundle*> icons = get_extruder_color_icons(use_thin_icon);

    if (!*ctrl) {
        *ctrl = new Slic3r::GUI::BitmapComboBox(parent, wxID_ANY, wxEmptyString, pos, size, 0, nullptr, wxCB_READONLY);
        Slic3r::GUI::wxGetApp().UpdateDarkUI(*ctrl);
    }
    else
    {
        (*ctrl)->SetPosition(pos);
        (*ctrl)->SetMinSize(size);
        (*ctrl)->SetSize(size);
        (*ctrl)->Clear();
    }
    if (first_item.empty())
        (*ctrl)->Hide();    // to avoid unwanted rendering before layout (ExtruderSequenceDialog)

    if (icons.empty() && !first_item.empty()) {
        (*ctrl)->Append(_(first_item), wxNullBitmap);
        return;
    }

    // For ObjectList we use short extruder name (just a number)
    const bool use_full_item_name = dynamic_cast<Slic3r::GUI::ObjectList*>(parent) == nullptr;

    int i = 0;
    wxString str = _(L("Extruder"));
    for (wxBitmapBundle* bmp : icons) {
        if (i == 0) {
            if (!first_item.empty())
                (*ctrl)->Append(_(first_item), *bmp);
            ++i;
        }

        (*ctrl)->Append(use_full_item_name
                        ? Slic3r::GUI::from_u8((boost::format("%1% %2%") % str % i).str())
                        : wxString::Format("%d", i), *bmp);
        ++i;
    }
    (*ctrl)->SetSelection(0);
}


// ----------------------------------------------------------------------------
// LockButton
// ----------------------------------------------------------------------------

LockButton::LockButton( wxWindow *parent, 
                        wxWindowID id, 
                        const wxPoint& pos /*= wxDefaultPosition*/, 
                        const wxSize& size /*= wxDefaultSize*/):
                        wxButton(parent, id, wxEmptyString, pos, size, wxBU_EXACTFIT | wxNO_BORDER)
{
    m_bmp_lock_closed   = ScalableBitmap(this, "lock_closed");
    m_bmp_lock_closed_f = ScalableBitmap(this, "lock_closed_f");
    m_bmp_lock_open     = ScalableBitmap(this, "lock_open");
    m_bmp_lock_open_f   = ScalableBitmap(this, "lock_open_f");

    Slic3r::GUI::wxGetApp().UpdateDarkUI(this);
    SetBitmap(m_bmp_lock_open.bmp());
    SetBitmapDisabled(m_bmp_lock_open.bmp());
    SetBitmapCurrent(m_bmp_lock_closed_f.bmp());

    //button events
    Bind(wxEVT_BUTTON, &LockButton::OnButton, this);
}

void LockButton::OnButton(wxCommandEvent& event)
{
    if (m_disabled)
        return;

    SetLock(!m_is_pushed);
    event.Skip();
}

void LockButton::SetLock(bool lock)
{
    if (m_is_pushed != lock) {
        m_is_pushed = lock;
        update_button_bitmaps();
    }
}

void LockButton::sys_color_changed()
{
    Slic3r::GUI::wxGetApp().UpdateDarkUI(this);

    m_bmp_lock_closed.sys_color_changed();
    m_bmp_lock_closed_f.sys_color_changed();
    m_bmp_lock_open.sys_color_changed();
    m_bmp_lock_open_f.sys_color_changed();

    update_button_bitmaps();
}

void LockButton::update_button_bitmaps()
{
    SetBitmap(m_is_pushed ? m_bmp_lock_closed.bmp() : m_bmp_lock_open.bmp());
    SetBitmapCurrent(m_is_pushed ? m_bmp_lock_closed_f.bmp() : m_bmp_lock_open_f.bmp());

    Refresh();
    Update();
}



// ----------------------------------------------------------------------------
// ModeButton
// ----------------------------------------------------------------------------

ModeButton::ModeButton( wxWindow *          parent,
                        wxWindowID          id,
                        const std::string&  icon_name   /* = ""*/,
                        const wxString&     mode        /* = wxEmptyString*/,
                        const wxSize&       size        /* = wxDefaultSize*/,
                        const wxPoint&      pos         /* = wxDefaultPosition*/) :
    ScalableButton(parent, id, icon_name, mode, size, pos, wxBU_EXACTFIT)
{
    Init(mode);
}

ModeButton::ModeButton( wxWindow*           parent,
                        const wxString&     mode/* = wxEmptyString*/,
                        const std::string&  icon_name/* = ""*/,
                        int                 px_cnt/* = 16*/) :
    ScalableButton(parent, wxID_ANY, icon_name, mode, wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT, px_cnt)
{
    Init(mode);
}

ModeButton::ModeButton( wxWindow*           parent,
                        int                 mode_id,/*ConfigOptionMode*/
                        const wxString&     mode /*= wxEmptyString*/,
                        int                 px_cnt /*= = 16*/) : ScalableButton(parent, mode_id, "", mode, wxDefaultSize, wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER, px_cnt),
    m_mode_id(mode_id)
{
    update_bitmap();
    Init(mode);
}

void ModeButton::Init(const wxString &mode)
{
    m_tt_focused  = Slic3r::GUI::format_wxstr(_L("Switch to the %s mode"), mode);
    m_tt_selected = Slic3r::GUI::format_wxstr(_L("Current mode is %s"),    mode);

    SetBitmapMargins(3, 0);
    SetForegroundColour(Slic3r::GUI::wxGetApp().m_color_top_font_def);
    SetBackgroundColour(Slic3r::GUI::wxGetApp().m_color_tab_bg); 
}

void ModeButton::OnButton()
{
    m_is_selected = true;
    //focus_button(m_is_selected);
    SetForegroundColour(Slic3r::GUI::wxGetApp().m_color_font_focus);
    SetBackgroundColour(wxColour(33, 39, 37));
#if __APPLE__
    wxFont font = Slic3r::GUI::wxGetApp().normal_font();
    font.SetPointSize(15);
    this->SetFont(font);
#endif
}

void ModeButton::SetState(const bool state)
{
    m_is_selected = state;
    focus_button(m_is_selected);
    SetToolTip(state ? m_tt_selected : m_tt_focused);
}

void ModeButton::update_bitmap()
{
    m_bmp = *get_bmp_bundle("mode", m_bmp_width, m_bmp_height, Slic3r::GUI::wxGetApp().get_mode_btn_color(m_mode_id));

    SetBitmap(m_bmp);
    SetBitmapCurrent(m_bmp);
    SetBitmapPressed(m_bmp);
}

void ModeButton::focus_button(const bool focus)
{
    const wxFont& new_font = focus ? 
                             Slic3r::GUI::wxGetApp().bold_font() : 
                             Slic3r::GUI::wxGetApp().normal_font();

//    SetFont(new_font);
//#ifdef _WIN32
//    GetParent()->Refresh(); // force redraw a background of the selected mode button
//#else
//    SetForegroundColour(wxSystemSettings::GetColour(focus ? wxSYS_COLOUR_BTNTEXT : 
//#if defined (__linux__) && defined (__WXGTK3__)
//        wxSYS_COLOUR_GRAYTEXT
//#elif defined (__linux__) && defined (__WXGTK2__)
//        wxSYS_COLOUR_BTNTEXT
//#else 
//        wxSYS_COLOUR_BTNSHADOW
//#endif    
//    ));
//#endif /* no _WIN32 */

    Refresh();
    Update();
}

void ModeButton::sys_color_changed()
{
    Slic3r::GUI::wxGetApp().UpdateDarkUI(this, m_has_border);
    update_bitmap();
}


// ----------------------------------------------------------------------------
// ModeSizer
// ----------------------------------------------------------------------------

ModeSizer::ModeSizer(wxWindow *parent, int hgap/* = 0*/) :
    wxFlexGridSizer(3, 0, hgap),
    m_hgap_unscaled((double)(hgap)/em_unit(parent))
{
    SetFlexibleDirection(wxHORIZONTAL);

    auto modebtnfn = [this](wxCommandEvent &event, int mode_id) {
        if (Slic3r::GUI::wxGetApp().save_mode(mode_id))
            event.Skip();
        else
            SetMode(Slic3r::GUI::wxGetApp().get_mode());
    };
    
    m_mode_btns.reserve(3);
    int mode_id = 0;
    for (const wxString &label : {_L("Simple"), _L("Advanced"), _L("Expert")}) {
        auto mode_btn = new ModeButton(parent, mode_id++, label, mode_icon_px_size());
        if (Slic3r::GUI::wxGetApp().get_mode_defaul() == mode_btn->GetLabel()) {
            mode_btn->OnButton();
            m_focs_label = mode_btn->GetLabel();
#if __APPLE__
            wxFont font = Slic3r::GUI::wxGetApp().normal_font();
            font.SetPointSize(15);
            mode_btn->SetFont(font);
#endif

            // m_is_mode_btns[mode_btn->GetId()] = true;
        }
        mode_btn->Bind(wxEVT_BUTTON, [=](wxCommandEvent &event) {
            if (m_focs_label == mode_btn->GetLabel()) {
                return;
            }
            m_old_id = event.GetId();
            wxFont font = Slic3r::GUI::wxGetApp().normal_font();
            font.SetPointSize(13);
            for (auto iter : m_mode_btns) {
                iter->SetForegroundColour(Slic3r::GUI::wxGetApp().m_color_top_font_def);
                iter->SetBackgroundColour(Slic3r::GUI::wxGetApp().m_color_tab_bg);
#if __APPLE__
                iter->SetFont(font);
#endif
            }
            m_focs_label = mode_btn->GetLabel();
            mode_btn->OnButton();
            event.Skip();
        });
        mode_btn->Bind(wxEVT_ENTER_WINDOW, [=](wxMouseEvent &event) {
            if (m_focs_label != mode_btn->GetLabel()) {
                mode_btn->SetBackgroundColour(Slic3r::GUI::wxGetApp().m_color_top_bg);
            }
            mode_btn->focus_button(true);
            event.Skip();
        });
        mode_btn->Bind(wxEVT_LEAVE_WINDOW, [=](wxMouseEvent &event) {
            if (m_focs_label != mode_btn->GetLabel()) {
                mode_btn->SetBackgroundColour(Slic3r::GUI::wxGetApp().m_color_tab_bg);
            }
            mode_btn->focus_button(mode_btn->m_is_selected);
        });
        m_mode_btns.push_back(mode_btn);
        m_mode_btns.back()->Bind(wxEVT_BUTTON, std::bind(modebtnfn, std::placeholders::_1, int(m_mode_btns.size() - 1)));
        Add(m_mode_btns.back(), 0, wxALIGN_CENTER);
    }
}

void ModeSizer::SetMode(const int mode)
{
    wxFont font = Slic3r::GUI::wxGetApp().normal_font();
    for (size_t m = 0; m < m_mode_btns.size(); m++) {
        if (int(m) == mode) {
            m_mode_btns[m]->SetForegroundColour(Slic3r::GUI::wxGetApp().m_color_font_focus);
            m_mode_btns[m]->SetBackgroundColour(wxColour(33, 39, 37 /*,204*/));
#if __APPLE__
            font.SetPointSize(15);
            m_mode_btns[m]->SetFont(font);
#endif
        } else {
            m_mode_btns[m]->SetForegroundColour(Slic3r::GUI::wxGetApp().m_color_top_font_def);
            m_mode_btns[m]->SetBackgroundColour(Slic3r::GUI::wxGetApp().m_color_tab_bg);
#if __APPLE__
            font.SetPointSize(13);
            m_mode_btns[m]->SetFont(font);
#endif
        }
        m_mode_btns[m]->SetState(int(m) == mode);
    }
}

void ModeSizer::set_items_flag(int flag)
{
    for (wxSizerItem* item : this->GetChildren())
        item->SetFlag(flag);
}

void ModeSizer::set_items_border(int border)
{
    for (wxSizerItem* item : this->GetChildren())
        item->SetBorder(border);
}

void ModeSizer::sys_color_changed()
{
    for (ModeButton* btn : m_mode_btns)
        btn->sys_color_changed();
}

void ModeSizer::update_mode_markers()
{
    for (ModeButton* btn : m_mode_btns)
        btn->update_bitmap();
}

// ----------------------------------------------------------------------------
// MenuWithSeparators
// ----------------------------------------------------------------------------

void MenuWithSeparators::DestroySeparators()
{
    if (m_separator_frst) {
        Destroy(m_separator_frst);
        m_separator_frst = nullptr;
    }

    if (m_separator_scnd) {
        Destroy(m_separator_scnd);
        m_separator_scnd = nullptr;
    }
}

void MenuWithSeparators::SetFirstSeparator()
{
    m_separator_frst = this->AppendSeparator();
}

void MenuWithSeparators::SetSecondSeparator()
{
    m_separator_scnd = this->AppendSeparator();
}

// ----------------------------------------------------------------------------
// PrusaBitmap
// ----------------------------------------------------------------------------
ScalableBitmap::ScalableBitmap( wxWindow *parent, 
                                const std::string& icon_name,
                                const int  width/* = 16*/,
                                const int  height/* = -1*/,
                                const bool grayscale/* = false*/):
    m_parent(parent), m_icon_name(icon_name),
    m_bmp_width(width), m_bmp_height(height)
{
    m_bmp = *get_bmp_bundle(icon_name, width, height);
    m_bitmap = m_bmp.GetBitmapFor(m_parent);
}

ScalableBitmap::ScalableBitmap( wxWindow*           parent,
                                const std::string&  icon_name,
                                const wxSize        icon_size,
                                const bool          grayscale/* = false*/) :
ScalableBitmap(parent, icon_name, icon_size.x, icon_size.y, grayscale)
{
}

void ScalableBitmap::sys_color_changed()
{
    m_bmp = *get_bmp_bundle(m_icon_name, m_bmp_width, m_bmp_height);
}

// ----------------------------------------------------------------------------
// PrusaButton
// ----------------------------------------------------------------------------

ScalableButton::ScalableButton( wxWindow *          parent,
                                wxWindowID          id,
                                const std::string&  icon_name /*= ""*/,
                                const wxString&     label /* = wxEmptyString*/,
                                const wxSize&       size /* = wxDefaultSize*/,
                                const wxPoint&      pos /* = wxDefaultPosition*/,
                                long                style /*= wxBU_EXACTFIT | wxNO_BORDER*/,
                                int                 width/* = 16*/, 
                                int                 height/* = -1*/) :
    m_parent(parent),
    m_current_icon_name(icon_name),
    m_bmp_width(width),
    m_bmp_height(height),
    m_has_border(!(style & wxNO_BORDER))
{
    Create(parent, id, label, pos, size, style);
    if (icon_name != "reduction" && icon_name != "cog" && icon_name != "edit_prest" && icon_name != "close" && label != _("Add Printer")) {
        Slic3r::GUI::wxGetApp().UpdateDarkUI(this);
    }

    if (!icon_name.empty()) {
        if (label == _("Add Printer")) {
            SetBitmap(*get_bmp_bundle(icon_name, m_bmp_width), wxTOP);
        }else if (label == _("No printer selected")) {
            SetBitmap(*get_bmp_bundle(icon_name, 32), wxTOP);
        } else {
            SetBitmap(*get_bmp_bundle(icon_name, m_bmp_width));
        }
        if (!label.empty()) {
            SetBitmapMargins(int(0.5 * em_unit(parent)), 0);
        }
    }
    if (label == _L("Simple") || label == _L("Advanced") || label == _L("Expert"))
        SetForegroundColour(wxColour(235,235,250));
    wxFont font = Slic3r::GUI::wxGetApp().normal_font();
    font.SetPointSize(13);
    this->SetFont(font);

    if (size != wxDefaultSize)
    {
        const int em = em_unit(parent);
        m_width = size.x/em;
        m_height= size.y/em;
    }
}


ScalableButton::ScalableButton( wxWindow *          parent, 
                                wxWindowID          id,
                                const ScalableBitmap&  bitmap,
                                const wxString&     label /*= wxEmptyString*/, 
                                long                style /*= wxBU_EXACTFIT | wxNO_BORDER*/) :
    m_parent(parent),
    m_current_icon_name(bitmap.name()),
    m_bmp_width(bitmap.px_size().x),
    m_bmp_height(bitmap.px_size().y),
    m_has_border(!(style& wxNO_BORDER))
{
    Create(parent, id, label, wxDefaultPosition, wxDefaultSize, style);
    //Slic3r::GUI::wxGetApp().UpdateDarkUI(this);

    SetBitmap(bitmap.bmp());
}

void ScalableButton::SetBitmap_(const ScalableBitmap& bmp)
{
    SetBitmap(bmp.bmp());
    m_current_icon_name = bmp.name();
}

bool ScalableButton::SetBitmap_(const std::string& bmp_name)
{
    m_current_icon_name = bmp_name;
    if (m_current_icon_name.empty())
        return false;

    wxBitmapBundle bmp = *get_bmp_bundle(m_current_icon_name, m_bmp_width, m_bmp_height);
    SetBitmap(bmp);
    SetBitmapCurrent(bmp);
    SetBitmapPressed(bmp);
    SetBitmapFocus(bmp);
    SetBitmapDisabled(bmp);
    return true;
}

void ScalableButton::SetBitmapDisabled_(const ScalableBitmap& bmp)
{
    SetBitmapDisabled(bmp.bmp());
    m_disabled_icon_name = bmp.name();
}

int ScalableButton::GetBitmapHeight()
{
#ifdef __APPLE__
    return GetBitmap().GetScaledHeight();
#else
    return GetBitmap().GetHeight();
#endif
}


void ScalableButton::sys_color_changed()
{
    //Slic3r::GUI::wxGetApp().UpdateDarkUI(this, m_has_border);

    wxBitmapBundle bmp = *get_bmp_bundle(m_current_icon_name, m_bmp_width, m_bmp_height);
    SetBitmap(bmp);
    SetBitmapCurrent(bmp);
    SetBitmapPressed(bmp);
    SetBitmapFocus(bmp);
    if (!m_disabled_icon_name.empty())
        SetBitmapDisabled(*get_bmp_bundle(m_disabled_icon_name, m_bmp_width, m_bmp_height));
    if (!GetLabelText().IsEmpty())
        SetBitmapMargins(int(0.5 * em_unit(m_parent)), 0);
}


BEGIN_EVENT_TABLE(TopButton, wxControl)
EVT_PAINT(TopButton::OnPaint)
// EVT_ENTER_WINDOW(TopButton::OnEnter)
// EVT_LEAVE_WINDOW(TopButton::OnLeave)
// EVT_LEFT_UP(TopButton::OnClick)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(TopButton, wxControl)
TopButton::~TopButton() {}

bool TopButton::Create(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, long style, const wxValidator &validator)
{
    if (!wxControl::Create(parent, id, pos, size, style, validator)) {
        return false;
    }
    return true;
}
wxColour TopButton::m_enter_colour = wxColor(255, 0, 0);
wxColour TopButton::m_leave_colour = wxColor(240, 240, 240);

//#ifdef __WIN32__
//WXLRESULT TopButton::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) 
//{
//    if (nMsg == WM_GETDLGCODE) {
//        return DLGC_WANTMESSAGE;
//    }
//    if (nMsg == WM_KEYDOWN) {
//        wxKeyEvent event(CreateKeyEvent(wxEVT_KEY_DOWN, wParam, lParam));
//        switch (wParam) {
//        case WXK_RETURN: { // WXK_RETURN key is handled by default button
//            GetEventHandler()->ProcessEvent(event);
//            return 0;
//        }
//        }
//    }
//    return wxWindow::MSWWindowProc(nMsg, wParam, lParam);
//}
//#endif
void      TopButton::init()
{
    m_border_colour = "blue";
    m_border_width  = 1;
    m_isClick       = false;
    m_showBorder    = false;
    m_size          = wxDefaultSize;
}

void TopButton::SetLabelBitmap(wxString label, wxBitmap bitmap)
{
    m_label  = label;
    m_bitmap = bitmap;
    if (!m_bitmap.IsNull() && m_label.empty()) {
        SetBitmap(bitmap);
        return;
    } else if (m_bitmap.IsNull() && !m_label.empty()) {
        SetLabel(m_label);
        return;
    }

    m_size.x = m_bitmap.GetSize().x + GetTextExtent(m_label).x + 20;
    m_size.y = m_bitmap.GetSize().y + GetTextExtent(m_label).y;
    SetMinSize(m_size);
}

void TopButton::SetLabel(wxString label)
{
    m_label  = label;
    m_size.x = GetTextExtent(m_label).x + 20;
    m_size.y = GetTextExtent(m_label).y + 7;
    SetMinSize(m_size);
}

void TopButton::SetBitmap(wxBitmap bitmap)
{
    m_bitmap = bitmap;
    m_size   = m_bitmap.GetSize();
    SetMinSize(m_size);
}

void TopButton::OnEnter(wxMouseEvent &event)
{
    if (this->GetLabel() != Slic3r::GUI::wxGetApp().btn_lable) {
        this->SetBackgroundColour(wxColour(33, 39, 37, 204));
        // this->SetBackgroundColour(m_enter_colour);
        this->Refresh();
    }
}

void TopButton::OnEnterMenu(wxMouseEvent &event)
{
    this->SetBackgroundColour(Slic3r::GUI::wxGetApp().m_color_tab_bg);
    this->ShowBorder(true);
    this->SetBorderColor(wxColour(0, 178, 104));
    this->Refresh();
}

void TopButton::OnEnterKey(wxMouseEvent &event)
{
    this->SetForegroundColour(Slic3r::GUI::wxGetApp().m_color_font_focus);
    this->Refresh();
}

void TopButton::OnLeave(wxMouseEvent &event)
{
    if (this->GetLabel() != Slic3r::GUI::wxGetApp().btn_lable) {
        this->SetBackgroundColour(Slic3r::GUI::wxGetApp().m_color_tab_bg);
        this->Refresh();
    }
}

void TopButton::OnLeaveMenu(wxMouseEvent &event)
{
    this->ShowBorder(false);
    this->SetBackgroundColour(Slic3r::GUI::wxGetApp().m_color_top_bg);
    this->Refresh();

    // SetBorderColor(wxColour(255, 0, 0));
}

void TopButton::OnLeaveKey(wxMouseEvent &event)
{
    if (this->GetLabel() != Slic3r::GUI::wxGetApp().key_lable) {
        this->SetForegroundColour(wxColor(0, 0, 0));
    }
    this->Refresh();
}

void TopButton::OnPaint(wxPaintEvent &event)
{
    wxPaintDC dc(this);

    wxPoint pos = GetPosition();
    wxPen   pen(m_border_colour, m_border_width);
    dc.SetPen(pen);

    if (m_showBorder) {
        dc.SetBrush(wxBrush(Slic3r::GUI::wxGetApp().m_color_top_bg)); // 设置填充颜色为绿色
        auto bnm = GetSize();
        dc.DrawRectangle(0, 0, bnm.x, bnm.y);
    }
    if (!m_bitmap.IsNull() && m_label.empty()) {
        dc.DrawBitmap(m_bitmap, wxDefaultPosition);
    } else if (m_bitmap.IsNull() && !m_label.empty()) {
        auto sizes = GetSize();
        dc.DrawLabel(m_label, m_bitmap, wxRect(wxPoint((sizes.x - m_size.x)/2, 0), wxSize(m_size.x - 3, m_size.y)), wxALIGN_CENTER);
    } else {
        auto          m_size = GetSize();
        const double &em = em_unit(m_parent) / 10;          
        dc.DrawLabel(m_label, m_bitmap, wxRect(wxPoint(0, 0), wxSize((m_size.x - 10) * em, m_size.y * em)),
                         wxALIGN_CENTER);
    }
}

// ----------------------------------------------------------------------------
// BlinkingBitmap
// ----------------------------------------------------------------------------

BlinkingBitmap::BlinkingBitmap(wxWindow* parent, const std::string& icon_name) :
    wxStaticBitmap(parent, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize(int(1.6 * Slic3r::GUI::wxGetApp().em_unit()), -1))
{
    bmp = ScalableBitmap(parent, icon_name);
}

void BlinkingBitmap::invalidate()
{
    this->SetBitmap(wxNullBitmap);
}

void BlinkingBitmap::activate()
{
    this->SetBitmap(bmp.bmp());
    show = true;
}

void BlinkingBitmap::blink()
{
    show = !show;
    this->SetBitmap(show ? bmp.bmp() : wxNullBitmap);
}

namespace Slic3r {
namespace GUI {

void Highlighter::set_timer_owner(wxWindow* owner, int timerid/* = wxID_ANY*/)
{
    m_timer.SetOwner(owner, timerid);
    bind_timer(owner);
}

bool Highlighter::init(bool input_failed)
{
    if (input_failed)
        return false;

    m_timer.Start(300, false);
    return true;
}
void Highlighter::invalidate()
{
    if (m_timer.IsRunning())
        m_timer.Stop();
    m_blink_counter = 0;
}

void Highlighter::blink()
{
    if ((++m_blink_counter) == 11)
        invalidate();
}

// HighlighterForWx

void HighlighterForWx::bind_timer(wxWindow* owner)
{
    owner->Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
        blink();
    });
}

// using OG_CustomCtrl where arrow will be rendered and flag indicated "show/hide" state of this arrow
void HighlighterForWx::init(std::pair<OG_CustomCtrl*, bool*> params)
{
    invalidate();
    if (!Highlighter::init(!params.first && !params.second))
        return;

    m_custom_ctrl = params.first;
    m_show_blink_ptr = params.second;

    *m_show_blink_ptr = true;
    m_custom_ctrl->Refresh();
}

// - using a BlinkingBitmap. Change state of this bitmap
void HighlighterForWx::init(BlinkingBitmap* blinking_bmp)
{
    invalidate();
    if (!Highlighter::init(!blinking_bmp))
        return;

    m_blinking_bitmap = blinking_bmp;
    m_blinking_bitmap->activate();
}

void HighlighterForWx::invalidate()
{
    Highlighter::invalidate();

    if (m_custom_ctrl && m_show_blink_ptr) {
        *m_show_blink_ptr = false;
        m_custom_ctrl->Refresh();
        m_show_blink_ptr = nullptr;
        m_custom_ctrl = nullptr;
    }
    else if (m_blinking_bitmap) {
        m_blinking_bitmap->invalidate();
        m_blinking_bitmap = nullptr;
    }
}

void HighlighterForWx::blink()
{
    if (m_custom_ctrl && m_show_blink_ptr) {
        *m_show_blink_ptr = !*m_show_blink_ptr;
        m_custom_ctrl->Refresh();
    }
    else if (m_blinking_bitmap)
        m_blinking_bitmap->blink();
    else
        return;

    Highlighter::blink();
}

}// GUI
}//Slicer




