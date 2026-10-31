#pragma once
namespace sct::services {void ReviewOptimizationChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void CaptureMagnifierChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void SelectionToolbarChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void AdvancedEditingChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void ProductivityChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void CommandToolsChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void EditableLayoutChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);void SidebarChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void WorkspaceToolsChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void EditorPerformanceChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void ObjectAlignmentChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void DefaultCursorChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void CropWorkflowChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);void EditorMenusChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void CompositionChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void ToolStyleChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
namespace sct::services {void ColorPickerChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
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

namespace sct::services {void DocumentationToolsChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}

namespace sct::services {void AssetSelectionChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}

namespace sct::services {void EditorRefinementsChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}

namespace sct::services {void RibbonClarityChecks(const std::function<void(bool,const char*)>&,std::ofstream&,const std::filesystem::path&);}
