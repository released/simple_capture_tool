#pragma once
namespace sct::services {
int RunSingleInstanceChild();
void SingleInstanceChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);
}
