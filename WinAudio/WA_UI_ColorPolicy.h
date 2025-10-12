
#ifndef WA_UI_COLORPOLICY_H
#define WA_UI_COLORPOLICY_H

typedef enum TagColorMode
{
	Light,
	Dark
} ColorMode;

typedef enum TagColorThemes
{
	Red,
	Orange,
	Yellow,
	Green,
	Blue,
	Violet
} ColorThemes;

#define WA_UI_COLORPOLICY_COLOR_NAMES_LEN 6
#define WA_UI_COLORPOLICY_COLOR_NAMES_TEXT_LEN 10

static const wchar_t WA_UI_ColoPolicy_ColorNamesArr[WA_UI_COLORPOLICY_COLOR_NAMES_LEN][WA_UI_COLORPOLICY_COLOR_NAMES_TEXT_LEN] = { L"Red", L"Orange" , L"Yellow" , L"Green" ,L"Blue" , L"Violet" };

typedef struct TagUIColors
{
	COLORREF Primary;
	COLORREF PrimaryVariant;
	COLORREF Secondary;
	COLORREF SecondaryVariant;
	COLORREF Background;
	COLORREF Surface;
	COLORREF TextOnPrimary;
	COLORREF TextOnSecondary;
	COLORREF TextOnBackground;
	COLORREF TextOnSurface;
} UIColors;

void ColorPolicy_Init(ColorMode Mode, ColorThemes Theme, UINT uCurrentDpi);
void ColorPolicy_Close();

HFONT ColorPolicy_Get_Default_Font();

COLORREF ColorPolicy_Get_Primary_Color();
COLORREF ColorPolicy_Get_PrimaryVariant_Color();
COLORREF ColorPolicy_Get_Secondary_Color();
COLORREF ColorPolicy_Get_SecondaryVariant_Color();
COLORREF ColorPolicy_Get_Background_Color();
COLORREF ColorPolicy_Get_Surface_Color();
COLORREF ColorPolicy_Get_TextOnPrimary_Color();
COLORREF ColorPolicy_Get_TextOnSecondary_Color();
COLORREF ColorPolicy_Get_TextOnBackground_Color();
COLORREF ColorPolicy_Get_TextOnSurface_Color();

HBRUSH ColorPolicy_Get_Primary_Brush();
HBRUSH ColorPolicy_Get_PrimaryVariant_Brush();
HBRUSH ColorPolicy_Get_Secondary_Brush();
HBRUSH ColorPolicy_Get_SecondaryVariant_Brush();
HBRUSH ColorPolicy_Get_Background_Brush();
HBRUSH ColorPolicy_Get_Surface_Brush();
HBRUSH ColorPolicy_Get_TextOnPrimary_Brush();
HBRUSH ColorPolicy_Get_TextOnSecondary_Brush();
HBRUSH ColorPolicy_Get_TextOnBackground_Brush();
HBRUSH ColorPolicy_Get_TextOnSurface_Brush();

HPEN ColorPolicy_Get_Primary_Pen();
HPEN ColorPolicy_Get_PrimaryVariant_Pen();
HPEN ColorPolicy_Get_Secondary_Pen();
HPEN ColorPolicy_Get_SecondaryVariant_Pen();
HPEN ColorPolicy_Get_Background_Pen();
HPEN ColorPolicy_Get_Surface_Pen();
HPEN ColorPolicy_Get_TextOnPrimary_Pen();
HPEN ColorPolicy_Get_TextOnSecondary_Pen();
HPEN ColorPolicy_Get_TextOnBackground_Pen();
HPEN ColorPolicy_Get_TextOnSurface_Pen();



#endif
