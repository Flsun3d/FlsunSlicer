#include "SplashScreen.hpp"
#include "GUI_App.hpp"
#include <wx/dcmemory.h>
#include "I18N.hpp"
SplashScreen::SplashScreen(const wxBitmap& bitmap, long splashStyle, int milliseconds, wxColour fontColor, wxFont font, wxPoint pos)
	: wxSplashScreen(bitmap, splashStyle, milliseconds, NULL, wxID_ANY, pos, wxDefaultSize,
		wxSIMPLE_BORDER | wxFRAME_NO_TASKBAR)
{
	wxASSERT(bitmap.IsOk());

	this->SetPosition(pos);
	this->SetClientSize(bitmap.GetWidth(), bitmap.GetHeight());
	this->CenterOnScreen();

	m_font_color = fontColor;
	m_font = m_action_font = font;
	m_main_bitmap = MakeBitmap(bitmap);

	init();	
}

SplashScreen::~SplashScreen()
{
}

void SplashScreen::Decorate(wxBitmap& bmp)
{
	if (!bmp.IsOk())
		return;

	int width = lround(bmp.GetWidth());
	int height = lround(bmp.GetHeight() * 0.75);
	
	wxCoord margin = int(m_scale * 20);

	wxRect banner_rect(wxPoint(0, height), wxPoint(width, bmp.GetHeight()));
	banner_rect.Deflate(margin, margin);

	// use a memory DC to draw directly onto the bitmap
	wxMemoryDC memDc(bmp);
    m_font.SetPointSize(12);
	memDc.SetFont(m_font);
	memDc.SetTextForeground(wxColor(102,102,102));
    auto recy = banner_rect;
    recy.x    = 300;
    recy.y    = 90;
    memDc.DrawLabel(m_version, recy, wxALIGN_TOP | wxALIGN_LEFT);

	// calculate position for the dynamic text
	m_action_line_y_position = bmp.GetHeight() * 0.80;

}

wxBitmap SplashScreen::MakeBitmap(wxBitmap bmp)
{

	if (!bmp.IsOk())
		return wxNullBitmap;

	// create dark grey background for the splashscreen
	// It will be 5/3 of the weight of the bitmap
	int width = lround(bmp.GetWidth());
	int height = bmp.GetHeight();

	wxImage image(width, height);
	unsigned char* imgdata_ = image.GetData();
	for (int i = 0; i < width * height; ++i) {
		*imgdata_++ = 255;
		*imgdata_++ = 255;
		*imgdata_++ = 255;
	}

	wxBitmap new_bmp(image);
	wxMemoryDC memDC;
	memDC.SelectObject(new_bmp);
	memDC.DrawBitmap(bmp, 0, 0, true);

	return new_bmp;

}

void SplashScreen::SetText(const wxString& text , wxColour color)
{
	SetBitmap(m_main_bitmap);
    auto bitmapWidth = m_main_bitmap.GetWidth();
	if (!text.empty()) {
		wxBitmap bitmap(m_main_bitmap);

		wxMemoryDC memDC;
        memDC.SetFont(m_font);
		memDC.SelectObject(bitmap);
		//memDC.SetBackground(*wxBLACK_BRUSH);
		memDC.SetFont(m_action_font);
        memDC.SetTextForeground(wxColor(102, 102, 102));
        wxSize tysize;
        tysize.x = GetTextExtent(text).x;
        auto textlocation = (bitmapWidth - GetTextExtent(text).x) / 2;
        memDC.DrawText(text, int(m_scale * textlocation), m_action_line_y_position);
		memDC.SelectObject(wxNullBitmap);
		SetBitmap(bitmap);
	}
}

void SplashScreen::SetBitmap(wxBitmap& bmp)
{
	m_window->SetBitmap(bmp);
	m_window->Refresh();
	m_window->Update();
}

void SplashScreen::init()
{

	 // title
    m_title = Slic3r::GUI::wxGetApp().is_editor() ? SLIC3R_APP_NAME : GCODEVIEWER_APP_NAME;
    // dynamically get the version to display
    m_version = _L("V") + std::string(SLIC3R_VERSION);
	Decorate(m_main_bitmap);
}
