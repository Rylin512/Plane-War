# 程序设计项目实践：Plane War

经典纵版飞行射击游戏，基于 Win32 + GDI+ / Direct2D 渲染后端实现。



---

## 特性

- **难度**：普通敌机 → 快速敌机 → 射击敌机 → Boss 战，难度递增
- **渲染后端**：Direct2D（硬件加速）/ GDI+（兼容回退）
- **分辨率**：支持 4:3 / 16:9 / 16:10 等共 20 种分辨率
- **高 DPI 支持**：任意缩放比例下 UI 一致
- **可变游戏速度**：慢速 45 / 中速 60 / 快速 90 tick/s 可调
- **5 种主题**：Classic / Neon / Lava / Aurora / Mono
- **刷新率**：60 / 75 / 120 / 144 / 165 / 240 Hz，游戏逻辑与画面刷新率解耦
- **性能监视器**：实时 FPS + 内存占用显示
- **排行榜**：本地持久化，记录分数 / 关卡 / 日期
- **设置持久化**：分辨率、帧率、皮肤、渲染后端自动保存

## 操作说明

| 按键 | 功能 |
|------|------|
| `W` `A` `S` `D` / 方向键 | 移动 |
| `Shift` | 加速 |
| `Ctrl` | 减速 |
| `B` | 炸弹（清屏） |
| `P` | 暂停 |
| `H` | 帮助（游戏中） |
| `ESC` | 暂停 / 返回菜单 |

- **自动射击**：无需按键，自动连发
- **鼠标**：菜单中可用鼠标点击选项

## 编译

### MSVC（推荐）

```powershell
# 1. 进入 VS 开发者命令行 (vcvars64.bat)
# 2. 编译
cl.exe /nologo /utf-8 /EHsc /O2 /std:c++17 /Fe:PlaneWar_msvc.exe ^
  main.cpp Game.cpp D2DRenderer.cpp Renderer.cpp ^
  Player.cpp Enemy.cpp Boss.cpp Bullet.cpp Item.cpp ^
  gdiplus.lib d2d1.lib dwrite.lib winmm.lib user32.lib gdi32.lib psapi.lib ^
  /link /SUBSYSTEM:WINDOWS
```

### 依赖库

| 库 | 用途 |
|---|------|
| `gdiplus.lib` | GDI+ 渲染 |
| `d2d1.lib` | Direct2D 渲染 |
| `dwrite.lib` | DirectWrite 文字 |
| `winmm.lib` | 高精度时钟 / MCI 音频 |
| `user32.lib` | 窗口 / 输入 |
| `gdi32.lib` | GDI 兼容 DC |
| `psapi.lib` | 进程内存查询 |

## 项目结构

```
Plane-War-main-Direct2D-Edition/
├── main.cpp          # 入口：DPI 感知 + 初始化
├── Game.h / .cpp     # 游戏主控：窗口、循环、碰撞、渲染调度
├── Config.h          # 全局常量：分辨率表、调色板、游戏参数
├── Player.h / .cpp   # 玩家飞机：移动、渲染、状态
├── Enemy.h / .cpp    # 敌机（普通/快速/射击）+ Boss
├── Bullet.h / .cpp   # 子弹：玩家/敌机 + 对象池
├── Item.h / .cpp     # 道具：火力/生命/炸弹/护盾/射速
├── Boss.h / .cpp     # Boss 专属逻辑
├── GameObject.h      # 实体基类 + AABB 碰撞
├── Renderer.h        # 抽象渲染接口
├── Renderer.cpp      # GDI+ 渲染实现
├── D2DRenderer.h     # Direct2D 渲染器头文件
├── D2DRenderer.cpp   # Direct2D 渲染器实现
├── Resource.h        # 素材加载（图片 / 音频）
└── custom/           # 游戏素材
    ├── backgrounds/  # 背景图
    ├── music/        # 背景音乐
    └── window_themes/# UI 边框素材
```

## 游戏机制

| 系统 | 说明 |
|------|------|
| **火力等级** | 1~5 级，增加子弹数量和扩散角度 |
| **护盾** | 一次性格挡，被击中后立即消失，蓝色特效反馈 |
| **炸弹** | 清屏 + 屏幕震动 + 大量粒子 |
| **射速提升** | 限时大幅缩短射击间隔 |
| **无敌帧** | 受伤后 3 秒无敌，闪烁提示 |
| **变速** | Shift 加速（1.65x）/ Ctrl 减速（0.38x） |
| **Boss 战** | 每关分数达标后触发，3 阶段血量 + 弹幕模式 |

## 更新

- **渲染后端**：新增Direct2D渲染后端，支持硬件加速
- **DPI 修复**：D2D 渲染目标固定 96 DPI，消除高 DPI 下界面错位和文字不对齐
- **游戏速度解耦**：逻辑更新与渲染帧率分离，刷新率仅影响画面流畅度
- **可变 tick 速率**：支持 45 / 60 / 90 tick/s 三档游戏速度
- **子弹缩放修复**：构造函数补齐 `GAME_SCALE`，池复用子弹不再双重缩放
- **性能监视器**：右上角实时 FPS + 内存占用
- **暂停菜单**：新增返回主菜单按钮
- **动态标题副标题**：显示当前分辨率 / 帧率上限 / 渲染后端 / 皮肤
