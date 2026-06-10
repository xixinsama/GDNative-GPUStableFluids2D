# Problems & Bugs — GPUStableFluids2D

> 最后更新: 2026-06-10

---

## 🐛 Bug: 静置流体逐渐产生毛边 (Static Fluid Fringing)

### 现象
流体注入后静置不动，随着时间推移，原本光滑的颜色/密度边界逐渐变得模糊并出现锯齿状"毛边"（grid-aliasing artifacts），类似低分辨率图像放大后的阶梯感。

### 根因分析

经过对全部着色器和管线代码的追踪，确认这是一个**多重因素叠加**的问题：

#### 原因1 (主要): 半拉格朗日平流的数值扩散

`advect.glsl` 每帧执行回追踪采样：

```glsl
vec2 prev_uv = uv - vel * params.dt / params.resolution;
prev_uv = clamp(prev_uv, 0.0, 1.0);
vec4 result = texture(input_field, prev_uv);  // 双线性插值
```

`texture()` 使用双线性插值 → 每次平流都是一次**低通滤波**。在 60fps 下，每秒执行 60 次低通滤波：
- 尖锐的颜色边界 → 逐渐模糊
- 模糊的边界 → 进一步模糊
- 最终在网格分辨率尺度上显现"阶梯"伪影

`clamp()` 在域边界处将采样坐标钳制到 [0,1]，导致边界像素始终从边缘采样，产生"粘性"边缘效应。

#### 原因2 (次要): 压力求解不充分

`_step_solve_pressure()` 使用固定 40 次 Jacobi 迭代。对于 512×512 网格，40 次迭代可能不足以将散度收敛到机器精度。残余散度 → 微小速度 → 累积平流 → 颜色涂抹。

标准 Stable Fluids 的 Jacobi 收敛速度为 O(N²)，512×512 网格理论上需要 ~2500 次迭代才能完全收敛。40 次是性能折衷，但会留下残余速度。

#### 原因3 (关键): ink_longevity 和 color_decay 是死属性

**这两个属性在 C++ 中定义、在 SimParams 中声明、在编辑器中可设置——但从未传递给任何着色器。**

搜索全部 12 个 `.glsl` 文件：`ink_longevity`、`color_decay`、`dissipat` → **零匹配**。

意味着：
- `ink_longevity` (默认 0.995): 颜色本应每帧乘以 0.995 自然衰减 → **未生效**
- `color_decay` (默认 0.0005): 颜色本应每帧减去固定量 → **未生效**
- 注入的颜色/密度会**永久存在**，数值噪声无限累积

#### 原因4 (结构): SimParams uniform buffer 未被使用

`sim_params.h` 定义了 64 字节的 SimParams 结构体，包含了 `color_decay`、`velocity_decay`、`ink_longevity` 等字段。但 `GPUResourceManager` 从未创建 `sim_params_buffer`，所有着色器参数都通过 16 字节 push constants 传递——而 push constants 中没有 color_decay 的位置。

### 修复方案

| 优先级 | 方案 | 效果 |
|--------|------|------|
| **P0** | 在 `advect_color` 步骤后新增 color_decay 着色器 pass，或修改 advect.glsl 的 push constant 包含 longevity 参数 | 颜色自然衰减，噪声不累积 |
| **P1** | 在 advect.glsl 中增加速度阈值：`if (length(vel) < epsilon) { output = input; return; }` | 静止区域不执行平流，避免不必要的插值 |
| **P2** | 增加 Jacobi 迭代次数默认值（40→80）或实现自适应收敛检查 | 减少残余散度 |
| **P3** | 将 SimParams uniform buffer 实际接入管线，让所有参数通过 uniform buffer 传递 | 统一参数管理 |

### 临时缓解方法（用户可操作）

- 在 Godot 编辑器中增大 `poisson_iterations`（40 → 60-80）
- 增大 `velocity_decay`（-1.0 → 0.001）让速度衰减
- 使用 `FluidEmitter2D` 持续注入新颜色以掩盖累积伪影

---

## 🐛 Bug: FluidDisplay2D 的 display_mode 不生效

**状态**: ✅ 已修复 (2026-06-10)

**修复内容**: 新增 `tex_display_velocity`、`tex_display_pressure`、`tex_display_divergence` 三个 display 纹理；在每帧末尾将内部场纹理复制到对应 display 纹理；FluidDisplay2D 根据 display_mode 选择正确的纹理。

---

## ⚠️ 未完成功能

### 障碍物系统 (部分实现)

**当前状态**: 
- ✅ `obstacle_force.glsl` 已重写为刚性屏障逻辑（零速度 + 无滑移边界条件）
- ✅ FluidObstacle2D 新增 `shape`、`one_way_collision`、`debug_color`、`sim_target` 属性
- ❌ 障碍物光栅化仍使用硬编码 32×32 矩形，未使用 Shape2D
- ❌ `advect.glsl` 和 `jacobi.glsl` 未采样障碍物纹理，无法在障碍物内部跳过计算
- ❌ `FluidObstacleDrawer` 不支持 CollisionShape2D 的 CircleShape2D / RectangleShape2D / CapsuleShape2D 形状

**需要的工作**:
1. `FluidObstacleDrawer` 从 Shape2D 读取形状并光栅化（而非硬编码 32×32 矩形）
2. `advect.glsl` 新增障碍物纹理绑定，回追踪时检查并跳过障碍物像素
3. `jacobi.glsl` 新增障碍物纹理绑定，扩散/压力求解时跳过障碍物像素

### TileMap 障碍物 (未实现)

**当前状态**: ❌ `FluidTileMapObstacle2D::_rasterize_tile_map()` 为空函数体

**需要的工作**:
1. 从 TileMapLayer 迭代 tile cell
2. 从 TileSet 获取 tile 形状（碰撞/纹理）
3. 将 tile 多边形光栅化到障碍物缓冲区

### 光照系统 (仅占位)

**当前状态**: ⚠️ `FluidLightOccluder2D` 返回硬编码矩形而非真正的 Marching Square 密度轮廓

**需要的工作**:
1. 集成 `marching_square_extract.glsl` (文件存在但未被加载为 pipeline)
2. GPU compute → Storage Buffer 提取 Marching Square 格点
3. CPU 端连通轮廓追踪（Job 系统并行化）
4. Douglas-Peucker 简化（已实现）
5. LightOccluder2D 对象池管理

### 编辑器调试可视化 (未实现)

**当前状态**: ❌ Godot 4 GDExtension 中 `_notification(NOTIFICATION_DRAW)` 和 `_draw()` 不是虚函数，无法直接覆盖。需要单独的 EditorPlugin 模块来注册 gizmo 插件。

**需要的工作**:
1. 创建 `src/editor/` 目录下的 EditorPlugin 子类
2. 为每种流体节点类型注册 EditorNode2DGizmoPlugin
3. 在编辑器中绘制流体域边界、发射器形状、障碍物轮廓、力场范围等

### GPU 并行力发射器 (未实现)

**当前状态**: ❌ `force_emitter_buffer` 已分配但未被任何着色器使用。`apply_force_emitter.glsl` 文件不存在。

**需要的工作**:
1. 编写 `apply_force_emitter.glsl` 着色器
2. 在 FluidRenderPipeline 中新增 dispatch step
3. 从 FluidForceEmitter2D 更新 force_emitter_buffer

### FluidVorticityVisualizer2D 涡度场可视化 (部分实现)

**当前状态**: ✅ 已从存根升级为功能 Sprite2D，可显示 Velocity/Pressure/Divergence 场
⚠️ Vorticity 模式回退到 Density（因为涡度是 curl of velocity，没有单独的标量场纹理）

**需要的工作**:
1. 编写 `compute_vorticity_field.glsl` 着色器计算 curl(velocity) 标量场
2. 新增 `tex_vorticity_display` 显示纹理

### sim_params uniform buffer 未接入

**当前状态**: ❌ `SimParams` 结构体定义在 `sim_params.h`（64 字节），`GPUResourceManager` 未创建对应 buffer，所有着色器参数通过 push constants 传递。

**需要的工作**:
1. 创建 `sim_params_buffer` 并每帧更新
2. 修改所有着色器接收 uniform buffer（set=0, binding=8）
3. 移除冗余的 per-step push constants（分辨率等静态参数只需传一次）

---

## 🔧 全局待改进项

### NodePath → 直接类型引用

**状态**: ✅ 已修复 (2026-06-10)

全部 8 个子节点的引用已统一为 `sim_target`，使用 `PROPERTY_HINT_NODE_TYPE` 限制为 GPUStableFluids2D。

### GPUStableFluids2D 配置警告

**状态**: ✅ 已修复 (2026-06-10)

`_get_configuration_warnings()` 已实现，当没有 FluidDisplay2D 子节点时显示警告。
