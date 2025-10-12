
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
RGB(200,200,200), // Background
RGB(211,211,211), // Surface
RGB(10,10,10), // TextOnPrimary
RGB(10,10,10), // TextOnSecondary
RGB(10,10,10), // TextOnBackground
RGB(10,10,10)  // TextOnSurface
};

static const UIColors DarkMode_Orange =
{
RGB(255,149,5), // Primary
RGB(226,113,29), // PrimaryVariant
RGB(255,201,113), // Secondary
RGB(255,182,39), // SecondaryVariant
RGB(20,20,20), // Background
RGB(30,30,30), // Surface
RGB(255,255,255), // TextOnPrimary
RGB(255,255,255), // TextOnSecondary
RGB(255,255,255), // TextOnBackground
RGB(255,255,255)  // TextOnSurface
};

static const UIColors LightMode_Orange =
{
RGB(255,136,0), // Primary
RGB(255,149,0), // PrimaryVariant
RGB(255,183,0), // Secondary
RGB(255,170,0), // SecondaryVariant
RGB(200,200,200), // Background
RGB(211,211,211), // Surface
RGB(10,10,10), // TextOnPrimary
RGB(10,10,10), // TextOnSecondary
RGB(10,10,10), // TextOnBackground
RGB(10,10,10)  // TextOnSurface
};


static const UIColors DarkMode_Yellow =
{
RGB(255,247,95), // Primary
RGB(254,207,62), // PrimaryVariant
RGB(253,184,51), // Secondary
RGB(253,190,57), // SecondaryVariant
RGB(20,20,20), // Background
RGB(30,30,30), // Surface
RGB(10,10,10), // TextOnPrimary
RGB(10,10,10), // TextOnSecondary
RGB(255,255,255), // TextOnBackground
RGB(255,255,255)  // TextOnSurface
};

static const UIColors LightMode_Yellow =
{
RGB(255,247,95), // Primary
RGB(254,207,62), // PrimaryVariant
RGB(253,184,51), // Secondary
RGB(253,190,57), // SecondaryVariant
RGB(200,200,200), // Background
RGB(211,211,211), // Surface
RGB(10,10,10), // TextOnPrimary
RGB(10,10,10), // TextOnSecondary
RGB(10,10,10), // TextOnBackground
RGB(10,10,10)  // TextOnSurface
};

static const UIColors DarkMode_Green =
{
RGB(88,129,87), // Primary
RGB(163,177,138), // PrimaryVariant
RGB(58,90,64), // Secondary
RGB(52,78,65), // SecondaryVariant
RGB(20,20,20), // Background
RGB(30,30,30), // Surface
RGB(255,255,255), // TextOnPrimary
RGB(255,255,255), // TextOnSecondary
RGB(255,255,255), // TextOnBackground
RGB(255,255,255)  // TextOnSurface
};

static const UIColors LightMode_Green =
{
RGB(88,129,87), // Primary
RGB(163,177,138), // PrimaryVariant
RGB(58,90,64), // Secondary
RGB(52,78,65), // SecondaryVariant
RGB(200,200,200), // Background
RGB(211,211,211), // Surface
RGB(10,10,10), // TextOnPrimary
RGB(218,215,205), // TextOnSecondary
RGB(10,10,10), // TextOnBackground
RGB(10,10,10)  // TextOnSurface
};

static const UIColors DarkMode_Blue =
{
RGB(27,73,101), // Primary
RGB(98,182,203), // PrimaryVariant
RGB(95,168,211), // Secondary
RGB(190,233,232), // SecondaryVariant
RGB(20,20,20), // Background
RGB(30,30,30), // Surface
RGB(255,255,255), // TextOnPrimary
RGB(255,255,255), // TextOnSecondary
RGB(255,255,255), // TextOnBackground
RGB(255,255,255)  // TextOnSurface
};

static const UIColors LightMode_Blue =
{
RGB(27,73,101), // Primary
RGB(98,182,203), // PrimaryVariant
RGB(95,168,211), // Secondary
RGB(190,233,232), // SecondaryVariant
RGB(200,200,200), // Background
RGB(211,211,211), // Surface
RGB(240,240,240), // TextOnPrimary
RGB(10,10,10), // TextOnSecondary
RGB(10,10,10), // TextOnBackground
RGB(10,10,10)  // TextOnSurface
};

static const UIColors DarkMode_Violet =
{
RGB(183,156,237), // Primary
RGB(149,127,239), // PrimaryVariant
RGB(222,192,241), // Secondary
RGB(113,97,239), // SecondaryVariant
RGB(20,20,20), // Background
RGB(30,30,30), // Surface
RGB(255,255,255), // TextOnPrimary
RGB(30,30,30), // TextOnSecondary
RGB(255,255,255), // TextOnBackground
RGB(255,255,255)  // TextOnSurface
};

static const UIColors LightMode_Violet =
{
RGB(183,156,237), // Primary
RGB(149,127,239), // PrimaryVariant
RGB(222,192,241), // Secondary
RGB(113,97,239), // SecondaryVariant
RGB(200,200,200), // Background
RGB(211,211,211), // Surface
RGB(10,10,10), // TextOnPrimary
RGB(10,10,10), // TextOnSecondary
RGB(10,10,10), // TextOnBackground
RGB(10,10,10)  // TextOnSurface
};



static UIPalette Palette;


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
		Palette.PrimaryBrush = CreateSolidBrush(LightMode_Orange.Primary);
		Palette.PrimaryVariantBrush = CreateSolidBrush(LightMode_Orange.PrimaryVariant);
		Palette.SecondaryBrush = CreateSolidBrush(LightMode_Orange.Secondary);
		Palette.SecondaryVariantBrush = CreateSolidBrush(LightMode_Orange.SecondaryVariant);
		Palette.BackgroundBrush = CreateSolidBrush(LightMode_Orange.Background);
		Palette.SurfaceBrush = CreateSolidBrush(LightMode_Orange.Surface);
		Palette.TextOnPrimaryBrush = CreateSolidBrush(LightMode_Orange.TextOnPrimary);
		Palette.TextOnSecondaryBrush = CreateSolidBrush(LightMode_Orange.TextOnSecondary);
		Palette.TextOnBackgroundBrush = CreateSolidBrush(LightMode_Orange.TextOnBackground);
		Palette.TextOnSurfaceBrush = CreateSolidBrush(LightMode_Orange.TextOnSurface);

		Palette.PrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Orange.Primary);
		Palette.PrimaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Orange.PrimaryVariant);
		Palette.SecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Orange.Secondary);
		Palette.SecondaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Orange.SecondaryVariant);
		Palette.BackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Orange.Background);
		Palette.SurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Orange.Surface);
		Palette.TextOnPrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Orange.TextOnPrimary);
		Palette.TextOnSecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Orange.TextOnSecondary);
		Palette.TextOnBackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Orange.TextOnBackground);
		Palette.TextOnSurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Orange.TextOnSurface);

		Palette.Color = LightMode_Orange;
		break;
	case Yellow:
		Palette.PrimaryBrush = CreateSolidBrush(LightMode_Yellow.Primary);
		Palette.PrimaryVariantBrush = CreateSolidBrush(LightMode_Yellow.PrimaryVariant);
		Palette.SecondaryBrush = CreateSolidBrush(LightMode_Yellow.Secondary);
		Palette.SecondaryVariantBrush = CreateSolidBrush(LightMode_Yellow.SecondaryVariant);
		Palette.BackgroundBrush = CreateSolidBrush(LightMode_Yellow.Background);
		Palette.SurfaceBrush = CreateSolidBrush(LightMode_Yellow.Surface);
		Palette.TextOnPrimaryBrush = CreateSolidBrush(LightMode_Yellow.TextOnPrimary);
		Palette.TextOnSecondaryBrush = CreateSolidBrush(LightMode_Yellow.TextOnSecondary);
		Palette.TextOnBackgroundBrush = CreateSolidBrush(LightMode_Yellow.TextOnBackground);
		Palette.TextOnSurfaceBrush = CreateSolidBrush(LightMode_Yellow.TextOnSurface);

		Palette.PrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Yellow.Primary);
		Palette.PrimaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Yellow.PrimaryVariant);
		Palette.SecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Yellow.Secondary);
		Palette.SecondaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Yellow.SecondaryVariant);
		Palette.BackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Yellow.Background);
		Palette.SurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Yellow.Surface);
		Palette.TextOnPrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Yellow.TextOnPrimary);
		Palette.TextOnSecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Yellow.TextOnSecondary);
		Palette.TextOnBackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Yellow.TextOnBackground);
		Palette.TextOnSurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Yellow.TextOnSurface);

		Palette.Color = LightMode_Yellow;
		break;
	case Green:
		Palette.PrimaryBrush = CreateSolidBrush(LightMode_Green.Primary);
		Palette.PrimaryVariantBrush = CreateSolidBrush(LightMode_Green.PrimaryVariant);
		Palette.SecondaryBrush = CreateSolidBrush(LightMode_Green.Secondary);
		Palette.SecondaryVariantBrush = CreateSolidBrush(LightMode_Green.SecondaryVariant);
		Palette.BackgroundBrush = CreateSolidBrush(LightMode_Green.Background);
		Palette.SurfaceBrush = CreateSolidBrush(LightMode_Green.Surface);
		Palette.TextOnPrimaryBrush = CreateSolidBrush(LightMode_Green.TextOnPrimary);
		Palette.TextOnSecondaryBrush = CreateSolidBrush(LightMode_Green.TextOnSecondary);
		Palette.TextOnBackgroundBrush = CreateSolidBrush(LightMode_Green.TextOnBackground);
		Palette.TextOnSurfaceBrush = CreateSolidBrush(LightMode_Green.TextOnSurface);

		Palette.PrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Green.Primary);
		Palette.PrimaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Green.PrimaryVariant);
		Palette.SecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Green.Secondary);
		Palette.SecondaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Green.SecondaryVariant);
		Palette.BackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Green.Background);
		Palette.SurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Green.Surface);
		Palette.TextOnPrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Green.TextOnPrimary);
		Palette.TextOnSecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Green.TextOnSecondary);
		Palette.TextOnBackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Green.TextOnBackground);
		Palette.TextOnSurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Green.TextOnSurface);

		Palette.Color = LightMode_Green;
		break;
	case Blue:
		Palette.PrimaryBrush = CreateSolidBrush(LightMode_Blue.Primary);
		Palette.PrimaryVariantBrush = CreateSolidBrush(LightMode_Blue.PrimaryVariant);
		Palette.SecondaryBrush = CreateSolidBrush(LightMode_Blue.Secondary);
		Palette.SecondaryVariantBrush = CreateSolidBrush(LightMode_Blue.SecondaryVariant);
		Palette.BackgroundBrush = CreateSolidBrush(LightMode_Blue.Background);
		Palette.SurfaceBrush = CreateSolidBrush(LightMode_Blue.Surface);
		Palette.TextOnPrimaryBrush = CreateSolidBrush(LightMode_Blue.TextOnPrimary);
		Palette.TextOnSecondaryBrush = CreateSolidBrush(LightMode_Blue.TextOnSecondary);
		Palette.TextOnBackgroundBrush = CreateSolidBrush(LightMode_Blue.TextOnBackground);
		Palette.TextOnSurfaceBrush = CreateSolidBrush(LightMode_Blue.TextOnSurface);

		Palette.PrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Blue.Primary);
		Palette.PrimaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Blue.PrimaryVariant);
		Palette.SecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Blue.Secondary);
		Palette.SecondaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Blue.SecondaryVariant);
		Palette.BackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Blue.Background);
		Palette.SurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Blue.Surface);
		Palette.TextOnPrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Blue.TextOnPrimary);
		Palette.TextOnSecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Blue.TextOnSecondary);
		Palette.TextOnBackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Blue.TextOnBackground);
		Palette.TextOnSurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Blue.TextOnSurface);

		Palette.Color = LightMode_Blue;
		break;
	case Violet:
		Palette.PrimaryBrush = CreateSolidBrush(LightMode_Violet.Primary);
		Palette.PrimaryVariantBrush = CreateSolidBrush(LightMode_Violet.PrimaryVariant);
		Palette.SecondaryBrush = CreateSolidBrush(LightMode_Violet.Secondary);
		Palette.SecondaryVariantBrush = CreateSolidBrush(LightMode_Violet.SecondaryVariant);
		Palette.BackgroundBrush = CreateSolidBrush(LightMode_Violet.Background);
		Palette.SurfaceBrush = CreateSolidBrush(LightMode_Violet.Surface);
		Palette.TextOnPrimaryBrush = CreateSolidBrush(LightMode_Violet.TextOnPrimary);
		Palette.TextOnSecondaryBrush = CreateSolidBrush(LightMode_Violet.TextOnSecondary);
		Palette.TextOnBackgroundBrush = CreateSolidBrush(LightMode_Violet.TextOnBackground);
		Palette.TextOnSurfaceBrush = CreateSolidBrush(LightMode_Violet.TextOnSurface);

		Palette.PrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Violet.Primary);
		Palette.PrimaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Violet.PrimaryVariant);
		Palette.SecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Violet.Secondary);
		Palette.SecondaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Violet.SecondaryVariant);
		Palette.BackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Violet.Background);
		Palette.SurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Violet.Surface);
		Palette.TextOnPrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Violet.TextOnPrimary);
		Palette.TextOnSecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Violet.TextOnSecondary);
		Palette.TextOnBackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Violet.TextOnBackground);
		Palette.TextOnSurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, LightMode_Violet.TextOnSurface);

		Palette.Color = LightMode_Violet;
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
		Palette.PrimaryBrush = CreateSolidBrush(DarkMode_Orange.Primary);
		Palette.PrimaryVariantBrush = CreateSolidBrush(DarkMode_Orange.PrimaryVariant);
		Palette.SecondaryBrush = CreateSolidBrush(DarkMode_Orange.Secondary);
		Palette.SecondaryVariantBrush = CreateSolidBrush(DarkMode_Orange.SecondaryVariant);
		Palette.BackgroundBrush = CreateSolidBrush(DarkMode_Orange.Background);
		Palette.SurfaceBrush = CreateSolidBrush(DarkMode_Orange.Surface);
		Palette.TextOnPrimaryBrush = CreateSolidBrush(DarkMode_Orange.TextOnPrimary);
		Palette.TextOnSecondaryBrush = CreateSolidBrush(DarkMode_Orange.TextOnSecondary);
		Palette.TextOnBackgroundBrush = CreateSolidBrush(DarkMode_Orange.TextOnBackground);
		Palette.TextOnSurfaceBrush = CreateSolidBrush(DarkMode_Orange.TextOnSurface);

		Palette.PrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Orange.Primary);
		Palette.PrimaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Orange.PrimaryVariant);
		Palette.SecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Orange.Secondary);
		Palette.SecondaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Orange.SecondaryVariant);
		Palette.BackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Orange.Background);
		Palette.SurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Orange.Surface);
		Palette.TextOnPrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Orange.TextOnPrimary);
		Palette.TextOnSecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Orange.TextOnSecondary);
		Palette.TextOnBackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Orange.TextOnBackground);
		Palette.TextOnSurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Orange.TextOnSurface);

		Palette.Color = DarkMode_Orange;

		break;
	case Yellow:
		Palette.PrimaryBrush = CreateSolidBrush(DarkMode_Yellow.Primary);
		Palette.PrimaryVariantBrush = CreateSolidBrush(DarkMode_Yellow.PrimaryVariant);
		Palette.SecondaryBrush = CreateSolidBrush(DarkMode_Yellow.Secondary);
		Palette.SecondaryVariantBrush = CreateSolidBrush(DarkMode_Yellow.SecondaryVariant);
		Palette.BackgroundBrush = CreateSolidBrush(DarkMode_Yellow.Background);
		Palette.SurfaceBrush = CreateSolidBrush(DarkMode_Yellow.Surface);
		Palette.TextOnPrimaryBrush = CreateSolidBrush(DarkMode_Yellow.TextOnPrimary);
		Palette.TextOnSecondaryBrush = CreateSolidBrush(DarkMode_Yellow.TextOnSecondary);
		Palette.TextOnBackgroundBrush = CreateSolidBrush(DarkMode_Yellow.TextOnBackground);
		Palette.TextOnSurfaceBrush = CreateSolidBrush(DarkMode_Yellow.TextOnSurface);

		Palette.PrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Yellow.Primary);
		Palette.PrimaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Yellow.PrimaryVariant);
		Palette.SecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Yellow.Secondary);
		Palette.SecondaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Yellow.SecondaryVariant);
		Palette.BackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Yellow.Background);
		Palette.SurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Yellow.Surface);
		Palette.TextOnPrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Yellow.TextOnPrimary);
		Palette.TextOnSecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Yellow.TextOnSecondary);
		Palette.TextOnBackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Yellow.TextOnBackground);
		Palette.TextOnSurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Yellow.TextOnSurface);

		Palette.Color = DarkMode_Yellow;

		break;
	case Green:
		Palette.PrimaryBrush = CreateSolidBrush(DarkMode_Green.Primary);
		Palette.PrimaryVariantBrush = CreateSolidBrush(DarkMode_Green.PrimaryVariant);
		Palette.SecondaryBrush = CreateSolidBrush(DarkMode_Green.Secondary);
		Palette.SecondaryVariantBrush = CreateSolidBrush(DarkMode_Green.SecondaryVariant);
		Palette.BackgroundBrush = CreateSolidBrush(DarkMode_Green.Background);
		Palette.SurfaceBrush = CreateSolidBrush(DarkMode_Green.Surface);
		Palette.TextOnPrimaryBrush = CreateSolidBrush(DarkMode_Green.TextOnPrimary);
		Palette.TextOnSecondaryBrush = CreateSolidBrush(DarkMode_Green.TextOnSecondary);
		Palette.TextOnBackgroundBrush = CreateSolidBrush(DarkMode_Green.TextOnBackground);
		Palette.TextOnSurfaceBrush = CreateSolidBrush(DarkMode_Green.TextOnSurface);

		Palette.PrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Green.Primary);
		Palette.PrimaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Green.PrimaryVariant);
		Palette.SecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Green.Secondary);
		Palette.SecondaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Green.SecondaryVariant);
		Palette.BackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Green.Background);
		Palette.SurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Green.Surface);
		Palette.TextOnPrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Green.TextOnPrimary);
		Palette.TextOnSecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Green.TextOnSecondary);
		Palette.TextOnBackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Green.TextOnBackground);
		Palette.TextOnSurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Green.TextOnSurface);

		Palette.Color = DarkMode_Green;
		break;
	case Blue:
		Palette.PrimaryBrush = CreateSolidBrush(DarkMode_Blue.Primary);
		Palette.PrimaryVariantBrush = CreateSolidBrush(DarkMode_Blue.PrimaryVariant);
		Palette.SecondaryBrush = CreateSolidBrush(DarkMode_Blue.Secondary);
		Palette.SecondaryVariantBrush = CreateSolidBrush(DarkMode_Blue.SecondaryVariant);
		Palette.BackgroundBrush = CreateSolidBrush(DarkMode_Blue.Background);
		Palette.SurfaceBrush = CreateSolidBrush(DarkMode_Blue.Surface);
		Palette.TextOnPrimaryBrush = CreateSolidBrush(DarkMode_Blue.TextOnPrimary);
		Palette.TextOnSecondaryBrush = CreateSolidBrush(DarkMode_Blue.TextOnSecondary);
		Palette.TextOnBackgroundBrush = CreateSolidBrush(DarkMode_Blue.TextOnBackground);
		Palette.TextOnSurfaceBrush = CreateSolidBrush(DarkMode_Blue.TextOnSurface);

		Palette.PrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Blue.Primary);
		Palette.PrimaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Blue.PrimaryVariant);
		Palette.SecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Blue.Secondary);
		Palette.SecondaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Blue.SecondaryVariant);
		Palette.BackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Blue.Background);
		Palette.SurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Blue.Surface);
		Palette.TextOnPrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Blue.TextOnPrimary);
		Palette.TextOnSecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Blue.TextOnSecondary);
		Palette.TextOnBackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Blue.TextOnBackground);
		Palette.TextOnSurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Blue.TextOnSurface);

		Palette.Color = DarkMode_Blue;
		break;
	case Violet:
		Palette.PrimaryBrush = CreateSolidBrush(DarkMode_Violet.Primary);
		Palette.PrimaryVariantBrush = CreateSolidBrush(DarkMode_Violet.PrimaryVariant);
		Palette.SecondaryBrush = CreateSolidBrush(DarkMode_Violet.Secondary);
		Palette.SecondaryVariantBrush = CreateSolidBrush(DarkMode_Violet.SecondaryVariant);
		Palette.BackgroundBrush = CreateSolidBrush(DarkMode_Violet.Background);
		Palette.SurfaceBrush = CreateSolidBrush(DarkMode_Violet.Surface);
		Palette.TextOnPrimaryBrush = CreateSolidBrush(DarkMode_Violet.TextOnPrimary);
		Palette.TextOnSecondaryBrush = CreateSolidBrush(DarkMode_Violet.TextOnSecondary);
		Palette.TextOnBackgroundBrush = CreateSolidBrush(DarkMode_Violet.TextOnBackground);
		Palette.TextOnSurfaceBrush = CreateSolidBrush(DarkMode_Violet.TextOnSurface);

		Palette.PrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Violet.Primary);
		Palette.PrimaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Violet.PrimaryVariant);
		Palette.SecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Violet.Secondary);
		Palette.SecondaryVariantPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Violet.SecondaryVariant);
		Palette.BackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Violet.Background);
		Palette.SurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Violet.Surface);
		Palette.TextOnPrimaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Violet.TextOnPrimary);
		Palette.TextOnSecondaryPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Violet.TextOnSecondary);
		Palette.TextOnBackgroundPen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Violet.TextOnBackground);
		Palette.TextOnSurfacePen = CreatePen(PEN_STYLE, PEN_WIDTH, DarkMode_Violet.TextOnSurface);

		Palette.Color = DarkMode_Violet;
		break;
	}
}


void ColorPolicy_Init(ColorMode Mode, ColorThemes Theme, UINT uCurrentDpi)
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
	SystemParametersInfoForDpi(SPI_GETICONTITLELOGFONT, sizeof(LogFont), &LogFont, 0, uCurrentDpi);
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

