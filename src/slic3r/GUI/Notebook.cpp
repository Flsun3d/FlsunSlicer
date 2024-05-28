///|/ Copyright (c) Prusa Research 2021 - 2022 Oleksandra Iushchenko @YuSanka, Lukáš Hejl @hejllukas
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Notebook.hpp"
#include "MainFrame.hpp"
#include "Plater.hpp"
//#ifdef _WIN32

#include "GUI_App.hpp"

#include <wx/button.h>
#include <wx/sizer.h>
#include "I18N.hpp"
wxDEFINE_EVENT(wxCUSTOMEVT_NOTEBOOK_SEL_CHANGED, wxCommandEvent);

ButtonsListCtrl::ButtonsListCtrl(wxWindow *parent, bool add_mode_buttons /* = false*/)
    : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTAB_TRAVERSAL)
{
    //#ifdef __WINDOWS__
    SetDoubleBuffered(true);
    //#endif //__WINDOWS__
    m_parent      = parent;
    int em        = em_unit(this); // Slic3r::GUI::wxGetApp().em_unit();
    m_btn_margin  = 0;
    m_line_margin = std::lround(0.1 * em);

    m_sizer = new wxBoxSizer(wxHORIZONTAL);
    this->SetSizer(m_sizer);

    m_buttons_sizer = new wxFlexGridSizer(1, m_btn_margin, m_btn_margin);
    m_sizer->Add(m_buttons_sizer, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxBOTTOM, m_btn_margin);

    if (add_mode_buttons) {
        m_mode_sizer = new ModeSizer(this, m_btn_margin);
        m_sizer->AddStretchSpacer(20);
        m_sizer->Add(m_mode_sizer, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxBOTTOM, m_btn_margin);
    }

    this->Bind(wxEVT_PAINT, &ButtonsListCtrl::OnPaint, this);
}

void ButtonsListCtrl::OnPaint(wxPaintEvent &)
{
    // Slic3r::GUI::wxGetApp().UpdateDarkUI(this);
    const wxSize sz    = GetSize();
    wxPoint      possz = GetPosition();
    wxPaintDC    dc(this);

    if (m_selection < 0 || m_selection >= (int) m_pageButtons.size())
        return;

    const wxColour &selected_btn_bg  = Slic3r::GUI::wxGetApp().m_color_top_bg;
    const wxColour &default_btn_bg   = Slic3r::GUI::wxGetApp().m_color_tab_bg;
    const wxColour &btn_marker_color = Slic3r::GUI::wxGetApp().m_color_font_focus;

    /* if (m_mode_sizer) {
         if (deaful_mode) {
             const std::vector<ModeButton *> &mode_btns = m_mode_sizer->get_btns();
             for (int idx = 0; idx < int(mode_btns.size()); idx++) {
                 ModeButton *btn = mode_btns[idx];
                 auto        nm  = Slic3r::GUI::wxGetApp().app_config->get("view_mode");
                 int         count;
                 if (nm == "simple") {
                     count = 0;
                 } else if (nm == "advanced") {
                     count = 1;
                 } else {
                     count = 2;
                 }

                 auto        bnm = btn->GetLabel();
                 if (idx == count)
                 {
                     btn->SetForegroundColour(Slic3r::GUI::wxGetApp().m_color_font_focus);
                     btn->SetBackgroundColour(Slic3r::GUI::wxGetApp().m_color_top_bg);
                 }

             }
             deaful_mode = false;
         }
     }*/
    if (Slic3r::GUI::wxGetApp().mainframe == m_parent->GetParent()) {
        dc.SetPen(Slic3r::GUI::wxGetApp().m_color_tab_bg);
        dc.SetBrush(Slic3r::GUI::wxGetApp().m_color_tab_bg);
        dc.DrawRectangle(0, 0, sz.x, sz.y);
    } else {
        dc.SetPen(Slic3r::GUI::wxGetApp().get_color_hovered_btn_label());
        dc.SetBrush(Slic3r::GUI::wxGetApp().get_color_hovered_btn_label());
        // dc.DrawRectangle(0, 0, sz.x, m_line_margin);
    }
}

void ButtonsListCtrl::UpdateMode() { m_mode_sizer->SetMode(Slic3r::GUI::wxGetApp().get_mode()); }

void ButtonsListCtrl::Rescale()
{
    int em        = em_unit(this);
    m_btn_margin  = std::lround(0.3 * em);
    m_line_margin = std::lround(0.1 * em);
    m_buttons_sizer->SetVGap(m_btn_margin);
    m_buttons_sizer->SetHGap(m_btn_margin);

    m_sizer->Layout();
}

void ButtonsListCtrl::OnColorsChanged()
{
    /* for (TopButton *btn : m_pageButtons)
         btn->sys_color_changed();*/

    m_mode_sizer->sys_color_changed();

    m_sizer->Layout();
}

void ButtonsListCtrl::UpdateModeMarkers() { m_mode_sizer->update_mode_markers(); }

void ButtonsListCtrl::SetSelection(int sel)
{
    if (m_selection == sel)
        return;
    m_selection = sel;
    Refresh();
}

bool ButtonsListCtrl::InsertPage(size_t n, const wxString &text, bool bSelect /* = false*/, const std::string &bmp_name /* = ""*/)
{
    TopButton *btn = new TopButton(this, wxID_ANY, text, bmp_name, wxDefaultPosition, bmp_name.empty() ? wxDefaultSize : wxSize(200, 30),
                                   wxBU_EXACTFIT | wxNO_BORDER | (bmp_name.empty() ? 0 : wxBU_LEFT));
    if (bmp_name.empty()) {
        if (_L("Commands") == text) {
            btn->SetForegroundColour(Slic3r::GUI::wxGetApp().m_color_font_focus);
            Slic3r::GUI::wxGetApp().btn_lable = btn->GetLabel();
        }
    } else {
        wxScreenDC dc;
        wxSize     ppi         = dc.GetPPI();
        int        standardPPI = 96; // 标准PPI（通常为96）
#ifdef _WIN32
        double scaleFactorY = static_cast < double>(ppi.y) / standardPPI;
#else
       double scaleFactorY = 1;
#endif

        btn->SetSize(wxSize(200 * scaleFactorY, 30 * scaleFactorY));
        btn->SetForegroundColour(Slic3r::GUI::wxGetApp().m_color_top_font_def);
        btn->SetBackgroundColour(Slic3r::GUI::wxGetApp().m_color_tab_bg);
        if (_L("Prepare") == text) {
            btn->SetForegroundColour(Slic3r::GUI::wxGetApp().m_color_font_focus);
            btn->SetBackgroundColour(Slic3r::GUI::wxGetApp().m_color_top_bg);
            Slic3r::GUI::wxGetApp().btn_lable = btn->GetLabel();
        }
    }
    btn->Bind(wxEVT_LEFT_DOWN, [this, btn](wxMouseEvent &event) {
        if (auto it = std::find(m_pageButtons.begin(), m_pageButtons.end(), btn); it != m_pageButtons.end()) {
            m_selection        = it - m_pageButtons.begin();
            wxCommandEvent evt = wxCommandEvent(wxCUSTOMEVT_NOTEBOOK_SEL_CHANGED);
            if (!btn->m_bitmap.IsOk()) {
                evt.SetId(m_selection);
                for (auto iter : m_pageButtons) {
                    iter->SetForegroundColour(wxColor(0, 0, 0));
                    // iter->SetBackgroundColour(Slic3r::GUI::wxGetApp().m_color_tab_bg);
                }
                wxPostEvent(this->GetParent(), evt);
                btn->SetForegroundColour(Slic3r::GUI::wxGetApp().m_color_font_focus);
                Slic3r::GUI::wxGetApp().key_lable = btn->GetLabel();
                Refresh();
                return;
            }
            if (m_selection == 1) {
                //evt.SetId(0);
                //Slic3r::GUI::wxGetApp().mainframe->m_plater->select_view_3D("Preview");
                Slic3r::GUI::wxGetApp().mainframe->reslice_now();
            } else {
                evt.SetId(m_selection);
                Slic3r::GUI::wxGetApp().mainframe->m_plater->select_view_3D("3D");
            }
            for (auto iter : m_pageButtons) {
                iter->SetForegroundColour(Slic3r::GUI::wxGetApp().m_color_top_font_def);
                iter->SetBackgroundColour(Slic3r::GUI::wxGetApp().m_color_tab_bg);
            }
            wxPostEvent(this->GetParent(), evt);
            btn->SetForegroundColour(Slic3r::GUI::wxGetApp().m_color_font_focus);
            btn->SetBackgroundColour(Slic3r::GUI::wxGetApp().m_color_top_bg);

            Slic3r::GUI::wxGetApp().btn_lable = btn->GetLabel();
            Refresh();
        }
    });
    m_pageButtons.insert(m_pageButtons.begin() + n, btn);

    m_buttons_sizer->Insert(n, new wxSizerItem(btn));
    m_buttons_sizer->SetCols(m_buttons_sizer->GetCols() + 1);
    m_sizer->Layout();
    return true;
}

void ButtonsListCtrl::RemovePage(size_t n)
{
    TopButton *btn = m_pageButtons[n];
    m_pageButtons.erase(m_pageButtons.begin() + n);
    m_buttons_sizer->Remove(n);
    btn->Reparent(nullptr);
    btn->Destroy();
    m_sizer->Layout();
}

bool ButtonsListCtrl::SetPageImage(size_t n, const std::string &bmp_name) const
{
    if (n >= m_pageButtons.size())
        return false;
    return true;
}

void ButtonsListCtrl::SetPageText(size_t n, const wxString &strText)
{
    TopButton *btn = m_pageButtons[n];
    btn->SetLabel(strText);
}

wxString ButtonsListCtrl::GetPageText(size_t n) const
{
    TopButton *btn = m_pageButtons[n];

    return btn->GetLabel();
}

//#endif // _WIN32
