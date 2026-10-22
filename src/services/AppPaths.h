#pragma once
#include <filesystem>
namespace sct::services::paths {
std::filesystem::path PortableRootFor(const std::filesystem::path& executableDirectory);
std::filesystem::path Root();
std::filesystem::path Runtime();
std::filesystem::path TestEvidence();
std::filesystem::path Counter();
std::filesystem::path Configuration();
CString DefaultCaptures();
CString UpgradeCaptureDirectory(const CString& value);
void UseTestData();
}
