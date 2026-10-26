#pragma once
#include "../core/Document.h"
#include "../core/ColorFilters.h"
#include "../core/SelectionMask.h"
namespace sct::services {
struct StickerEntry {CString name,path;};
bool DecodeStickerLibrary(const CString&,std::vector<StickerEntry>&,CString&);
bool EncodeStickerLibrary(const std::vector<StickerEntry>&,CString&,CString&);
CString StickerPath(const CString& stored,const CString& directory);
CString PortableStickerPath(const CString& full,const CString& directory);
core::Image LoadSticker(const CString&);
core::Image StickerThumbnail(const core::Image&,int side);
CRect SelectedObjectBounds(const core::DocumentState&,const std::vector<int>&);
core::Image RenderSelectedObjects(const core::DocumentState&,const std::vector<int>&);
core::Image RenderPicturePixels(const core::Annotation&);
core::Annotation FilterPicture(const core::Annotation&,const core::ColorFilterOptions&,bool& changed);
std::shared_ptr<const core::SelectionMask> PictureAlphaMask(const core::Annotation&,int,int,int);
}
