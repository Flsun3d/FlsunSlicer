///|/ Copyright (c) Prusa Research 2018 - 2023 Oleksandra Iushchenko @YuSanka, Vojtěch Bubník @bubnikv, David Kocík @kocikdav, Enrico Turri
///@enricoturri1966, Lukáš Matěna @lukasmatena, Tomáš Mészáros @tamasmeszaros, Vojtěch Král @vojtechkral
///|/ Copyright (c) 2019 John Drake @foxox
///|/
///|/ ported from lib/Slic3r/GUI/MainFrame.pm:
///|/ Copyright (c) Prusa Research 2016 - 2019 Vojtěch Bubník @bubnikv, Vojtěch Král @vojtechkral, Oleksandra Iushchenko @YuSanka, Tomáš
///Mészáros @tamasmeszaros, Enrico Turri @enricoturri1966
///|/ Copyright (c) Slic3r 2014 - 2016 Alessandro Ranellucci @alranel
///|/ Copyright (c) 2014 Mark Hindess
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#ifndef slic3r_MainFrame_hpp_
#define slic3r_MainFrame_hpp_

#include "libslic3r/PrintConfig.hpp"

#include <wx/taskbar.h>
#include <wx/frame.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/filehistory.h>
#ifdef __APPLE__
#include <wx/taskbar.h>
#endif // __APPLE__
#include <vector>
#include <string>
#include <map>
#include "WebPanel.hpp"
#include "GUI_Utils.hpp"
#include "Event.hpp"
#include "UnsavedChangesDialog.hpp"
#include "Tab.hpp"

class wxBookCtrlBase;
class wxProgressDialog;

namespace Slic3r {

class ProgressStatusBar;

namespace GUI {

class Tab;
class PrintHostQueueDialog;
class Plater;
class MainFrame;
class PreferencesDialog;
class GalleryDialog;

enum QuickSlice { qsUndef = 0, qsReslice = 1, qsSaveAs = 2, qsExportSVG = 4, qsExportPNG = 8 };

struct PresetTab
{
    std::string       name;
    Tab *             panel;
    PrinterTechnology technology;
};

// ----------------------------------------------------------------------------
// SettingsDialog
// ----------------------------------------------------------------------------

class MyPrintDialog : public wxDialog
{
public:
    MyPrintDialog(const wxString &title, wxSize defaulsize)
        : wxDialog(NULL, wxID_ANY, title, wxDefaultPosition, defaulsize, wxDEFAULT_DIALOG_STYLE)
    {
#if _WIN32

        Bind(wxEVT_CHAR_HOOK, &MyPrintDialog::OnCharHook, this);
#endif
        CenterOnParent();
        wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        if (_L("Printer Settings") == title) {
            m_dialog = new TabPrinter(this);
            m_dialog->create_preset_tab();
            // sizer->Add(new wxStaticText(this, wxID_ANY, _("这是一个模态弹窗")));
            sizer->Add(m_dialog, 1, wxEXPAND);
        } else if (_L("Filament Settings") == title) {
            m_dialog = new TabFilament(this);
            m_dialog->create_preset_tab();
            sizer->Add(m_dialog, 1, wxEXPAND);
        } else if (_L("Print Settings") == title) {
            m_dialog = new TabPrint(this);
            m_dialog->create_preset_tab();
            sizer->Add(m_dialog, 1, wxEXPAND);
        }
        m_dialog->OnActivate();
        //m_dialog->SetBackgroundColour(wxColour(255, 255, 255));
        SetSizer(sizer); // 设置布局管理器
    }
#if _WIN32
    void OnCharHook(wxKeyEvent &event);
#endif
    Tab *m_dialog;
};

class SettingsDialog : public DPIFrame // DPIDialog
{
    wxBookCtrlBase *m_tabpanel{nullptr};
    MainFrame *     m_main_frame{nullptr};
    wxMenuBar *     m_menubar{nullptr};

public:
    SettingsDialog(MainFrame *mainframe);
    ~SettingsDialog() = default;
    void       set_tabpanel(wxBookCtrlBase *tabpanel) { m_tabpanel = tabpanel; }
    wxMenuBar *menubar() { return m_menubar; }

protected:
    void on_dpi_changed(const wxRect &suggested_rect) override;
};

class MainFrame : public DPIFrame
{
    bool m_loaded{false};

    wxString   m_qs_last_input_file  = wxEmptyString;
    wxString   m_qs_last_output_file = wxEmptyString;
    wxString   m_last_config         = wxEmptyString;
    wxMenuBar *m_menubar{nullptr};

public:
    MyPrintDialog *              m_print_dialog;
    MyPrintDialog *              m_printer_dialog;
    MyPrintDialog *              m_filament_dialog;
    std::vector<MyPrintDialog *> m_dialog_vec;
#if 0
    wxMenuItem* m_menu_item_repeat { nullptr }; // doesn't used now
#endif
    bool        m_is_enable_send;
    bool        m_isDragging; // 是否正在拖动窗口
    wxPoint     m_startPos;   // 鼠标开始拖动时的位置
    wxTimer *   timer;
    wxMenuItem *m_menu_item_reslice_now{nullptr};
    wxSizer *   m_main_sizer{nullptr};

    wxMenu *fileMenuGcodeviewer;
    wxMenu *helpMenuGcodeviewer;

    wxMenu *                  fileMenu;
    wxMenu *                  editMenu;
    wxMenu *                  helpMenu;
    wxMenu *                  configurationMenu;
    wxPanel *                 m_title_bar;
    std::vector<wxMenuItem *> m_menuitem;
    wxMenu *                  recent_projects_menu;
    wxMenu *                  export_menu;
    bool                      m_is_close = true;
    size_t                    m_last_selected_tab;
    wxPoint         m_offset; //偏移坐标
    std::string     get_base_name(const wxString &full_name, const char *extension = nullptr) const;
    std::string     get_dir_name(const wxString &full_name) const;
    ScalableButton *m_restoreBtn;
    // bool ProcessEvent(wxEvent &event) override;
    void OnPaint(wxPaintEvent &event);
    void OnMotion(wxMouseEvent &event);
    void OnLeftDown(wxMouseEvent &event);
    void OnLeftUp(wxMouseEvent &event);
    void OnMouseLost(wxMouseCaptureLostEvent &event);
    void OnDoubleClick(wxMouseEvent &event); //双击事件
  
    //最小化事件
    void OnMinimizeButtonClick(wxMouseEvent &event);
    void OnRestoreButtonClick(wxMouseEvent &event);
    void OnCloseButtonClick(wxMouseEvent &event);
    //点击任务栏事件
    void onItemLeftButton(wxMouseEvent &event);
    void windowSwitching();
    void init_title_bar();

    void init_title_gcodeviewer_bar();
    void OnCharHook(wxKeyEvent &event);
    void OnTimer(wxTimerEvent &event);
    void on_presets_changed(SimpleEvent &);
    void on_value_changed(wxCommandEvent &);
    bool can_start_new_project() const;
    bool can_export_model() const;
    bool can_export_toolpaths() const;
    bool can_export_supports() const;
    bool can_export_gcode() const;
    bool can_send_gcode() const;
    bool can_export_gcode_sd() const;
    bool can_eject() const;
    bool can_slice() const;
    bool can_change_view() const;
    bool can_select() const;
    bool can_deselect() const;
    bool can_delete() const;
    bool can_delete_all() const;
    bool can_reslice() const;
    void bind_diff_dialog();

    // MenuBar items changeable in respect to printer technology
    enum MenuItems {   //   FFF                  SLA
        miExport = 0,  // Export G-code        Export
        miSend,        // Send G-code          Send to print
        miMaterialTab, // Filament Settings    Material Settings
        miPrinterTab,  // Different bitmap for Printer Settings
    };

    // vector of a MenuBar items changeable in respect to printer technology
    std::vector<wxMenuItem *> m_changeable_menu_items;

    wxFileHistory m_recent_projects;

    enum class ESettingsLayout { Unknown, Old, New, Dlg, GCodeViewer };

    ESettingsLayout m_layout{ESettingsLayout::Unknown};

protected:
#ifdef __WIN32__
    WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) override;
#endif

    virtual void on_dpi_changed(const wxRect &suggested_rect) override;
    virtual void on_sys_color_changed() override;

public:
    MainFrame(const int font_point_size);
    ~MainFrame() = default;

    void update_layout();
    void update_mode_markers();

    // Called when closing the application and when switching the application language.
    void shutdown();

    Plater *       plater() { return m_plater; }
    GalleryDialog *gallery_dialog();

    void update_title();

    void init_tabpanel();
    void create_preset_tabs();
    void add_created_tab(Tab *panel, const std::string &bmp_name = "");
    bool is_active_and_shown_tab(Tab *tab);
    // Register Win32 RawInput callbacks (3DConnexion) and removable media insert / remove callbacks.
    // Called from wxEVT_ACTIVATE, as wxEVT_CREATE was not reliable (bug in wxWidgets?).
    void register_win32_callbacks();
    void init_menubar_as_editor();
    void init_menubar_as_gcodeviewer();
    void update_menubar();
    // Open item in menu by menu and item name (in actual language)
    void open_menubar_item(const wxString &menu_name, const wxString &item_name);
#ifdef _WIN32
    void show_tabs_menu(bool show);
#endif
    void update_ui_from_settings();
    bool is_loaded() const { return m_loaded; }
    bool is_last_input_file() const { return !m_qs_last_input_file.IsEmpty(); }
    bool is_dlg_layout() const { return m_layout == ESettingsLayout::Dlg; }

    //    void        quick_slice(const int qs = qsUndef);
    void reslice_now();
    void repair_stl();
    void export_config();
    // Query user for the config file and open it.
    void load_config_file();
    // Open a config file. Return true if loaded.
    bool load_config_file(const std::string &path);
    void export_configbundle(bool export_physical_printers = false);
    void load_configbundle(wxString file = wxEmptyString);
    void load_config(const DynamicPrintConfig &config);
    // Select tab in m_tabpanel
    // When tab == -1, will be selected last selected tab
    void select_tab(Tab *tab);
    void select_tab(size_t tab = size_t(-1));
    void select_view(const std::string &direction);
    // Propagate changed configuration from the Tab to the Plater and save changes to the AppConfig
    void on_config_changed(DynamicPrintConfig *cfg) const;

    bool can_save() const;
    bool can_save_as() const;
    void save_project();
    bool save_project_as(const wxString &filename = wxString());

    void add_to_recent_projects(const wxString &filename);
    void technology_changed();

    PrintHostQueueDialog *printhost_queue_dlg() { return m_printhost_queue_dlg; }
    WebPage *             m_monitoring_page;
    Plater *              m_plater{nullptr};
    Plater *              m_preview_page{nullptr};
    wxBookCtrlBase *      m_tabpanel{nullptr};
    SettingsDialog        m_settings_dialog;
    DiffPresetDialog      diff_dialog;
    wxWindow *            m_plater_page{nullptr};
    //    wxProgressDialog*     m_progress_dialog { nullptr };
    PreferencesDialog *   preferences_dialog{nullptr};
    PrintHostQueueDialog *m_printhost_queue_dlg;
    //    std::shared_ptr<ProgressStatusBar>  m_statusbar;
    GalleryDialog *m_gallery_dialog{nullptr};

#ifdef __APPLE__
    std::unique_ptr<wxTaskBarIcon> m_taskbar_icon;
#endif // __APPLE__

#ifdef _WIN32
    void *               m_hDeviceNotify{nullptr};
    uint32_t             m_ulSHChangeNotifyRegister{0};
    static constexpr int WM_USER_MEDIACHANGED{
        0x7FFF}; // WM_USER from 0x0400 to 0x7FFF, picking the last one to not interfere with wxWidgets allocation

private:
    wxDECLARE_EVENT_TABLE();
#endif // _WIN32
};

} // namespace GUI
} // namespace Slic3r

#endif // slic3r_MainFrame_hpp_
