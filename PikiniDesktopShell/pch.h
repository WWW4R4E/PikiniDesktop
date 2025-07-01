// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include "framework.h"

#include <winrt/base.h> 

// 包含 WRL 的核心头文件
#include <wrl.h>
#include <wrl/module.h>
#include <wrl/wrappers/corewrappers.h>

// 包含我们自己的类定义，这样在 dllmain.cpp 中就能识别它
#include "PikiniExplorerCommand.h" 
#endif //PCH_H
//DEFINE_GUID(CLSID_PikiniExplorerCommand,
//    0xc19ffafd, 0x4ac9, 0x42a0, 0xa1, 0xdb, 0x35, 0xcb, 0x22, 0xc0, 0x81, 0x61);