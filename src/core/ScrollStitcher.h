#pragma once
#include "Image.h"
namespace sct::core {
// Returns the number of novel rows; zero means duplicate; -1 means no reliable overlap.
int FindScrollOffset(const Image& previous, const Image& next,bool horizontal=false);
bool AppendScroll(Image& destination, const Image& next, int novelRows,bool horizontal=false);
}
