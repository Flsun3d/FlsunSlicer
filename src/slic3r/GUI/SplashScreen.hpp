#pragma once
#include <wx/splash.h>

class SplashScreen : public wxSplashScreen
{
public:
	SplashScreen(const wxBitmap& bitmap, long splashStyle, int milliseconds, wxColour fontColor = *wxBLACK,wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT), wxPoint pos = wxDefaultPosition);
		
	~SplashScreen();

	void Decorate(wxBitmap& bmp);

	wxBitmap MakeBitmap(wxBitmap bmp);
	

	void SetText(const wxString& text ,wxColour color = *wxRED);

	void SetBitmap(wxBitmap& bmp);

protected:
	void init();


public:
	wxBitmap    m_main_bitmap;
	wxFont      m_action_font; //可变动字体
	wxColour    m_font_color;  //可变动字体颜色
	wxFont      m_font;        //字体
	int         m_action_line_y_position;
	float       m_scale{ 1.0 };
	
	wxString m_title;
	wxString m_version;
	wxString m_credits;
};

