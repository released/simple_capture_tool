#pragma once
#include "Document.h"
namespace sct::core::composition {
using Indices=std::vector<int>;
void Validate(const DocumentState&);
Indices Expand(const DocumentState&,const Indices&);
RECT Bounds(const DocumentState&,const Indices&);
bool Locked(const DocumentState&,const Indices&);
void RequireEditable(const DocumentState&,const Indices&);
DocumentState Group(const DocumentState&,const Indices&,bool ungroup=false);
DocumentState Lock(const DocumentState&,const Indices&,bool);
DocumentState Transform(const DocumentState&,const Indices&,RECT destination,bool scale);
DocumentState Recolor(const DocumentState&,const Indices&,COLORREF);
DocumentState Remove(const DocumentState&,const Indices&);
DocumentState Duplicate(const DocumentState&,const Indices&,Indices& selected);
DocumentState Reorder(const DocumentState&,const Indices&,int direction,Indices& selected);
void Append(DocumentState& destination,const std::vector<Annotation>& objects,int dx=12,int dy=12);
}
