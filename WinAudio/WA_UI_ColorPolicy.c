
#include "stdafx.h"
#include "WA_UI_ColorPolicy.h"

// see https://coolors.co/ for palette ref

#define PEN_WIDTH 1 // Pen Witdh
#define PEN_STYLE PS_SOLID // Pen Style ref: https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createpen

typedef struct TagUIPalette
{
	ColorMode Mode;
	ColorThemes Theme;
	UIColors Color;

	HBRUSH PrimaryBrush;
	HBRUSH PrimaryVariantBrush;
	HBRUSH SecondaryBrush;
	HBRUSH SecondaryVariantBrush;
	HBRUSH BackgroundBrush;
	HBRUSH SurfaceBrush;
	HBRUSH TextOnPrimaryBrush;
	HBRUSH TextOnSecondaryBrush;
	HBRUSH TextOnBackgroundBrush;
	HBRUSH TextOnSurfaceBrush;

	HPEN PrimaryPen;
	HPEN PrimaryVariantPen;
	HPEN SecondaryPen;
	HPEN SecondaryVariantPen;
	HPEN BackgroundPen;
	HPEN SurfacePen;
	HPEN TextOnPrimaryPen;
	HPEN TextOnSecondaryPen;
	HPEN TextOnBackgroundPen;
	HPEN TextOnSurfacePen;

	HFONT hDefaultFont;
} UIPalette;

static const UIColors DarkMode_Red =
{
RGB(123,59,50), // Primary
RGB(73,55,54), // PrimaryVariant
RGB(242,112,89), // Secondary
RGB(185,215,217), // SecondaryVariant
RGB(20,20,20), // Background
RGB(30,30,30), // Surface
RGB(255,255,255), // TextOnPrimary
RGB(40,40,40), // TextOnSecondary
RGB(255,255,255), // TextOnBackground
RGB(255,255,255)  // TextOnSurface
};

static const UIColors LightMode_Red =
{
RGB(255,202,212), // Primary
RGB(244,172,183), // PrimaryVariant
RGB(216,226,220), // Secondary
RGB(255,229,217), // SecondaryVariant
RGB(190,190,190), // Background
RGB(211,211,211), // Surface
RGB(10,10,10), // TextOnPrimary
RGB(10,10,10), // TextOnSecondary
RGB(10,10,10), // TextOnBackground
RGB(10,10,10)  // TextOnSurface
};

UIPalette Palette;


// https://github.com/notepad-plus-plus/notepad-plus-plus/blob/master/PowerEditor/src/NppDarkMode.h

static void ColorPolicy_InitLight(ColorThemes Theme)
{
	switch (Theme)
	{
	case Red:
		Palette.PrimaryBrush = CreateSolidBrush(LightMode_Red.Primary);
		Palette.PrimaryVariantBrush = CreateSolidBrush(LightMode_Red.PrimaryVariant);
		Palette.SecondaryBrush = CreateSolidBrush(LightMode_Red.Secondary);
		Palette.SecondaryVariantBrush = CreateSolidBrush(LightMode_Red.SecondaryVariant);
		Palette.BackgroundBrush = CreateSolidBrush(LightMode_Red.Background);
		Palette.SurfaceBrush = CreateSolidBrush(LightMode_Red.Surface);
		Palette.TextOnPrimaryBrush = CreateSolidBrush(LightMode_Red.TextOnPrimary);
		Palette.TextOnSecondaryBrush = CreateSolidBrush(LightMode_Red.TextOnSecondary);
		Palette.TextOnBackgroundBrush = CreateSolidBrush(LightMode_Red.TextOnBackground);
		Palette.TextOnSurfaceBrush = CreateSolidBrush(LightMode_Red.TextOnSurface);

		Palette.PrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Red.Primary);
		Palette.PrimaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Red.PrimaryVariant);
		Palette.SecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Red.Secondary);
		Palette.SecondaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Red.SecondaryVariant);
		Palette.BackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Red.Background);
		Palette.SurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Red.Surface);
		Palette.TextOnPrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Red.TextOnPrimary);
		Palette.TextOnSecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Red.TextOnSecondary);
		Palette.TextOnBackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Red.TextOnBackground);
		Palette.TextOnSurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Red.TextOnSurface);

		Palette.Color = LightMode_Red;

		break;
	case Orange:
		break;
	case Yellow:
		break;
	case Green:
		break;
	case Blue:
		break;
	case Violet:
		break;
	}
}

static void ColorPolicy_InitDark(ColorThemes Theme)
{

	switch (Theme)
	{
	case Red:
		Palette.PrimaryBrush = CreateSolidBrush(DarkMode_Red.Primary);
		Palette.PrimaryVariantBrush = CreateSolidBrush(DarkMode_Red.PrimaryVariant);
		Palette.SecondaryBrush = CreateSolidBrush(DarkMode_Red.Secondary);
		Palette.SecondaryVariantBrush = CreateSolidBrush(DarkMode_Red.SecondaryVariant);
		Palette.BackgroundBrush = CreateSolidBrush(DarkMode_Red.Background);
		Palette.SurfaceBrush = CreateSolidBrush(DarkMode_Red.Surface);
		Palette.TextOnPrimaryBrush = CreateSolidBrush(DarkMode_Red.TextOnPrimary);
		Palette.TextOnSecondaryBrush = CreateSolidBrush(DarkMode_Red.TextOnSecondary);
		Palette.TextOnBackgroundBrush = CreateSolidBrush(DarkMode_Red.TextOnBackground);
		Palette.TextOnSurfaceBrush = CreateSolidBrush(DarkMode_Red.TextOnSurface);

		Palette.PrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Red.Primary);
		Palette.PrimaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Red.PrimaryVariant);
		Palette.SecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Red.Secondary);
		Palette.SecondaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Red.SecondaryVariant);
		Palette.BackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Red.Background);
		Palette.SurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Red.Surface);
		Palette.TextOnPrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Red.TextOnPrimary);
		Palette.TextOnSecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Red.TextOnSecondary);
		Palette.TextOnBackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Red.TextOnBackground);
		Palette.TextOnSurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Red.TextOnSurface);

		Palette.Color = DarkMode_Red;
		break;
	case Orange:
		break;
	case Yellow:
		break;
	case Green:
		break;
	case Blue:
		break;
	case Violet:
		break;
	}
}


void ColorPolicy_Init(ColorMode Mode, ColorThemes Theme)
{
	LOGFONT LogFont;


	Palette.Mode = Mode;
	Palette.Theme = Theme;

	switch (Mode)
	{
	case Light:
		ColorPolicy_InitLight(Theme);
		break;
	case Dark:
		ColorPolicy_InitDark(Theme);
		break;
	}

	// Create Font Object
	SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LogFont), &LogFont, 0);
	Palette.hDefaultFont = CreateFontIndirect(&LogFont);

}

void ColorPolicy_Close()
{

	DeleteObject(Palette.PrimaryBrush);
	DeleteObject(Palette.PrimaryVariantBrush);
	DeleteObject(Palette.SecondaryBrush);
	DeleteObject(Palette.SecondaryVariantBrush);
	DeleteObject(Palette.BackgroundBrush);
	DeleteObject(Palette.SurfaceBrush);
	DeleteObject(Palette.TextOnPrimaryBrush);
	DeleteObject(Palette.TextOnSecondaryBrush);
	DeleteObject(Palette.TextOnBackgroundBrush);
	DeleteObject(Palette.TextOnSurfaceBrush);

	DeleteObject(Palette.PrimaryPen);
	DeleteObject(Palette.PrimaryVariantPen);
	DeleteObject(Palette.SecondaryPen);
	DeleteObject(Palette.SecondaryVariantPen);
	DeleteObject(Palette.BackgroundPen);
	DeleteObject(Palette.SurfacePen);
	DeleteObject(Palette.TextOnPrimaryPen);
	DeleteObject(Palette.TextOnSecondaryPen);
	DeleteObject(Palette.TextOnBackgroundPen);
	DeleteObject(Palette.TextOnSurfacePen);

	DeleteObject(Palette.hDefaultFont);
}

HFONT ColorPolicy_Get_Default_Font() { return Palette.hDefaultFont; }

COLORREF ColorPolicy_Get_Primary_Color() { return Palette.Color.Primary; }
COLORREF ColorPolicy_Get_PrimaryVariant_Color() { return Palette.Color.PrimaryVariant;  }
COLORREF ColorPolicy_Get_Secondary_Color(){ return Palette.Color.Secondary; }
COLORREF ColorPolicy_Get_SecondaryVariant_Color(){ return Palette.Color.SecondaryVariant; }
COLORREF ColorPolicy_Get_Background_Color() { return Palette.Color.Background; }
COLORREF ColorPolicy_Get_Surface_Color(){ return Palette.Color.Surface; }
COLORREF ColorPolicy_Get_TextOnPrimary_Color(){ return Palette.Color.TextOnPrimary; }
COLORREF ColorPolicy_Get_TextOnSecondary_Color(){ return Palette.Color.TextOnSecondary; }
COLORREF ColorPolicy_Get_TextOnBackground_Color() { return Palette.Color.TextOnBackground; }
COLORREF ColorPolicy_Get_TextOnSurface_Color() { return Palette.Color.TextOnSurface; }


HBRUSH ColorPolicy_Get_Primary_Brush() { return Palette.PrimaryBrush; }
HBRUSH ColorPolicy_Get_PrimaryVariant_Brush(){ return Palette.PrimaryVariantBrush; }
HBRUSH ColorPolicy_Get_Secondary_Brush(){ return Palette.SecondaryBrush; }
HBRUSH ColorPolicy_Get_SecondaryVariant_Brush(){ return Palette.SecondaryVariantBrush; }
HBRUSH ColorPolicy_Get_Background_Brush(){ return Palette.BackgroundBrush; }
HBRUSH ColorPolicy_Get_Surface_Brush(){	return Palette.SurfaceBrush; }
HBRUSH ColorPolicy_Get_TextOnPrimary_Brush(){ return Palette.TextOnPrimaryBrush; }
HBRUSH ColorPolicy_Get_TextOnSecondary_Brush(){	return Palette.TextOnSecondaryBrush; }
HBRUSH ColorPolicy_Get_TextOnBackground_Brush(){ return Palette.TextOnBackgroundBrush; }
HBRUSH ColorPolicy_Get_TextOnSurface_Brush(){ return Palette.TextOnSurfaceBrush; }

HPEN ColorPolicy_Get_Primary_Pen() { return Palette.PrimaryPen; }
HPEN ColorPolicy_Get_PrimaryVariant_Pen() { return Palette.PrimaryVariantPen; }
HPEN ColorPolicy_Get_Secondary_Pen() { return Palette.SecondaryPen; }
HPEN ColorPolicy_Get_SecondaryVariant_Pen() { return Palette.SecondaryVariantPen; }
HPEN ColorPolicy_Get_Background_Pen() { return Palette.BackgroundPen; }
HPEN ColorPolicy_Get_Surface_Pen() { return Palette.SurfacePen; }
HPEN ColorPolicy_Get_TextOnPrimary_Pen() { return Palette.TextOnPrimaryPen; }
HPEN ColorPolicy_Get_TextOnSecondary_Pen() { return Palette.TextOnSecondaryPen; }
HPEN ColorPolicy_Get_TextOnBackground_Pen() { return Palette.TextOnBackgroundPen; }
HPEN ColorPolicy_Get_TextOnSurface_Pen() { return Palette.TextOnSurfacePen; }

