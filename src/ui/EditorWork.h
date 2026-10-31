#pragma once
namespace sct::ui {
// Numeric values remain compatible with private native fixtures; not a file schema.
enum EditorWorkKind:int {
    WorkRender=0,WorkImageSave=1,WorkProjectSave=2,WorkTransform=3,WorkInsertFit=4,
    WorkClipboard=5,WorkOpenPage=7,WorkComposition=8,WorkOutput=9,WorkPreset=10,
    WorkPrintPreview=11,WorkBackground=12,WorkCanvas=13,WorkImagePaste=14,
    WorkNewDimensions=15,WorkScreenColor=16,WorkCrop=17,WorkLayout=18,
    WorkComparison=19,WorkPrecision=20,WorkAutoCrop=21,WorkReuse=22,
    WorkReusePages=23,WorkColorFilter=24,WorkDiscovery=25,WorkDiscoveryPages=27,
    WorkMask=28,WorkEnhancement=29,WorkProductivity=30,WorkAdvanced=31,
    WorkWorkflow=32,WorkAsset=33,WorkRefinement=34
};
}
