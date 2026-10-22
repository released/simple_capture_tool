#pragma once
namespace sct::services {void ImageHomeChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void PasteOriginalChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void CaptureLatencyChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void AppMenuChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void ShellChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void FreehandChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void RibbonMenuChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void EditorHomeChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void CanvasResizeChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void ObjectEditingChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void ObjectEditingCoreChecks(const std::function<void(bool,const char*)>&,const std::filesystem::path&);}
namespace sct::services {void EditorUsabilityChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void RamCompositionChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {int RunAlignmentTests();void AlignmentCoreChecks(const std::function<void(bool,const char*)>&,const std::filesystem::path&);void AlignmentCaptureChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);void ManualSavePaintChecks(const std::function<void(bool,const char*)>&,const std::filesystem::path&);CFrameWnd* CreateCaptureTestSurface();}
