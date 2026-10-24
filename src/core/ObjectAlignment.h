#pragma once
#include "ObjectComposition.h"
#include <optional>
namespace sct::core::alignment {
enum class Arrange { Left,Center,Right,Top,Middle,Bottom,Horizontal,Vertical };
using Units=std::vector<composition::Indices>;
Units SelectionUnits(const DocumentState&,const composition::Indices&);
DocumentState ArrangeObjects(const DocumentState&,const composition::Indices&,Arrange,bool canvas=false);
Annotation MoveObject(const Annotation&,int dx,int dy);
bool PositionsChanged(const DocumentState&,const DocumentState&,const composition::Indices&);
struct Targets {std::vector<LONG> x,y;};
struct SnapResult {RECT rect{};std::optional<LONG> x,y;};
Targets BuildTargets(const DocumentState&,const composition::Indices&);
SnapResult SnapMove(RECT,const Targets&,double tolerance);
}
