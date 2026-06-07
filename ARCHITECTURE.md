# GPUStableFluids2D — 完整架构设计文档

> 基于对 FluidPlayGround (Unity C#)、godot-fluid-simulation (Godot GDScript)、GodotFluidSim (Godot C#) 三个参考项目的深度代码分析，以及 Godot 4 GDExtension / RenderingDevice 最佳实践的综合设计。

---

## 一、整体架构概览

```
┌─────────────────────────────────────────────────────────────────┐
│                        GPUStableFluids2D Plugin                  │
├─────────────────────────────────────────────────────────────────┤
│  C++ GDExtension (src/)        │  GLSL Compute Shaders (shaders/)│
│  ┌───────────────────────────┐ │  ┌──────────────────────────┐  │
│  │ Core Simulation Nodes     │ │  │ advect.glsl              │  │
│  │  • GPUStableFluids2D      │ │  │ jacobi.glsl              │  │
│  │  • FluidDomain2D          │ │  │ divergence.glsl          │  │
│  ├───────────────────────────┤ │  │ subtract.glsl            │  │
│  │ Emitter Nodes             │ │  │ boundary.glsl            │  │
│  │  • FluidEmitter2D         │ │  │ vorticity.glsl           │  │
│  │  • FluidForceEmitter2D    │ │  │ obstacle_force.glsl      │  │
│  ├───────────────────────────┤ │  │ splat.glsl               │  │
│  │ Obstacle Nodes            │ │  │ splat_batch.glsl         │  │
│  │  • FluidObstacle2D        │ │  │ shift_texture.glsl       │  │
│  │  • FluidTileMapObstacle2D │ │  │ copy_texture.glsl        │  │
│  ├───────────────────────────┤ │  │ apply_force_emitter.glsl │  │
│  │ Utility / Display Nodes   │ │  │ apply_colors.glsl        │  │
│  │  • FluidDisplay2D         │ │  │ apply_forces.glsl        │  │
│  │  • FluidMouseInteractor2D │ │  │ marching_square_extract. │  │
│  ├───────────────────────────┤ │  │   glsl                   │  │
│  │ Lighting Bridge Nodes     │ │  │ render_density_lit.glsl  │  │
│  │  • FluidLightOccluder2D   │ │  └──────────────────────────┘  │
│  └───────────────────────────┘ │                                  │
│                                 │  Godot Integration              │
│  Data Resources                │  • Texture2DRD output           │
│  • FluidTextureResource        │  • LightOccluder2D bridge       │
│  • SimParams (uniform buffer)  │  • Camera2D follow system       │
│  • ImpulseBuffer (uniform)     │  • Godot's built-in 2D lighting  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 二、节点系统设计

### 2.1 核心模拟节点

#### `GPUStableFluids2D` (继承 `Node2D`) — 流体模拟主节点

**职责**: 管理完整的 GPU 流体模拟管线，是整个系统的核心控制器。基于 Navier-Stokes 方程的 Stable Fluids (Stam 1999) 算法，使用 GPU Compute Shader 实现实时流体效果。

**导出属性**:

| 属性名 | 类型 | 默认值 | 范围 | 描述 |
|--------|------|--------|------|------|
| `resolution` | Vector2i | (512, 512) | 32–2048 | 模拟网格分辨率 |
| `timestep` | float | 1.0 | 0.1–10.0 | 模拟时间步长 |
| `grid_scale` | float | 1.0 | 0.1–100.0 | 网格缩放系数(Δx) |
| `viscosity` | float | 0.0 | 0–0.01 | 流体粘稠度 |
| `diffusion` | float | 0.0 | 0–0.01 | 扩散系数 |
| `jacobi_pressure_iterations` | int | 40 | 1–200 | 压力Poisson求解的Jacobi迭代次数 |
| `jacobi_velocity_iterations` | int | 10 | 1–200 | 速度扩散的Jacobi迭代次数 |
| `ink_longevity` | float | 0.995 | 0.9–1.0 | 墨水/染料消散速率(每帧乘性) |
| `ink_color` | Color | (1,1,1,1) | — | 默认墨水颜色 |
| `clear_color` | Color | (0,0,0,0) | — | 颜色场背景清除颜色 |
| `color_decay` | float | 0.0005 | 0–1.0 | 颜色每帧加性衰减量 |
| `velocity_decay` | float | -1.0 | -1.0–1.0 | 速度衰减系数(-1=不衰减) |
| `vorticity_enabled` | bool | true | — | 涡度增强(Vorticity Confinement)开关 |
| `vorticity_scale` | float | 0.4 | 0–5.0 | 涡度增强强度系数 |
| `obstacle_force_strength` | float | 5.0 | 0–100.0 | 障碍物对流体的排斥力强度 |
| `density_scale` | float | 1.0 | 0.1–10.0 | 颜色注入的密度缩放系数 |
| `subtractive_mixing` | bool | false | — | 减色混合(CMY)模式 |
| `follow_mode` | enum(FollowMode) | Camera2D | — | 跟随模式: Camera2D/Node2D/None |
| `follow_path` | NodePath | — | — | 流体域跟随的节点路径(通常指向Camera2D) |
| `fluid_world_size` | Vector2 | (1920,1080) | — | 流体域在世界空间中的实际尺寸 |
| `domain_wrap_mode` | enum(DomainWrap) | Toroidal | — | 域边界包裹: Toroidal(环面无限)/Clamp(硬边界) |

**导出信号**:

| 信号名 | 参数 | 描述 |
|--------|------|------|
| `simulation_initialized` | — | GPU资源初始化完成 |
| `simulation_reset` | — | 模拟状态被重置(所有场清零) |
| `obstacle_changed` | — | 障碍物纹理被更新 |

**导出方法**:

| 方法 | 参数 | 返回值 | 描述 |
|------|------|--------|------|
| `add_impulse` | Vector2 world_pos, Vector2 strength, float radius, bool add_ink=true | void | 注入单次力+颜色 |
| `queue_draw_request` | Vector2 pos, Color color, Vector2 vel, float color_r, float vel_r | void | 队列单个绘制请求 |
| `queue_draw_batch` | PackedVector2Array, PackedColorArray, PackedVector2Array, PackedFloat32Array | void | 批量队列绘制请求 |
| `reset` | — | void | 重置所有模拟场 |
| `get_density_texture` | — | Ref\<Texture2DRD\> | 获取颜色/密度场纹理 |
| `get_velocity_texture` | — | Ref\<Texture2DRD\> | 获取速度场纹理 |
| `world_to_fluid_pos` | Vector2 world_pos | Vector2 | 世界坐标→流体像素坐标 |
| `sample_velocity` | Vector2 world_pos | Vector2 | 采样指定世界位置的流体速度 |
| `get_domain_offset` | — | Vector2 | 获取当前域偏移量(UV空间) |
| `get_domain_center_world` | — | Vector2 | 获取流体域中心在世界空间的位置 |

**背后数据资源**:

```
GPU Textures (rgba32f, Storage+Sampling+CanCopyTo+CanUpdate):
  tex_velocity      — RG通道: 二维速度向量 (U,V)
  tex_pressure      — R通道: 标量压力值
  tex_color         — RGBA通道: 颜色/密度染料
  tex_divergence    — R通道: 速度场散度值
  tex_temp          — 乒乓缓冲交换用
  tex_obstacle      — 当前帧障碍物 (RG=障碍物速度, A=遮挡)
  tex_obstacle_pre  — 前一帧障碍物 (用于差分检测运动)

GPU Storage Buffers:
  batch_buffer         — 批量splat点数据 (每点 40 bytes)
  force_emitter_buffer — 力发射器参数数组 (每个 48 bytes)
  sim_params_buffer    — 模拟参数 (64 bytes, 16-byte aligned)

Godot Output:
  Texture2DRD — 暴露给 Sprite2D / TextureRect 显示的密度纹理
```

---

### 2.2 发射器节点

#### `FluidEmitter2D` (继承 `Node2D`) — 流体粒子发射器

**职责**: 向流体模拟中注入颜色/密度粒子。支持多种发射模式、形状和预设效果。

**导出属性**:

| 属性名 | 类型 | 默认值 | 描述 |
|--------|------|--------|------|
| `active` | bool | true | 发射器激活开关 |
| `emission_mode` | enum(EmissionMode) | Continuous | Continuous/Burst/Periodic |
| `emission_shape` | enum(EmissionShape) | Point | Point/Circle/Rect/LineSegment/TextureMask |
| `emission_preset` | enum(EmitterPreset) | Custom | Explosion/Mist/Smoke/Fountain/Steam/Fire |
| `emit_color` | Color | (0.5,0.5,0.5,0.5) | 发射颜色(Alpha控制注入强度) |
| `emit_velocity` | Vector2 | (0, -1) | 发射速度方向和大小 |
| `color_radius` | float | 0.5 | 颜色注入扩散半径倍率 |
| `velocity_radius` | float | 0.8 | 速度注入扩散半径倍率 |
| `emit_interval` | float | 0.05 | 持续/周期模式发射间隔(秒) |
| `burst_count` | int | 16 | 爆发/周期模式每次粒子数 |
| `shape_size` | Vector2 | (1, 1) | 形状尺寸(Circle=X半径, Rect=宽高) |
| `velocity_pattern` | enum(VelocityPattern) | Directional | Directional/Radial/Random |
| `velocity_falloff` | float | 0.0 | 径向速度衰减指数 |
| `swirl_strength` | float | 0.0 | 旋转/切向扰动力 |
| `sim_target_path` | NodePath | — | 目标GPUStableFluids2D(空=自动查找) |
| `lifetime` | float | 0.0 | 生命周期(秒, 0=永久) |
| `auto_destroy` | bool | false | 到期自动销毁节点 |

**导出信号**: `emission_finished` — 单次爆发完成或生命周期到期时触发。

---

#### `FluidForceEmitter2D` (继承 `Node2D`) — 力场发射器

**职责**: 向流体施加方向性外力。非Mask形状使用GPU并行计算路径避免CPU瓶颈。

**导出属性**:

| 属性名 | 类型 | 默认值 | 描述 |
|--------|------|--------|------|
| `active` | bool | true | 发射器激活开关 |
| `force_pattern` | enum(ForcePattern) | Directional | Directional/PointRadial/Swirl/Centripetal/Centrifugal |
| `force` | Vector2 | (0, -1) | 力的方向和基础大小 |
| `force_radius` | float | 100.0 | 作用半径(世界单位) |
| `emission_shape` | enum(EmissionShape) | Circle | 作用形状 |
| `shape_size` | Vector2 | (1, 1) | 形状尺寸 |
| `falloff_exponent` | float | 2.0 | 距离衰减指数(0=均匀) |
| `swirl_strength` | float | 1.0 | 旋涡/切向力强度 |
| `force_preset` | enum(ForcePreset) | Custom | Wind/Gravity/Vortex/Explosion/Magnet |
| `lifetime` | float | 0.0 | 生命周期(秒) |
| `auto_destroy` | bool | false | 到期自动销毁 |

---

### 2.3 障碍物节点

#### `FluidObstacle2D` (继承 `Node2D`) — 流体障碍物

**职责**: 标记场景2D节点为流体障碍物。自动从父节点(RigidBody2D/CharacterBody2D)检测物理速度，编码到障碍物纹理RG通道用于GPU计算推力。

**导出属性**:

| 属性名 | 类型 | 默认值 | 描述 |
|--------|------|--------|------|
| `auto_detect_velocity` | bool | true | 从父物理体自动检测速度 |
| `velocity` | Vector2 | (0,0) | 手动线速度(关闭自动检测时) |
| `angular_velocity` | float | 0.0 | 手动角速度(弧度/秒) |
| `obstacle_texture` | Texture2D | — | 自定义形状纹理(Alpha>阈值为障碍) |

**接口 (`IFluidObstacle` — C++虚基类)**:

```cpp
class IFluidObstacle {
public:
    virtual Vector2 get_object_linear_velocity() const = 0;
    virtual float   get_object_angular_velocity() const = 0;
    virtual Vector2 get_object_center() const = 0;
};
```

**原理**: 内部 `FluidObstacleDrawer` 扫描 `"fluid_obstacles"` 组，将节点光栅化到 `tex_obstacle`。支持 CollisionShape2D (所有形状)、Sprite2D、ColorRect、Polygon2D。

---

#### `FluidTileMapObstacle2D` (继承 `Node2D`) — TileMap障碍物

**职责**: 将 Godot TileMapLayer 自动转换为流体障碍物纹理。

**导出属性**:

| 属性名 | 类型 | 默认值 | 描述 |
|--------|------|--------|------|
| `tile_map_path` | NodePath | — | 目标TileMapLayer路径 |
| `obstacle_mode` | enum | AllTiles | AllTiles/LayerSpecific/PhysicsLayer |
| `fill_mode` | enum | Opaque | Opaque/PerTileShape/CollisionShape |
| `physics_layer_index` | int | 0 | 物理层索引 |
| `terrain_set` | int | -1 | 地形集(-1=全部) |

---

### 2.4 工具/显示节点

#### `FluidDisplay2D` (继承 `Sprite2D`) — 流体渲染显示

**职责**: 便捷地将流体密度/速度纹理渲染到屏幕。

**导出属性**:

| 属性名 | 类型 | 默认值 | 描述 |
|--------|------|--------|------|
| `sim_source_path` | NodePath | — | 源GPUStableFluids2D |
| `display_mode` | enum | Density | Density/Velocity/Pressure/Divergence/Vorticity |
| `color_ramp` | Gradient | — | 颜色映射渐变 |
| `auto_size` | bool | true | 自动匹配流体尺寸 |
| `interpolation` | enum | Linear | 采样插值模式 |

---

#### `FluidMouseInteractor2D` (继承 `Node2D`) — 鼠标/触控交互

**职责**: 自动处理鼠标/触控输入，转换为流体力和颜色请求。

**导出属性**:

| 属性名 | 类型 | 默认值 | 描述 |
|--------|------|--------|------|
| `sim_target_path` | NodePath | — | 目标模拟节点 |
| `mouse_button` | enum | Left | 触发鼠标键 |
| `draw_color` | Color | (1,1,1,1) | 绘制颜色 |
| `brush_size` | float | 5.0 | 画笔大小(像素) |
| `velocity_scale` | float | 0.1 | 鼠标速度缩放 |
| `interaction_mode` | enum | Draw | Draw/Push/Pull/Swirl |

---

### 2.5 光照桥接节点

#### `FluidLightOccluder2D` (继承 `Node2D`) — 流体光照桥接器

**职责**: 桥梁节点，将流体密度纹理自动转换为 Godot 2D 光照系统可识别的 `LightOccluder2D` 遮罩多边形。通过 GPU Marching Square 算法从密度场提取轮廓，简化后作为 LightOccluder2D 的遮罩形状——与 FluidPlayGround 的 `FluidShadow` 系统等效。

**设计理由**: 不需要自定义 `FluidLightSource`。流体是光的*接收者*和*遮挡者*，而非光源。用户使用 Godot 标准的 `PointLight2D` / `DirectionalLight2D` 发光，本节点负责让流体正确遮挡光线。

**导出属性**:

| 属性名 | 类型 | 默认值 | 描述 |
|--------|------|--------|------|
| `sim_source_path` | NodePath | — | 源 GPUStableFluids2D 节点(空=自动查找) |
| `density_threshold` | float | 0.1 | Marching Square密度阈值(高于此值视为流体) |
| `max_contours` | int | 8 | 每帧最多提取的轮廓数(流体对象池大小) |
| `update_frequency` | int | 15 | 更新频率(Hz, 默认15=平衡质量/性能) |
| `simplify_tolerance` | float | 0.1 | 轮廓简化容差(控制多边形顶点数) |
| `min_contour_size` | int | 50 | 最小轮廓点数(过滤噪声小斑块) |
| `occluder_light_mask` | int | 1 | LightOccluder2D的遮罩层 |
| `downscale_factor` | int | 2 | 密度纹理降采样因子(性能优化, 2=1/4像素) |

**导出信号**:

| 信号名 | 参数 | 描述 |
|--------|------|------|
| `contours_updated` | int count | 新轮廓提取完成时触发 |

**工作原理** (参考 FluidPlayGround `FluidShadow.cs`):

```
每 1/update_frequency 秒:
  1. Readback: 从 tex_color 异步读取密度纹理到CPU (降采样)
  2. MarchingSquare: GPU compute → Storage Buffer 提取16种MarchingSquare格点
  3. ContourTrace: CPU端追踪连通轮廓 (Job系统并行化)
  4. Filter: 按点数过滤(丢弃<min_contour_size的噪声)
  5. Simplify: Douglas-Peucker简化多边形(减少顶点数)
  6. Create/Update: 动态创建LightOccluder2D子节点, 设置OccluderPolygon2D
  7. Pool: 超出max_contours的轮廓→释放旧LightOccluder2D到对象池
```

**与Godot 2D光照的集成**: 生成的 `LightOccluder2D` 节点自动参与 Godot 的标准 2D 光照管道——`PointLight2D` 的光线遇到密度轮廓会被遮挡，形成流体体积阴影效果。流体密度越高的区域轮廓密度越大，产生更深的自阴影。

---

### 2.6 摄像机与无限域系统设计

#### 核心问题

用户需求: (1) 模拟场只在Camera2D视野内活跃计算，(2) 产生"无限大"的流体幻觉，(3) 性能可控。

#### 设计决策: **不需要自定义流体摄像机**

`FluidPlayGround` 和 `GodotFluidSim` 均采用了相同的方案——不需要自定义摄像机类，而是将现有的 Godot `Camera2D` 作为 `follow_path` 指向的目标。

#### 实现原理: 域偏移 (Domain Shifting)

```
┌────────────────────── 无限世界 ──────────────────────┐
│                                                       │
│    ┌──────── Camera View ────────┐                   │
│    │  ┌── Sim Grid (512×512) ──┐│                   │
│    │  │ velocity/density tex    ││                   │
│    │  │ 中心对齐Camera位置      ││                   │
│    │  │ shift_texture滚动数据   ││                   │
│    │  └────────────────────────┘│                   │
│    │  仅在摄像机视野内渲染       │                   │
│    └────────────────────────────┘                   │
│                                                       │
│  流体模拟网格始终以Camera2D为中心                     │
│  摄像机移动 → FluidDomainOffset计算 → 纹理卷动        │
└──────────────────────────────────────────────────────┘
```

**每帧流程**:

```cpp
// 1. 计算域偏移 (主线程)
if (follow_node && follow_mode == FOLLOW_CAMERA2D) {
    Vector2 current_pos = follow_node->get_global_position();
    fluid_domain_offset = (current_pos - previous_pos) / fluid_world_size;
    previous_pos = current_pos;
}

// 2. 渲染线程: 在模拟循环Step 2执行
// shift_texture.glsl: 每个纹素按 -FluidDomainOffset 采样
_run_compute(shift_pipeline, {color_tex, temp_tex}, push_constant(fluid_domain_offset));
swap(color_tex, temp_tex);
_run_compute(shift_pipeline, {velocity_tex, temp_tex}, push_constant(fluid_domain_offset));
swap(velocity_tex, temp_tex);
```

**性能优势**:
- 模拟网格固定在 512×512 (或用户配置的分辨率)，GPU计算量恒定
- 域偏移仅是一次 `shift_texture.glsl` dispatch，开销几乎为零
- 环面包裹(Toroidal Wrap)边界条件让从一侧出界的流体从对侧重新进入
- 世界坐标到流体像素坐标的映射完全在GPU端完成，零CPU开销

**`follow_mode` 选项**:

| 模式 | 行为 | 适用场景 |
|------|------|---------|
| `Camera2D` | 自动跟随场景中第一个Camera2D | 单人游戏(最常见) |
| `Node2D` | 跟随 `follow_path` 指定的Node2D | 车辆/飞船模拟、多人分屏 |
| `None` | 流体域固定在场景原点，不跟随 | 固定区域流体(池塘/水池) |

---

## 三、数据资源架构

### 3.1 GPU纹理资源

所有模拟纹理统一规格:
```
Format:  R32G32B32A32_SFLOAT (rgba32f)
Type:    TEXTURE_TYPE_2D
Usage:   STORAGE_BIT | SAMPLING_BIT | CAN_COPY_TO_BIT | CAN_UPDATE_BIT
Size:    resolution.x × resolution.y
```

| 纹理 | 通道用途 | 说明 |
|------|----------|------|
| `tex_velocity` | RG=速度(U,V) | 速度场—核心数据 |
| `tex_pressure` | R=标量压力 | 压力场—Poisson解 |
| `tex_color` | RGBA=颜色 | 染料场—视觉输出 |
| `tex_divergence` | R=∇·v | 散度场—中间量 |
| `tex_temp` | 全通道 | 乒乓缓冲交换用 |
| `tex_obstacle` | RG=障碍速度, A=遮挡 | 当前帧障碍物 |
| `tex_obstacle_pre` | 同上 | 前一帧障碍物 |

### 3.2 Storage Buffer资源

| Buffer | 大小 | 用途 |
|--------|------|------|
| `batch_buffer` | MaxBatchPoints×40B (默认163840B) | 批量splat点(pos8+vel8+color16+r4+r4) |
| `force_emitter_buffer` | MaxEmitters×48B (默认1536B) | GPU力发射器参数数组 |
| `sim_params_buffer` | 64B (16B对齐) | 模拟参数uniform |

### 3.3 SimParams Uniform Buffer 布局 (GLSL std430)

```glsl
layout(set = 0, binding = 8, std430) uniform SimParams {
    vec2  resolution;            // offset 0
    float dt;                    // offset 8
    float rdx;                   // offset 12  (1.0/grid_scale)
    float viscosity;             // offset 16
    float diffusion;             // offset 20
    float color_decay;           // offset 24
    float velocity_decay;        // offset 28
    float density_scale;         // offset 32
    float obstacle_force;        // offset 36
    float vorticity_scale;       // offset 40
    int   jacobi_pressure_iter;  // offset 44
    int   jacobi_velocity_iter;  // offset 48
    float subtractive_val;       // offset 52
    float _pad[2];               // offset 56
} params;                        // total: 64 bytes
```

---

## 四、计算着色器管线 (GLSL Shaders)

### 4.1 每帧模拟流程 (16 Steps)

```
Rendering Thread Frame Loop:
─────────────────────────────────────────────────────────────────
 1. ProcessBatchQueue()       ← 主线程队列→渲染线程
 2. ProcessDomainShift()      ← shift_texture.glsl
 3. SplatBatch()              ← splat_batch.glsl
 4. ApplyForceEmitters()      ← apply_force_emitter.glsl
 5. ApplyForces()             ← apply_forces.glsl
 6. ApplyColors()             ← apply_colors.glsl
 7. ApplyObstacleForce()      ← obstacle_force.glsl
 8. AdvectVelocity()          ← advect.glsl
 9. DiffuseVelocity()         ← jacobi.glsl × N
10. ApplyVorticity()          ← vorticity.glsl
11. ComputeDivergence()       ← divergence.glsl
12. SolvePressure()           ← jacobi.glsl × N
13. SubtractPressureGradient()← subtract.glsl
14. ApplyBoundary()           ← boundary.glsl
15. AdvectColor()             ← advect.glsl
16. CopyObstacleTexture()     ← copy_texture.glsl
─────────────────────────────────────────────────────────────────
Update: output_texture.texture_rd_rid = tex_color
```

### 4.2 着色器文件清单

```
shaders/
├── advect.glsl              — 半拉格朗日平流
├── jacobi.glsl              — Jacobi迭代求解(扩散+压力共用)
├── divergence.glsl          — 散度计算 ∇·v
├── subtract.glsl            — 压力梯度减法(投影)
├── boundary.glsl            — 域边界条件
├── vorticity.glsl           — 涡度增强力
├── obstacle_force.glsl      — 障碍物排斥力
├── splat_batch.glsl         — 批量Splat
├── apply_forces.glsl        — 外部力场注入
├── apply_colors.glsl        — 外部颜色注入
├── apply_force_emitter.glsl — GPU并行力发射器
├── shift_texture.glsl       — 域偏移纹理平移
├── copy_texture.glsl        — 纹理复制
├── marching_square_extract.glsl — 密度轮廓提取(光照)
├── render_density_lit.glsl  — 密度梯度光照渲染(可选)
└── render_density.glsl      — 基础密度渲染(可选)
```

### 4.3 着色器绑定约定

```
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
// Set 0: sampler2D (SamplerWithTexture, binding=0)
// Set 1..N: restrict image2D (Image, binding=0)
// Push Constants: std430, 16~48 bytes
```

---

## 五、源文件组织

```
src/
├── register_types.h / .cpp                  # 模块声明 + 统一注册入口
├── core/
│   ├── fluid_simulation_2d.h / .cpp         # GPUStableFluids2D 主节点
│   ├── gpu_resource_manager.h / .cpp        # GPU纹理/管线生命周期管理
│   ├── fluid_render_pipeline.h / .cpp       # 渲染管线顺序调度器
│   ├── fluid_domain.h / .cpp                # FluidDomain2D 配置资源
│   ├── sim_params.h                         # SimParams Uniform结构体
│   └── fluid_types.h                        # 公共枚举/类型定义
├── emitters/
│   ├── fluid_emitter_2d.h / .cpp            # FluidEmitter2D
│   ├── fluid_force_emitter_2d.h / .cpp      # FluidForceEmitter2D
│   ├── emitter_shape_sampler.h / .cpp       # 发射形状随机采样
│   └── emitter_velocity_generator.h / .cpp  # 速度方向生成器
├── obstacles/
│   ├── fluid_obstacle_2d.h / .cpp           # FluidObstacle2D + IFluidObstacle
│   ├── fluid_obstacle_drawer.h / .cpp       # CPU障碍物光栅化
│   ├── tile_map_obstacle_drawer.h / .cpp    # TileMapLayer光栅化
│   └── polygon_rasterizer.h / .cpp          # 多边形填充工具
├── lighting/
│   ├── fluid_light_occluder_2d.h / .cpp     # FluidLightOccluder2D 桥接节点
│   ├── marching_square_extractor.h / .cpp   # GPU MarchingSquare轮廓提取
│   ├── contour_tracer.h / .cpp              # CPU连通轮廓追踪
│   └── contour_simplifier.h / .cpp          # Douglas-Peucker轮廓简化
├── display/
│   ├── fluid_display_2d.h / .cpp            # FluidDisplay2D
│   ├── fluid_mouse_interactor_2d.h / .cpp   # FluidMouseInteractor2D
│   └── fluid_vorticity_visualizer_2d.h/.cpp # Debug场可视化
└── utils/
    ├── fluid_coord_utils.h / .cpp           # 坐标转换工具
    ├── packed_buffer_builder.h              # PackedByteArray序列化
    └── draw_request.h                       # DrawRequest结构体
```

---

## 六、类继承关系

```
godot::Node2D
├── GPUStableFluids2D              ← 核心模拟节点
├── FluidEmitter2D                 ← 颜色/粒子发射器
├── FluidForceEmitter2D            ← 力场发射器
├── FluidObstacle2D                ← 通用障碍物(实现IFluidObstacle)
├── FluidTileMapObstacle2D         ← TileMap障碍物
├── FluidLightOccluder2D           ← 密度→LightOccluder2D桥接(光照)
├── FluidDisplay2D (Sprite2D)      ← 模拟结果渲染显示
├── FluidMouseInteractor2D         ← 鼠标/触控交互
└── FluidVorticityVisualizer2D     ← Debug场可视化

IFluidObstacle (虚基类)            ← 障碍物速度查询接口

godot::RefCounted
├── FluidDomain2D                  ← 流体域配置资源
└── FluidEmitterPreset             ← 发射器预设数据

godot::Resource
└── FluidTextureResource           ← 可保存/加载的纹理资源
```

---

## 七、关键设计决策

### 7.1 完整Multi-Pass模拟循环

**现有问题**: 当前 `_run_compute_shader(pass)` 每帧只调1个pass，且16步模拟未被正确循环。

**解决方案**: `FluidRenderPipeline::execute(dt, sim)` 在渲染线程单次调用中顺序执行全部16步。

### 7.2 乒乓缓冲

所有读写同一纹理的pass使用乒乓缓冲:
```cpp
_run_compute(pipeline, {sampler_tex(tex_X), image_tex(tex_temp)});
swap(tex_X, tex_temp);  // 避免read-after-write hazard
```

### 7.3 Uniform Set 缓存

缓存 `(shaderId, textureId, setIndex, uniformType)` → `RID` 映射。参考 GodotFluidSim 的 `_uniformSetCache` 实现。

### 7.4 线程安全

- **主线程**: Input处理、DrawRequest入队、属性更新
- **渲染线程**: 所有GPU操作 (`RenderingServer::call_on_render_thread`)
- 绘制请求: 预分配环形缓冲 + 原子计数器

### 7.5 批量绘制

每个绘制点40字节，Storage Buffer存4096点(GPU单次dispatch全处理):
```cpp
struct BatchPoint {
    float pos[2], vel[2];     // 16B
    float color[4];           // 16B
    float color_r, vel_r;     // 8B
}; // 40B
```

### 7.6 障碍物光栅化

参考 FluidPlayGround + GodotFluidSim:
1. 维护 `tex_obstacle`(当前) + `tex_obstacle_pre`(前一帧)
2. CPU端光栅化 `"fluid_obstacles"` 组节点 → raw buffer
3. `texture_update()` 上传GPU
4. `obstacle_force.glsl` 差分检测运动障碍物 → 推力计算
5. 帧末 `copy_texture` 当前→前一帧
6. 障碍物速度编码RG通道: `v = v_linear + ω × r`

### 7.7 无限域与Camera跟随

**设计问题**: 是否需要自定义 `FluidCamera2D` 节点？

**分析**: FluidPlayGround 和 GodotFluidSim 均**不**使用自定义摄像机。流体模拟节点通过 `follow_path: NodePath` 属性指向现有的 Godot `Camera2D`。每帧计算摄像机位置变化 → 归一化为UV空间的 `FluidDomainOffset` → `shift_texture.glsl` 卷动所有流体纹理。

**结论**: **不需要自定义摄像机节点**。直接使用 Godot 的 `Camera2D`，通过 `follow_path` 引用即可。这避免了:
- 重复实现 Camera2D 的所有功能(拖拽/平滑/限制/锚点等)
- 与 Godot 内置 camera 系统的冲突
- 用户学习成本和API不一致

**域包裹模式**: `Toroidal` (默认)——流体数据在网格边界处环面包裹，从一侧流出后从对侧流回。这与 `shift_texture` 的周期边界条件配合，完美支持无限流体幻觉。

### 7.8 流体光照设计

**设计问题**: (1) 如何实现有光照动态明暗的流体效果？(2) 是否需要 `FluidLightSource` 节点？(3) 能否直接使用 Godot 的光照系统？

**FluidPlayGround 的实现**:
- `FluidShadow` 类使用 Marching Square compute shader 从密度纹理提取轮廓
- 提取的轮廓经简化后动态创建 Unity `ShadowCaster2D` 组件
- Unity 的 2D 光照系统自动处理阴影投射
- 运行频率可配置(默认30fps，降低开销)
- LibTessDotNet 库用于轮廓三角剖分

**我们的方案——双路径**:

**路径A: Godot内置光照 + 动态LightOccluder2D (主路径)**
```
密度纹理 → MarchingSquare GPU → 轮廓提取 → CPU简化
→ 动态创建/更新 LightOccluder2D 子节点
→ Godot标准2D光照管道自动处理光线遮挡
→ PointLight2D/DirectionalLight2D 产生流体阴影
```

**路径B: 密度梯度Shader光照 (增强路径)**
```
render_density_lit.glsl 在渲染密度纹理时:
  - 计算 ∇density (Sobel算子)
  - 梯度的大小 = 表面法线强度(伪normal map)
  - 梯度方向 × Light2D方向 = 漫反射高光
  - 产生体积感的明暗效果
```

**结论**: 
- **不需要 `FluidLightSource` 节点**——用户使用 Godot 标准的 `PointLight2D` / `DirectionalLight2D`
- 流体是光的**遮挡者**而非光源——通过 `FluidLightOccluder2D` 桥接到 Godot 内置光照
- `CanvasModulate` 节点设置环境暗度，未被光照到的流体区域自然变暗
- 路径B提供额外的体积感，适用于需要"熔岩/发光流体"效果的场景

---

## 八、性能优化

| 优化 | 方案 | 预期收益 |
|------|------|---------|
| 合并Shader | 离散简单pass合并(如apply_forces+apply_colors) | -30% pipeline绑定开销 |
| GPU力发射器 | 非Mask形状→GPU并行(替代CPU逐像素) | CPU从ms→μs |
| Uniform Set缓存 | 复用组合 | 减少数百次RID创建/帧 |
| 环形缓冲 | 预分配storage buffer | 零分配零拷贝 |
| 哈希脏检测 | 障碍物位/旋/缩哈希→跳过静态帧光栅化 | 静态场景零开销 |
| 自适应Jacobi | 散度残差阈值提前终止 | 静流态迭代减半 |
| 降采样 | 256²模拟+双线性上采样渲染 | 4×计算量↓ |
| 异步Compute | Vulkan compute+draw并行 | ~15%帧时间隐藏 |
| 光照异步更新 | MarchingSquare+轮廓提取降低到15Hz | CPU开销降到每帧<0.2ms |
| 域偏移切换 | Camera静止时跳过shift_texture dispatch | 静态摄像机零开销 |
| 轮廓对象池 | LightOccluder2D对象池复用 | 零GC/零重复创建 |

---

## 九、Godot集成矩阵

| 集成点 | 实现方式 |
|--------|---------|
| **场景树** | 所有节点继承 `Node2D` |
| **信号系统** | `ADD_SIGNAL` + `emit_signal` ↔ GDScript/C# |
| **纹理系统** | `Texture2DRD` → Sprite2D/Material |
| **物理引擎** | 自动读取 RigidBody2D/CharacterBody2D 速度 |
| **Input系统** | Godot `Input` 单例 |
| **TileMap** | 直接遍历 TileMapLayer cells |
| **CollisionShape2D** | 支持所有形状类型光栅化 |
| **编辑器** | PROPERTY_HINT_RANGE/ENUM、XML文档 |
| **Group系统** | `"fluid_sim_nodes"`, `"fluid_obstacles"` 组自动发现 |
| **NodePath** | 跨节点引用(编辑器友好) |
| **2D光照系统** | `LightOccluder2D` 动态生成 → Godot标准光照管道 |
| **Camera2D** | 通过 `follow_path` 跟随，无需自定义摄像机 |
| **CanvasModulate** | 配合环境暗色实现未光照流体区域自然暗化 |
| **CanvasItemMaterial** | 流体Display可选Blend Mode(Add/Subtract/Mix) |

---

## 十、实现路线图

### Phase 1: 核心管线修复 (MVP)
- [ ] 完整16步多pass循环
- [ ] 乒乓缓冲纹理交换
- [ ] 提取 `GPUResourceManager` (纹理/管线生命周期)
- [ ] 提取 `FluidRenderPipeline` (调度逻辑)
- [ ] 正确边界条件 → `boundary.glsl`

### Phase 2: 交互系统
- [ ] 批量绘制系统 (`splat_batch.glsl` + batch_buffer)
- [ ] `FluidMouseInteractor2D` 节点
- [ ] `FluidDisplay2D` 显示节点

### Phase 3: 障碍物系统
- [ ] `FluidObstacle2D` + `IFluidObstacle` 接口
- [ ] `FluidObstacleDrawer` CPU光栅化
- [ ] `obstacle_force.glsl` GPU排斥力
- [ ] `shift_texture.glsl` 域跟随 + `follow_mode` Camera/Node/None
- [ ] `copy_texture.glsl` 帧间障碍物复制
- [ ] `domain_wrap_mode` 环面包裹边界条件

### Phase 4: 发射器系统
- [ ] `FluidEmitter2D` (全部模式+形状+预设)
- [ ] `FluidForceEmitter2D` (全部模式+形状+预设)
- [ ] `apply_force_emitter.glsl` GPU并行力发射器

### Phase 5: 光照系统
- [ ] `FluidLightOccluder2D` 桥接节点
- [ ] `marching_square_extract.glsl` GPU轮廓提取
- [ ] ContourTracer CPU连通轮廓追踪(Job并行)
- [ ] ContourSimplifier Douglas-Peucker简化
- [ ] LightOccluder2D对象池管理
- [ ] `render_density_lit.glsl` 密度梯度光照(增强路径)
- [ ] 与Godot CanvasModulate + Light2D集成测试

### Phase 6: 高级特性 & 发布
- [ ] `FluidTileMapObstacle2D` TileMap集成
- [ ] `FluidDomain2D` 可保存配置资源
- [ ] `FluidVorticityVisualizer2D` Debug工具
- [ ] 动态分辨率/自适应Jacobi
- [ ] 编辑器Plugin + 自定义节点图标(含光照节点)
- [ ] Demo场景集(基础+障碍物+发射器+光照)
- [ ] XML类文档(全部11个节点)
- [ ] CI/CD多平台自动构建
