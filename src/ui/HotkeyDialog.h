#pragma once
#include "../services/ConfigService.h"
namespace sct::ui { bool EditHotkeys(CWnd* owner,services::Settings& settings,std::function<void(CWnd&)> audit={}); }
