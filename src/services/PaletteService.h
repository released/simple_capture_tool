#pragma once
namespace sct::services {
struct PaletteData {std::vector<COLORREF> favorites,recent;};
bool ParseHexColor(CString,COLORREF&);
CString HexColor(COLORREF);
bool DecodePalette(const CString&,PaletteData&,CString&);
CString EncodePalette(const PaletteData&);
void RememberColor(PaletteData&,COLORREF);
bool ToggleFavorite(PaletteData&,COLORREF);
}
