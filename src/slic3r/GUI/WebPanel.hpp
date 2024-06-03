#pragma once
#include "I18N.hpp"
#include "wxExtensions.hpp"
#include "ConfigIni.hpp"
#include "GUI_Utils.hpp"
#include <wx/panel.h>
#include <wx/webview.h>
#include <wx/stattext.h>
#include <wx/simplebook.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/menu.h>
#include "wx/statline.h"
#include <wx/textdlg.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/wx.h>
#include <wx/socket.h>  
#include <map>
#include <vector>
#include <list>

const wxString urlPrefix = "http://";
namespace Slic3r {
namespace GUI {

class AddDialog : public DPIDialog
{
public:
    AddDialog(const wxString &title);

    wxString GetUsername() const { return m_usernameTextCtrl->GetValue(); }
    wxString GetPassword() const { return m_passwordTextCtrl->GetValue(); }
    wxButton *loginButton;

protected:
    void on_dpi_changed(const wxRect &suggested_rect) override;
    wxTextCtrl *m_usernameTextCtrl;
    wxTextCtrl *m_passwordTextCtrl;
};


//使用 url 创建一个web页面，使用 isVilid 函数判断 web控件是否创建成功
class WebPanel : public wxPanel
{
public:
    WebPanel(wxWindow *parent, wxWindowID winid, const wxString &url);
    ~WebPanel();
    void     loadURL(const wxString &url);
    void     reLoad();
    wxString getURL() { return m_url; }

    //页面是否要用（m_web_view 创建是否成功）
    bool isVilid() { return m_vilid; }
    bool is_connect_false = false;
    void OnWebViewError(wxWebViewEvent &event);
    //void OnWebViewNavigated(wxWebViewEvent &event); 

protected:
    void OnLoadError(const wxString &url) { wxLogError("无法打开URL: %s", url); }

protected:
    wxString   m_url;
    wxWebView *m_web_view;
    bool       m_vilid;
};

/*
 * 管理一系列的web页面
 */
class WebPage : public wxPanel
{
public:
    WebPage(wxWindow *parent, wxWindowID winid);
    ~WebPage();

    wxString GetWorkPath();

    //添加 webview 控件
    bool installWebViewWidget();

    //判断IP格式是否正确
    bool is_valid_ip(string s);

    void DeletePageByLabel(const wxString &targetLabel);
    //添加web页
    bool addWebPanel(const wxString &url, wxString username);

    void CreateRefenshPage();

    //响应添加按钮
    void onAddButton(wxMouseEvent &event);
    //响应按钮鼠标左键单击
    void onItemButton(wxMouseEvent &event);
    //响应按钮鼠标右键
    void onItemRightButton(wxMouseEvent &event);
    //右键菜单
    void onMenuItem(wxCommandEvent &event);
    std::map<wxString, wxString> m_name_and_ip;

protected:
    void update();

protected:
    wxSimplebook *      m_webManager;
    int           m_focus_buttonid;
    ConfigIni *         m_config;
    //<button id, button window>
    std::map<int, TopButton *> m_button_map;
    //<button id, webpanel window>
    std::map<int, wxWindow *> m_page_map;

    wxPanel * m_item_panel; //左侧列表区域
    ScalableButton *m_add_button;
    wxPanel *       init_panel;
    int       m_select_button; //鼠标单击的按钮 id
    wxColor         m_select_color;
    wxMenu *m_menu; //弹出菜单

    bool m_webview_installed;

private:
    static int itemID;
};

}} // namespace Slic3r::GUI