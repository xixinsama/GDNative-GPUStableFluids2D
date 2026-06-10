# GPUStableFluids2D — 架构设计文档

> **版本**: 3.0 (2026-06-10)
>
> 基于 FluidPlayGround (Unity C#)、godot-fluid-simulation (Godot GDScript)、GodotFluidSim (Godot C#) 的综合设计。
> Bug 列表和未完成工作请见 [`problems.md`](problems.md)。

---

## 一、整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        GPUStableFluids2D Plugin                  │
├─────────────────────────────────────────────────────────────────┤
│  C++ GDExtension (src/)        │  GLSL Compute Shaders           │
│  ┌───────────────────────────┐ │  ┌──────────────────────────┐  │
│  │ Core Simulation           │ │  │ advect.glsl              │  │
│  │  • GPUStableFluids2D      │ │  │ jacobi.glsl              │  │
│  │  • FluidDomain2D (Config) │ │  │ divergence.glsl          │  │
│  ├───────────────────────────┤ │  │ subtract.glsl            │  │
│  │ Emitters                  │ │  │ boundary.glsl            │  │
│  │  • FluidEmitter2D         │ │  │ vorticity.glsl           │  │
│  │  • FluidForceEmitter2D    │ │  │ obstacle_force.glsl      │  │
│  ├───────────────────────────┤ │  │ splat_batch.glsl         │  │
│  │ Obstacles                 │ │  │ shift_texture.glsl       │  │
│  │  • FluidObstacle2D        │ │  │ copy_texture.glsl        │  │
│  │  • FluidTileMapObstacle2D │ │  └──────────────────────────┘  │
│  ├───────────────────────────┤ │                                  │
│  │ Display / Interaction     │ │  Godot Integration               │
│  │  • FluidDisplay2D         │ │  • Texture2DRD → Sprite2D      │
│  │  • FluidMouseInteractor2D │ │  • LightOccluder2D bridge       │
│  │  • FluidVorticityVis2D    │ │  • Camera2D domain follow       │
│  ├───────────────────────────┤ │  • Group auto-discovery         │
│  │ Lighting Bridge           │ │                                  │
│  │  • FluidLightOccluder2D   │ │                                  │
│  └───────────────────────────┘ │                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 二、节点系统

### 2.1 核心节点: GPUStableFluids2D (Node2D)

流体模拟主控制器。管理 GPU 资源、调度计算管线、处理域跟随。

**设计模式**: 类似于 `PhysicsBody2D`——是"物理世界"的容器。需要 `FluidDisplay2D` 子节点来显示流体（正如 PhysicsBody2D 需要 CollisionShape2D）。缺少子节点时通过 `_get_configuration_warnings()` 显示编辑器警告。

**导出属性**:

| 属性 | 类型 | 默认值 | 范围 | 说明 |
|------|------|--------|------|------|
| `width` | int | 512 | 32–2048 | 网格宽度 |
| `height` | int | 512 | 32–2048 | 网格高度 |
| `timestep` | float | 1.0 | 0.1–10.0 | 时间步长 Δt |
| `grid_scale` | float | 1.0 | 0.1–100.0 | 网格缩放系数 (Δx) |
| `viscosity` | float | 0.0 | 0–0.01 | 粘稠度 |
| `diffusion` | float | 0.0 | 0–0.01 | 扩散系数 |
| `poisson_iterations` | int | 40 | 1–200 | Jacobi 压力迭代次数 |
| `ink_longevity` | float | 0.995 | 0.9–1.0 | 墨水/染料每帧乘性衰减 |
| `ink_color` | Color | (1,1,1,1) | — | 默认注入颜色 |
| `clear_color` | Color | (0,0,0,0) | — | 清除颜色 |
| `color_decay` | float | 0.0005 | 0–1.0 | 颜色每帧加性衰减 |
| `velocity_decay` | float | -1.0 | -1.0–1.0 | 速度衰减 (-1=不衰减) |
| `vorticity_enabled` | bool | true | — | 涡度增强开关 |
| `vorticity_scale` | float | 0.4 | 0–5.0 | 涡度强度 |
| `density_scale` | float | 1.0 | 0.1–10.0 | 密度缩放 |
| `obstacle_force_strength` | float | 5.0 | 0–100.0 | 障碍物强度 |
| `subtractive_mixing` | bool | false | — | 减色混合 (CMY) |
| `follow_mode` | enum | Camera2D | — | Camera2D / Node2D / None |
| `follow_path` | NodePath | — | — | 跟随目标路径 |
| `fluid_world_size` | Vector2 | (1920,1080) | — | 世界空间尺寸 |
| `domain_wrap_mode` | enum | Toroidal | — | Toroidal / Clamp |

**信号**: `simulation_initialized`, `simulation_reset`, `obstacle_changed`

**公共方法**: `add_impulse()`, `queue_draw_request()`, `clear_draw_requests()`, `reset()`, `get_output_texture()`, `get_velocity_texture()`, `get_pressure_texture()`, `get_divergence_texture()`, `world_to_fluid_pos()`, `sample_velocity()`, `get_domain_offset()`

**内部 GPU 资源**:

| 纹理 | 通道 | 说明 |
|------|------|------|
| `tex_velocity` | RG = (vx, vy) | 速度场 |
| `tex_pressure` | R = 压力 | 压力标量场 |
| `tex_color` | RGBA | 染料/颜色场 (SIM 用, STORAGE_BIT) |
| `tex_divergence` | R = ∇·v | 散度 |
| `tex_temp` | 全通道 | 乒乓缓冲 |
| `tex_display` | RGBA | 显示副本 (无 STORAGE_BIT → READ_ONLY) |
| `tex_display_velocity` | RGBA | 速度场显示副本 |
| `tex_display_pressure` | RGBA | 压力场显示副本 |
| `tex_display_divergence` | RGBA | 散度场显示副本 |
| `tex_obstacle` | RG=速度, A=掩码 | 当前帧障碍物 |
| `tex_obstacle_pre` | 同上 | 前一帧障碍物 |

格式: `R32G32B32A32_SFLOAT`, 512×512
SIM 纹理: `STORAGE | SAMPLING | CAN_COPY_TO | CAN_COPY_FROM | CAN_UPDATE`
显示纹理: `SAMPLING | CAN_COPY_TO` (无 STORAGE_BIT)

**Storage Buffers**:
- `batch_buffer`: 4096 点 × 48 字节 (BatchPoint) = 192 KB
- `force_emitter_buffer`: 32 发射器 × 48 字节 = 1.5 KB

---

### 2.2 发射器节点

#### FluidEmitter2D (Node2D)

向流体注入颜色/密度粒子。支持 Continuous / Burst / Periodic 模式，Point / Circle / Rect / LineSegment / TextureMask 形状，7 种预设效果 (Explosion, Mist, Smoke, Fountain, Steam, Fire)，Directional / Radial / Random 速度模式。

**属性**: `active`, `emission_mode`, `emission_shape`, `emission_preset`, `emit_color`, `emit_velocity`, `color_radius`, `velocity_radius`, `emit_interval`, `burst_count`, `shape_size`, `velocity_pattern`, `swirl_strength`, `lifetime`, `auto_destroy`, `sim_target`

#### FluidForceEmitter2D (Node2D)

向流体施加方向性力场。支持 Directional / PointRadial / Swirl / Centripetal / Centrifugal 模式。

**属性**: `active`, `force_pattern`, `force`, `force_radius`, `emission_shape`, `shape_size`, `falloff_exponent`, `swirl_strength`, `force_preset`, `lifetime`, `auto_destroy`, `sim_target`

---

### 2.3 障碍物节点

#### FluidObstacle2D (Node2D + IFluidObstacle)

流体刚性屏障。类似于 CollisionShape2D 之于 PhysicsBody2D。可附着于 RigidBody2D / CharacterBody2D 下自动检测速度。

**属性**: `auto_detect_velocity`, `velocity`, `angular_velocity`, `obstacle_texture`, `shape` (Shape2D), `one_way_collision`, `debug_color`, `sim_target`

**IFluidObstacle 接口**: `get_object_linear_velocity()`, `get_object_angular_velocity()`, `get_object_center()`

障碍物通过 CPU 光栅化写入 `tex_obstacle`：
- RG 通道：障碍物速度（线速度 + 角速度 × 半径）
- A 通道：障碍物掩码（1.0 = 固体）

`obstacle_force.glsl` 在着色器中执行：
- 障碍物内部 → 速度清零（刚性屏障）
- 边界单元 → 无滑移条件（匹配障碍物表面速度）
- 障碍物离开 → 释放速度推动流体

#### FluidTileMapObstacle2D (Node2D)

从 TileMapLayer 提取障碍物形状。

**属性**: `tile_map_path`, `obstacle_mode`, `fill_mode`, `physics_layer_index`, `terrain_set`, `sim_target`

---

### 2.4 显示与交互节点

#### FluidDisplay2D (Sprite2D)

将流体密度/内部场纹理渲染到屏幕。作为 GPUStableFluids2D 的必要子节点。

**属性**: `sim_target`, `display_mode` (Density/Velocity/Pressure/Divergence/Vorticity), `auto_size`

**运行时**: `_process()` 根据 `display_mode` 从 GPUStableFluids2D 获取对应纹理并更新 Sprite2D 的 Texture2DRD。

#### FluidMouseInteractor2D (Node2D)

将鼠标/触控输入转换为流体绘制请求。

**属性**: `sim_target`, `mouse_button`, `draw_color`, `brush_size`, `velocity_scale`, `continuous_draw`, `interaction_mode` (Draw/Push/Pull/Swirl)

#### FluidVorticityVisualizer2D (Sprite2D)

调试可视化工具。与 FluidDisplay2D 类似，但专门用于显示内部模拟场（速度、压力、散度等）。

**属性**: `sim_target`, `display_mode`, `auto_update`

---

### 2.5 光照桥接

#### FluidLightOccluder2D (Node2D)

将流体密度纹理桥接到 Godot 的 2D 光照系统。通过 Marching Square（已规划）从密度场提取轮廓，动态创建 `LightOccluder2D` 子节点。

**属性**: `sim_target`, `density_threshold`, `max_contours`, `update_frequency`, `simplify_tolerance`, `occluder_light_mask`, `downscale_factor`

**信号**: `contours_updated(int count)`

---

### 2.6 配置资源

#### FluidDomain2D (RefCounted)

可保存的流体模拟域配置。可在多个 GPUStableFluids2D 节点间共享。

**方法**: `apply_to(sim: GPUStableFluids2D)` 将配置批量应用到模拟节点。

---

## 三、类继承关系

```
godot::Node2D
├── GPUStableFluids2D              — 核心模拟节点
├── FluidEmitter2D                 — 颜色/粒子发射器
├── FluidForceEmitter2D            — 力场发射器
├── FluidObstacle2D                — 障碍物 (实现 IFluidObstacle)
├── FluidTileMapObstacle2D         — TileMap 障碍物
├── FluidLightOccluder2D           — 光照桥接
├── FluidMouseInteractor2D         — 鼠标/触控交互
└── FluidVorticityVisualizer2D (→ see below)

godot::Sprite2D
├── FluidDisplay2D                 — 模拟结果显示
└── FluidVorticityVisualizer2D     — 调试场可视化

IFluidObstacle (虚基类)            — 障碍物速度查询接口

godot::RefCounted
└── FluidDomain2D                  — 流体域配置资源
```

---

## 四、GPU 计算管线 (11 Steps)

所有步骤在渲染线程上顺序执行 (`RenderingServer::call_on_render_thread`)。

```
每帧流程:
─────────────────────────────────────────────────────────────────
 0. DomainShift()              ← shift_texture.glsl
     条件: 摄像机/跟随节点发生移动

 1. SplatBatch()               ← splat_batch.glsl
     条件: 有待处理的绘制请求 (_draw_request_count > 0)

 2. ObstacleForce()            ← obstacle_force.glsl
     零化障碍物内部速度 + 无滑移边界条件 + 障碍物离开推力

 3. AdvectVelocity()           ← advect.glsl
     半拉格朗日回追踪平流

 4. DiffuseVelocity()          ← jacobi.glsl × 10
     条件: viscosity > 0.000001

 5. Vorticity()                ← vorticity.glsl
     条件: vorticity_enabled = true

 6. Divergence()               ← divergence.glsl
     ∇·v 计算

 7. SolvePressure()            ← jacobi.glsl × poisson_iterations
     Poisson 方程求解 (默认 40 次 Jacobi 迭代)

 8. SubtractGradient()         ← subtract.glsl
     压力梯度减法 (投影步骤)

 9. Boundary()                 ← boundary.glsl × 2
     速度边界 (scale=-1) + 压力边界 (scale=+1)

10. AdvectColor()              ← advect.glsl
     颜色/密度场平流

11. CopyObstacle()             ← copy_texture.glsl
     tex_obstacle → tex_obstacle_pre
─────────────────────────────────────────────────────────────────
Post: texture_copy(tex_color    → tex_display)
      texture_copy(tex_velocity → tex_display_velocity)
      texture_copy(tex_pressure → tex_display_pressure)
      texture_copy(tex_divergence → tex_display_divergence)
```

### 着色器文件 (demo/shaders/)

| 着色器 | 绑定 | 说明 |
|--------|------|------|
| `advect.glsl` | S0=field(S), S1=vel(S), S2=out(I) | 半拉格朗日平流 |
| `jacobi.glsl` | S0=x(I), S1=b(I), S2=x'(I) | Jacobi 迭代求解 |
| `divergence.glsl` | S0=vel(I), S1=div(I) | 散度 ∇·v |
| `subtract.glsl` | S0=p(I), S1=vel(I), S2=out(I) | 压力梯度减法 |
| `boundary.glsl` | S0=in(I), S1=out(I) | Neumann 边界条件 |
| `vorticity.glsl` | S0=vel(I), S1=out(I) | 涡度增强力 |
| `obstacle_force.glsl` | S0=vel_in(I), S1=vel_out(I), S2=obs(S), S3=obs_pre(S) | 障碍物边界处理 |
| `splat_batch.glsl` | S0=pts(B), S1=vel(I), S2=color(I) | 批量 splat |
| `shift_texture.glsl` | S0=tex(S), S1=out(I) | 域偏移纹理卷动 |
| `copy_texture.glsl` | S0=src(I), S1=dst(I) | 纹理复制 |

(S=Sampler, I=Image, B=StorageBuffer)

### Push Constant 布局 (std430, 16 字节)

```cpp
AdvectionPushConstant    { vec2 resolution; float dt; float rdx; }
JacobiPushConstant       { vec2 resolution; float alpha; float rbeta; }
DivergencePushConstant   { vec2 resolution; float half_rdx; }
SubtractPushConstant     { vec2 resolution; float half_rdx; }
BoundaryPushConstant     { vec2 resolution; float scale; }
VorticityPushConstant    { vec2 resolution; float dt; float vorticity_scale; }
ShiftTexturePushConstant { vec2 resolution; float offset[2]; }
SplatBatchPushConstant   { vec2 resolution; int point_count; float dt; }
```

### Uniform Set 缓存

缓存键: `(shader_id, texture_id, set_index, uniform_type)` → RID。
避免每帧创建/销毁大量 uniform set（Jacobi 压力迭代 40 次/帧 × 3 个 set = 120 个/帧会耗尽 Vulkan 描述符池）。

---

## 五、源文件组织

```
src/
├── register_types.h / .cpp                  — 模块注册入口 (10 个类)
├── GPU_stable_fluids_2D_init.h / .cpp       — GPUStableFluids2D 主节点
├── core/
│   ├── gpu_resource_manager.h / .cpp        — GPU 纹理/管线生命周期
│   ├── fluid_render_pipeline.h / .cpp       — 11 步管线调度
│   ├── fluid_domain.h / .cpp                — FluidDomain2D 配置资源
│   ├── fluid_types.h                        — 公共枚举定义
│   ├── sim_params.h                         — SimParams 结构体
│   └── fluid_debug_config.h                 — 调试日志开关
├── emitters/
│   ├── fluid_emitter_2d.h / .cpp            — FluidEmitter2D
│   └── fluid_force_emitter_2d.h / .cpp      — FluidForceEmitter2D
├── obstacles/
│   ├── fluid_obstacle_2d.h / .cpp           — FluidObstacle2D + IFluidObstacle
│   ├── fluid_obstacle_drawer.h / .cpp       — CPU 障碍物光栅化
│   ├── fluid_tile_map_obstacle_2d.h / .cpp  — FluidTileMapObstacle2D
│   └── polygon_rasterizer.h / .cpp          — 软件多边形光栅化
├── lighting/
│   └── fluid_light_occluder_2d.h / .cpp     — FluidLightOccluder2D
├── display/
│   ├── fluid_display_2d.h / .cpp            — FluidDisplay2D (Sprite2D)
│   ├── fluid_mouse_interactor_2d.h / .cpp   — FluidMouseInteractor2D
│   └── fluid_vorticity_visualizer_2d.h/.cpp — FluidVorticityVisualizer2D (Sprite2D)
└── utils/
    ├── fluid_coord_utils.h / .cpp           — 坐标转换
    ├── draw_request.h                       — DrawRequest + BatchPoint + ForceEmitterData
    └── polygon_rasterizer.h / .cpp          — 多边形填充
```

---

## 六、关键设计决策

### 6.1 渲染线程安全

所有 RenderingDevice 命令必须在渲染线程上执行。主线程通过 `RenderingServer::call_on_render_thread()` 调度 GPU 工作：

```cpp
RenderingServer::get_singleton()->call_on_render_thread(
    callable_mp(this, &GPUStableFluids2D::_gpu_process_on_render_thread).bind(dt, offset)
);
```

### 6.2 乒乓缓冲

读写同一纹理的 pass 使用乒乓缓冲避免 read-after-write hazard：

```cpp
_dispatch(..., tex_velocity, IMG, tex_temp, IMG);
std::swap(tex_velocity, tex_temp);
```

### 6.3 显示纹理分离

模拟纹理有 `STORAGE_BIT`（compute shader 可写）→ GENERAL 布局。
显示纹理无 `STORAGE_BIT` → `SHADER_READ_ONLY_OPTIMAL` 布局，Forward+ 渲染器可安全采样。

### 6.4 批量绘制

绘制请求存储在 4096 条目的环形缓冲区中，每帧打包为 `BatchPoint` 数组并通过 `buffer_update()` 上传 GPU。`splat_batch.glsl` 单次 dispatch 处理全部点。

```cpp
struct BatchPoint {
    float pos[2];         // 8B  — UV 空间 [0,1]
    float vel[2];         // 8B  — 注入速度
    float color[4];       // 16B — RGBA
    float color_radius;   // 4B  — 颜色扩散半径
    float vel_radius;     // 4B  — 速度扩散半径
    float _pad[2];        // 8B  — std430 对齐至 48B
};
```

### 6.5 无限域与摄像机跟随

不需要自定义摄像机节点。使用 Godot 的 `Camera2D` + `follow_mode` 属性：

1. 每帧计算摄像机/跟随节点的位置变化
2. 归一化为 UV 空间偏移量 `domain_offset`
3. `shift_texture.glsl` 按 `-domain_offset` 卷动所有流体纹理
4. 摄像机静止时跳过 shift dispatch

域包裹模式 `Toroidal` → 从一侧流出的流体从对侧流回，配合 shift_texture 的周期边界实现无限流体幻觉。

### 6.6 障碍物光栅化

`FluidObstacleDrawer` 扫描 `"fluid_obstacles"` 组，将障碍物光栅化到 `tex_obstacle`（16 字节/像素: vx, vy, 0, alpha）。通过哈希脏检测跳过静态帧光栅化。

### 6.7 域偏移

```cpp
// 主线程: 计算域偏移
if (follow_mode == FollowMode::Camera2D) {
    Vector2 current = cam->get_global_position();
    domain_offset = (current - previous_pos) / fluid_world_size;
    previous_pos = current;
}

// 渲染线程: shift_texture 卷动纹理
// color_tex[texel] = color_tex[(texel + offset) % resolution]
```

### 6.8 流体光照设计

双路径方案：
- **路径 A**: 密度纹理 → Marching Square GPU → 轮廓提取 → CPU 简化 → 动态 LightOccluder2D → Godot 标准 2D 光照
- **路径 B**: `render_density_lit.glsl` 密度梯度着色器光照（增强路径）

不需要自定义 `FluidLightSource`——用户使用 Godot 标准的 `PointLight2D` / `DirectionalLight2D`。

---

## 七、Godot 集成矩阵

| 集成点 | 实现方式 |
|--------|---------|
| 场景树 | 所有节点继承 Node2D / Sprite2D / RefCounted |
| 信号系统 | `ADD_SIGNAL` + `emit_signal` |
| 纹理系统 | `Texture2DRD` → Sprite2D / Material |
| 物理引擎 | 自动读取 RigidBody2D / CharacterBody2D 速度 |
| Input 系统 | Godot `Input` 单例 |
| TileMap | 规划通过 TileMapLayer cells 遍历 |
| CollisionShape2D | 规划通过 Shape2D 光栅化 |
| Group 系统 | `"fluid_sim_nodes"`, `"fluid_obstacles"` 组自动发现 |
| 2D 光照系统 | LightOccluder2D 动态生成 → Godot 标准光照管道 |
| Camera2D | 通过 `follow_mode` + `follow_path` 跟随 |
| 编辑器 | PROPERTY_HINT_RANGE / ENUM / NODE_TYPE / RESOURCE_TYPE |

---

## 八、性能优化

| 优化 | 方案 |
|------|------|
| Uniform Set 缓存 | 复用 (shader, texture, set, type) → RID 组合 |
| 环形缓冲 | 预分配 4096 条目 storage buffer，零动态分配 |
| 哈希脏检测 | 障碍物位/旋/缩哈希 → 跳过静态帧光栅化 |
| 域偏移跳过 | 摄像机静止时跳过 shift_texture dispatch |
| 乒乓缓冲 | 避免 read-after-write hazard |
| 显示纹理分离 | STORAGE_BIT / NO_STORAGE_BIT 分离，避免布局转换 |
