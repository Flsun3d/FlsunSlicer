#include "WebPanel.hpp"
#include "GUI_App.hpp"
#include "libslic3r/Preset.hpp"
#include "libslic3r/Utils.hpp"
#include <wx/socket.h>
#include <wx/url.h>
#include <curl/curl.h> 
namespace Slic3r {
namespace GUI {

#define WEBITEMBUTTONSTART 1100
WebPanel::WebPanel(wxWindow *parent, wxWindowID winid, const wxString &url) : wxPanel(parent, winid), m_url(url), m_vilid(false)
{
    auto sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(sizer);
    m_web_view = wxWebView::New(this, winid, url);
  
    if (m_web_view) {
        m_web_view->LoadURL(url);
        sizer->Add(m_web_view, 1, wxEXPAND);
        m_vilid = true;
    } else {
        m_vilid = false;
    }
}

WebPanel::~WebPanel() 
{
    //m_web_view->ClearHistory();
}

void WebPanel::loadURL(const wxString &url) { m_web_view->LoadURL(url); }

void WebPanel::reLoad() { m_web_view->Reload(); }

//void WebPanel::OnWebViewNavigating(wxWebViewEvent &event) 
//{
//    auto vbn = event.GetURL();
//    if (event.GetURL().StartsWith("https://flsun-slicer.oss-cn-shanghai.aliyuncs.com/version")) 
//    {
//        // 在这里执行下载操作，比如使用
//       //
//       //  wxDownloadURL wxDownloadURL(event.GetURL());
//        // 取消默认行为，阻止浏览器打开下载链接
//        event.Veto();
//    } else {
//        event.Skip(); // 其他情况下继续默认行为
//    }
//}
//
//void WebPanel::OnWebViewNavigated(wxWebViewEvent &event) 
//{ 
//    wxString currentURL = m_web_view->GetCurrentURL();
//}

int WebPage::itemID = WEBITEMBUTTONSTART;

WebPage::WebPage(wxWindow *parent, wxWindowID winid) : wxPanel(parent, winid), m_select_button(-1), m_webview_installed(false)
{
    m_select_color = wxColor(179, 246, 203);
    //按钮弹出菜单
    m_menu = new wxMenu;
    // m_menu->Append(++itemID, _("Eidt"));
    m_menu->Append(++itemID, _("Delete"));
    m_menu->Bind(wxEVT_MENU, &WebPage::onMenuItem, this);

    auto mainSizer = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(mainSizer);
    //左侧按钮列表区域
    m_item_panel = new wxPanel(this, wxID_ANY);
    m_item_panel->SetMinSize(wxSize(220, 500));
    m_item_panel->SetBackgroundColour(wxColor(255, 255, 255));
    mainSizer->Add(m_item_panel, 0, wxEXPAND);
    auto item_sizer = new wxBoxSizer(wxVERTICAL);
    m_item_panel->SetSizer(item_sizer);

    m_add_button = new ScalableButton(m_item_panel, ++itemID, "add_printer", _("Add Printer"), wxDefaultSize, wxDefaultPosition,
                                      wxBU_EXACTFIT | wxNO_BORDER);

    //m_add_button->SetMinSize(wxSize(30, 30));
    item_sizer->Add(m_add_button, 0, wxEXPAND);
    m_add_button->Bind(wxEVT_LEFT_DOWN, &WebPage::onAddButton, this);
    m_add_button->SetBackgroundColour(wxColor(255,255,255));
    m_add_button->Bind(wxEVT_ENTER_WINDOW, [=](wxMouseEvent &event) 
        {
        m_add_button->SetForegroundColour(Slic3r::GUI::wxGetApp().m_color_font_focus);
        });
    m_add_button->Bind(wxEVT_LEAVE_WINDOW, [=](wxMouseEvent &event) { m_add_button->SetForegroundColour(wxColor(10, 10, 10)); });
    //右侧web页面显示区
    m_webManager = new wxSimplebook(this, wxID_ANY);
    m_webManager->SetBackgroundColour(wxColor(240, 240, 240));
    CreateRefenshPage();
    mainSizer->Add(m_webManager, 1, wxEXPAND);
    
    const std::string path = wxStandardPaths::Get().GetUserDataDir().ToUTF8().data();
    m_config = new ConfigIni(path + "/ipconfig.ini");
    m_config->GetNodeValue("printerIP", m_name_and_ip);
    if (!m_name_and_ip.empty()) 
    {
        for (auto iter : m_name_and_ip) {
            {
                addWebPanel(iter.second, iter.first);
            }
        }
        for (auto iter : m_button_map) 
        {
            iter.second->SetForegroundColour(wxColor(153, 153, 153));
            iter.second->SetBackgroundColour(wxColor(242, 242, 240));
        }
        if (!m_button_map.empty()) 
        {
            m_button_map.begin()->second->SetForegroundColour(wxColor(20, 20, 20));
            m_button_map.begin()->second->SetBackgroundColour(m_select_color);
            m_webManager->SetSelection(0);
            m_focus_buttonid = m_button_map.begin()->first;
        }
    } else {
        m_webManager->AddPage(init_panel, "refresh", true);
        int pageCount = m_webManager->GetPageCount();
    }
    Refresh();
    
}

WebPage::~WebPage()
{
    if (m_menu) {
        delete m_menu;
        m_menu = nullptr;
    }
}

wxString WebPage::GetWorkPath()
{
    auto       appFile = wxStandardPaths::Get().GetExecutablePath();
    wxFileName appName(appFile);
    wxString   appPath = appName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);

    return appPath;
}

bool WebPage::installWebViewWidget()
{
    wxString msg(_("install webview"));
    const wxString installExe = wxString::FromUTF8(Slic3r::resources_dir() + "\\MicrosoftEdgeWebview2Setup.exe");
    int exitCode = wxExecute(installExe, wxEXEC_SYNC);
    if (exitCode == -1) {
        return false;
    }

    return m_webview_installed = true;
}

bool WebPage::is_valid_ip(string s)
{
    std::regex pattern("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    return std::regex_match(s, pattern);
}

void WebPage::DeletePageByLabel(const wxString &targetLabel)
{
    int pageCount = m_webManager->GetPageCount();
    for (int i = 0; i < pageCount; ++i) {
        wxString pageLabel = m_webManager->GetPageText(i);
        if (pageLabel == targetLabel) {
            // 找到匹配的页面，删除它
            m_webManager->DeletePage(i);
            break;
        }
    }
}

bool WebPage::addWebPanel(const wxString &url, wxString username)
{
    auto webpage = new WebPanel(m_webManager, wxID_ANY, urlPrefix + url );
    if (!webpage->isVilid()) {
        delete webpage;
        webpage = nullptr;
        if (!m_webview_installed) {
            if (!installWebViewWidget()) {
                // install failed  show msg todo...
                return false;
            } else {
                webpage = new WebPanel(m_webManager, wxID_ANY, urlPrefix + url);
            }
        } else {
            //已安装webview，仍然无法加载web页面，弹窗提示未知错误 todo...
            return false;
        }
    }
    webpage->SetWindowStyleFlag(webpage->GetWindowStyleFlag() & ~wxBORDER_SIMPLE);
    //std::string urlll = "http://" + url.ToStdString();
    //CURL *curl = curl_easy_init();
    //if (curl) {
    //    curl_easy_setopt(curl, CURLOPT_URL, urlll.c_str());
    //    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // 只发送HEAD请求
    //    CURLcode res = curl_easy_perform(curl);
    //    if (res == CURLE_OK) {
    //        long response_code;
    //        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    //        if (response_code == 200) {
    //            wxLogMessage(wxT("URL存在"));  
    //        } else if (response_code == 404) {
    //            wxLogMessage(wxT("URL不存在"));  
    //        } else {
    //            wxLogMessage(wxT("检查URL是否存在时出错"));  
    //        }
    //    } else {
    //        wxLogMessage(curl_easy_strerror(res));  
    //    }
    //    curl_easy_cleanup(curl);
    //} else {
    //    std::cout << "Failed to initialize libcurl." << std::endl;
    //    return false;
    //}
 
    m_webManager->AddPage(webpage, url, true);
    int  id     = ++itemID;
    std::string lable_name = wxGetApp().symbol_turn_dot(username.ToStdString());
    TopButton * button     = new TopButton(m_item_panel, id, lable_name);
    auto num = m_item_panel->GetSizer()->GetItemCount();
    m_item_panel->GetSizer()->Insert(num - 1, button, 0, wxEXPAND);
    m_item_panel->Layout();
    button->SetBackgroundColour(wxColor(242, 242, 240));
    button->SetForegroundColour(wxColor(153, 153, 153));
    button->SetToolTip("IP:"+ url);
    wxFont font = Slic3r::GUI::wxGetApp().normal_font();
    font.SetPointSize(13);
    button->SetFont(font);
    //按钮事件处理，鼠标左键页面跳转，右键弹出菜单
    button->Bind(
        wxEVT_LEFT_DOWN, [=](wxMouseEvent &event){
        //页面跳转
        auto id   = event.GetId();
        m_focus_buttonid = event.GetId();
        auto iter = m_page_map.find(id);
        if (iter != m_page_map.end()) {
            auto webPanel = iter->second;
            int  index    = m_webManager->FindPage(webPanel);
            m_webManager->SetSelection(index);
        } else {
            wxMessageBox(_("find web page failed"), _("warring"));
        }
        for (auto iter : m_button_map) 
        {
            iter.second->SetForegroundColour(wxColor(153, 153, 153));
            iter.second->SetBackgroundColour(wxColor(242, 242, 240));
        }
        button->SetForegroundColour(wxColor(20, 20, 20));
        button->SetBackgroundColour(m_select_color);
        Refresh();
        });

    button->Bind(wxEVT_RIGHT_DOWN, &WebPage::onItemRightButton, this);
    for (auto iter : m_button_map)
    {
        iter.second->SetForegroundColour(wxColor(153, 153, 153));
        iter.second->SetBackgroundColour(wxColor(242, 242, 240));
    }
    button->SetForegroundColour(wxColor(20, 20, 20));
    button->SetBackgroundColour(m_select_color);
    Refresh();

    username                = wxGetApp().dot_turn_symbol(username.ToStdString());
    m_name_and_ip[username] = url;
    m_button_map[id] = button;
    m_page_map[id]   = webpage;
    
    return true;
}

void WebPage::CreateRefenshPage() 
{
    init_panel = new wxPanel(m_webManager);
    init_panel->SetBackgroundColour(wxColor(240, 240, 240));

    ScalableButton *refresh_bmp = new ScalableButton(init_panel, ++itemID, "refresh_btn", _("No printer selected"), wxDefaultSize,
                                                     wxDefaultPosition, wxBU_EXACTFIT | wxNO_BORDER);
    refresh_bmp->Enable(false);
    refresh_bmp->SetBackgroundColour(wxColor(240, 240, 240));
    wxBoxSizer *refre_sizer = new wxBoxSizer(wxHORIZONTAL);
    init_panel->SetSizer(refre_sizer);
    refre_sizer->Add(refresh_bmp, 1, wxALIGN_CENTER_VERTICAL);
}


void WebPage::onAddButton(wxMouseEvent &event)
{
    //弹窗编辑 URL
    //设置输入格式 （只能输入数字和点，IP格式 xxx.xxx.xxx.xxx）todo...
    AddDialog *loginDialog = new AddDialog(_L("Add printer"));
    auto         size = loginDialog->GetSize();
    wxString     username;
    wxString     res;
    if (loginDialog->ShowModal() == wxID_OK) {
        username = loginDialog->GetUsername();
        res      = loginDialog->GetPassword();
    } else {
        return;
    }
    if (username.find(' ') != wxString::npos) 
    {
        wxMessageDialog *dial = new wxMessageDialog(NULL, _("The input name cannot contain spaces"), "Error", wxOK | wxICON_ERROR);
        dial->ShowModal();
        return;
    }
    //判断IP中是否含有http
    if (res.find(urlPrefix) != wxString::npos) {
        res.erase(0, urlPrefix.size());
        auto dight = res.find("/");
        if (dight != wxString::npos) {
            res.erase(dight, 1);
        }
    }
    //判断IP格式是否正确
    if (!is_valid_ip(res.ToStdString())) {
        wxMessageDialog *dial = new wxMessageDialog(NULL, _("The format of the input IP is wrong"), "Error", wxOK | wxICON_ERROR);
        dial->ShowModal();
        return;
    }

    std::string bnm = username.ToStdString();
    bnm             = wxGetApp().dot_turn_symbol(bnm);
    
    //判断重复添加
    for (auto iter : m_name_and_ip) {

        if (res == iter.second || bnm == iter.first) {
            wxMessageDialog *dial = new wxMessageDialog(NULL, _("Entered a duplicate printer IP address or name"), "Error",
                                                        wxOK | wxICON_ERROR);
            dial->ShowModal();
            return;
        }
    }

    // URL正确且没有重复时，添加新页面
    if (!res.IsEmpty() && !username.IsEmpty()) {
        DeletePageByLabel("refresh");
        addWebPanel(res, username);
    }
    m_config->WriteItem("printerIP." + bnm, res);

}

void WebPage::onItemButton(wxMouseEvent &event)
{
    //页面跳转
    auto id   = event.GetId();
    auto iter = m_page_map.find(id);
    if (iter != m_page_map.end()) {
        auto webPanel = iter->second;
        int  index    = m_webManager->FindPage(webPanel);
        m_webManager->SetSelection(index);
    } else {
        wxMessageBox(_("find web page failed"), _("warring"));
    }
}

void WebPage::onItemRightButton(wxMouseEvent &event)
{
    auto id         = event.GetId();
    m_select_button = id;
    // auto pos = event.GetPosition(); //此种方式 pos 是响应鼠标控件(Item)内的坐标系
    // PopupMenu 使用的是控件的局部坐标系
    auto globalPos = wxGetMousePosition();
    auto pos       = m_item_panel->ScreenToClient(globalPos);
    PopupMenu(m_menu, pos);
}

void WebPage::onMenuItem(wxCommandEvent &event)
{
    int id = event.GetId();

    if (id == WEBITEMBUTTONSTART + 1) // delete
    {
        auto get_button = m_button_map.find(m_select_button);
        if (get_button != m_button_map.end()) {
            TopButton *button = get_button->second;
           
            button->Hide();
            std::string lable_name         = button->GetLabel().ToStdString();
            lable_name             = wxGetApp().dot_turn_symbol(lable_name);
             
            m_name_and_ip.erase(lable_name);
            m_config->DeleteNodeValue();

            for (auto iter : m_name_and_ip) {
                m_config->WriteItem("printerIP." + iter.first.ToStdString(), iter.second);
            }

            m_item_panel->GetSizer()->Detach(button);
            delete button;
            button = nullptr;
            m_item_panel->Layout();
            m_button_map.erase(get_button);
        }

        auto pageIter = m_page_map.find(m_select_button);
        if (pageIter != m_page_map.end()) {
            auto page  = pageIter->second;
            int  index = m_webManager->FindPage(page);
            m_webManager->DeletePage(index);
            m_page_map.erase(pageIter);
            if (m_webManager->GetPageCount() == 0) 
            {
                CreateRefenshPage();
                m_webManager->AddPage(init_panel, "refresh", true);
            }
        }
        if (!m_page_map.empty()) 
        {
            for (auto iter : m_page_map) 
            {
                if (m_webManager->GetCurrentPage() == iter.second) 
                {
                    m_focus_buttonid = iter.first;
                    m_button_map[m_focus_buttonid]->SetForegroundColour(wxColor(20, 20, 20));
                    m_button_map[m_focus_buttonid]->SetBackgroundColour(m_select_color);
                    Refresh();
                    break;
                }
            }
        }
    }
}

void WebPage::update() {}

} // namespace GUI
GUI::AddDialog::AddDialog(const wxString &title)
    : DPIDialog(NULL, wxID_ANY, title, wxDefaultPosition, wxDefaultSize /*wxSize(400,220)*/, wxDEFAULT_DIALOG_STYLE)
{
    CenterOnParent();
    // 移除大小调整边框样式
   // SetWindowStyle(GetWindowStyle() & ~wxRESIZE_BORDER);
    wxPanel *    panel      = new wxPanel(this);
    const float &em         = em_unit() / 10;
    wxBoxSizer * sizer      = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer * name_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer * IP_sizer   = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer * btn_sizer  = new wxBoxSizer(wxHORIZONTAL);

    wxStaticText *usernameLabel = new wxStaticText(panel, wxID_ANY, _L("Printer Name: "));
    m_usernameTextCtrl          = new wxTextCtrl(panel, wxID_ANY);
    wxStaticText *passwordLabel = new wxStaticText(panel, wxID_ANY, _L("Printer IP: "));
    m_passwordTextCtrl          = new wxTextCtrl(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize);
    m_usernameTextCtrl->SetMinSize(wxSize(200 * em, 25 * em));
    m_passwordTextCtrl->SetMinSize(wxSize(200 * em, 25 * em));

    loginButton            = new wxButton(panel, wxID_OK, _L("OK"));
    wxButton *cancelButton = new wxButton(panel, wxID_CANCEL, _L("Cancel"));

    SetDefaultItem(loginButton);

    name_sizer->Add(usernameLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    name_sizer->Add(m_usernameTextCtrl, 1, /*wxEXPAND |*/ wxALL, 5);

    name_sizer->Add(passwordLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    name_sizer->Add(m_passwordTextCtrl, 1, /*wxEXPAND |*/ wxALL, 5);
    sizer->Add(name_sizer, 1, wxEXPAND | wxALL, 5);

    btn_sizer->Add(0, 0, 1, wxEXPAND, 5);
    btn_sizer->Add(loginButton, 0, wxALL, 5);
    btn_sizer->Add(cancelButton, 0, wxALL, 5);
    btn_sizer->Add(0, 0, 1, wxEXPAND, 5);

    sizer->Add(btn_sizer, 1, wxEXPAND | wxALL, 5);
    panel->SetSizer(sizer);
    wxStaticBitmap *m_bitmap1 = new wxStaticBitmap(this, wxID_ANY, *get_bmp_bundle("flsun_2.png", 120), wxDefaultPosition, wxDefaultSize, 0);
    wxBoxSizer *    sizer_dia = new wxBoxSizer(wxHORIZONTAL);
    sizer_dia->Add(m_bitmap1, 0, wxTOP, 20 * em);
    sizer_dia->Add(panel, 1, wxEXPAND);
    SetSizer(sizer_dia);
    Bind(wxEVT_TIMER, [=](wxTimerEvent &event) {
        if (m_usernameTextCtrl->GetValue().empty() || m_passwordTextCtrl->GetValue().empty()) {
            loginButton->Enable(false);
        } else {
            loginButton->Enable(true);
        }
    });
    wxTimer *timer = new wxTimer(this, wxID_ANY);
    timer->SetOwner(this);
    timer->Start(500);
     Fit();
}
// namespace GUI
void GUI::AddDialog::on_dpi_changed(const wxRect &suggested_rect) 
{
        const int &em = em_unit();
        msw_buttons_rescale(this, em, {wxID_OK});
        Layout();
        Fit();
        Refresh();
}
} // namespace Slic3r