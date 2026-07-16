#include "Resource.h"

using namespace Gdiplus;

Image* Resource::LoadImage(ImageID id) {
    const wchar_t* path = nullptr;
    switch (id) {
    case BG_CIRCUIT: path = L"custom\\bg_circuit.png"; break;
    case UI_FRAME:   path = L"custom\\ui_frame.png";   break;
    default: return nullptr;
    }
    // 文件不存在时返回 nullptr，游戏静默降级运行
    return Image::FromFile(path, FALSE);
}

void Resource::PlayBGM(const wchar_t*) {}
void Resource::StopBGM() {}
