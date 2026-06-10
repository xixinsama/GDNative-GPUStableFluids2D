# problems.md 详细回答

> 日期: 2026-06-09
> 基于对完整源码的分析

---

## 一、GPUStableFluids2D 与 FluidDisplay2D 的绑定关系

### 问题

GPUStableFluids2D 管理完整的 GPU 流体模拟管线，FluidDisplay2D 负责显示流体，这两个节点应该像 `PhysicsBody2D` 与 `CollisionShape2D` 一样绑定。当 GPUStableFluids2D 单独出现时应该显示配置警告。

### 当前状态

**没有绑定。** 当前实现中：
- `FluidDisplay2D` 通过 `sim_source_path: NodePath` 手动查找目标 `GPUStableFluids2D`
- `GPUStableFluids2D` **完全不检查**是否有 `FluidDisplay2D` 子节点
- 它们之间没有类似 `PhysicsBody2D`→`CollisionShape2D` 的父子关系约定
- 没有 `_get_configuration_warning()` 实现

**源码证据**：
- `GPU_stable_fluids_2D_init.cpp:181-192` (`_ready()`): 不检查子节点，只初始化 GPU 资源并加入 `"fluid_sim_nodes"` 组
- `fluid_display_2d.cpp:37-56` (`_ready()`): 通过 NodePath 解析查找 GPUStableFluids2D，而不是向上查找父节点

### 应该如何实现

```cpp
// GPUStableFluids2D 应新增:
String GPUStableFluids2D::_get_configuration_warning() const {
    // 检查是否有 FluidDisplay2D 子节点
    TypedArray<Node> children = get_children();
    for (int i = 0; i < children.size(); i++) {
        if (Object::cast_to<FluidDisplay2D>(children[i])) {
            return String(); // 找到了，无警告
        }
    }
    return String("该节点没有用于显示流体的子节点，因此流体模拟结果将不可见。\n"
                  "请添加一个 FluidDisplay2D 类型的子节点来显示流体。");
}
```

```cpp
// FluidDisplay2D 应自动检测父节点:
void FluidDisplay2D::_ready() {
    // 优先使用显式路径
    if (!_sim_source_path.is_empty()) {
        // ... 现有逻辑
    }
    // 否则向上查找父 GPUStableFluids2D
    Node *parent = get_parent();
    while (parent) {
        GPUStableFluids2D *sim = Object::cast_to<GPUStableFluids2D>(parent);
        if (sim) {
            // 找到！直接绑定
            bind_to_sim(sim);
            return;
        }
        parent = parent->get_parent();
    }
    // 都没找到 → 警告
    WARN_PRINT("FluidDisplay2D: 未找到 GPUStableFluids2D 父节点或 sim_source_path 无效");
}
```

### 工作量评估

约 1-2 小时改动：2 个文件修改（`GPU_stable_fluids_2D_init.cpp` 新增 `_get_configuration_warning()`，`fluid_display_2d.cpp` 增加父节点自动检测）。

---

## 二、FluidDisplay2D 的域跟随与 display_mode

### 问题

FluidDisplay2D 应该是域跟随的**表现对象**：
1. 当跟踪摄像机时，FluidDisplay2D 的 transform 跟随摄像机
2. GPUStableFluids2D 计算相应的输出给 FluidDisplay2D
3. 流体网格分辨率决定实际大小，FluidDisplay2D 的 transform.scale 控制显示大小
4. `display_mode` 变量没有生效——完全看不出几种模式的区别

### 当前状态

#### 域跟随

域跟随的**数据层**已经正确实现在 `GPUStableFluids2D` 中：
- `_process()` (main thread): 计算域偏移量
- `_gpu_process_on_render_thread()` (render thread): 执行 `shift_texture.glsl` 卷动纹理

但 **FluidDisplay2D 本身不参与域跟随**。它只是一个静态的 Sprite2D，显示完整的模拟纹理。它的位置/缩放设置在 `_ready()` 中且不再更新：

```cpp
// fluid_display_2d.cpp:51-55
if (_auto_size) {
    Vector2 ws = sim->get_fluid_world_size();
    set_scale(Vector2(ws.x / (float)sim->get_width(),
                      ws.y / (float)sim->get_height()));
}
```

**这意味着**: 当摄像机移动时，纹理数据在 GPU 上被卷动（shift），但 FluidDisplay2D 节点本身不移动。这是正确的设计——数据在 GPU 内部循环，Sprite2D 保持在场景中的位置不变。域跟随表现由 `shift_texture` 在 GPU 端完成。

但实际上，对于"摄像机跟踪"的场景，FluidDisplay2D 的 **transform 应该跟随 Camera2D**，这样 Sprite2D 始终在摄像机视野中，而 shift_texture 确保纹理数据看起来像无限大。目前缺少将 FluidDisplay2D 的 global_position 同步到摄像机位置的代码。

#### display_mode 完全不生效

**这是已确认的 Bug。** 代码证据：

```cpp
// fluid_display_2d.cpp:68
Ref<Texture2DRD> out_tex = sim->get_output_texture();
```

无论 `display_mode` 设为什么值，始终使用 `get_output_texture()`，而它始终返回 `tex_display`（= `tex_color` 的副本，即密度/颜色场）。

`DisplayMode` 枚举定义了 5 种模式：
```cpp
enum class DisplayMode {
    Density     = 0,  // ← 始终显示这个
    Velocity    = 1,  // ← 从未实现
    Pressure    = 2,  // ← 从未实现
    Divergence  = 3,  // ← 从未实现
    Vorticity   = 4,  // ← 从未实现
};
```

### 应该如何实现

#### display_mode 修复

`GPUStableFluids2D` 需要暴露多个输出纹理的 getter：
```cpp
Ref<Texture2DRD> get_output_texture() const;        // 已有 → 颜色/密度
Ref<Texture2DRD> get_velocity_texture() const;      // 需要新增
Ref<Texture2DRD> get_pressure_texture() const;      // 需要新增
Ref<Texture2DRD> get_divergence_texture() const;    // 需要新增
```

`FluidDisplay2D::_process()` 需要根据 display_mode 选择合适的纹理：
```cpp
Ref<Texture2DRD> out_tex;
switch (_display_mode) {
    case DisplayMode::Density:    out_tex = sim->get_output_texture(); break;
    case DisplayMode::Velocity:   out_tex = sim->get_velocity_texture(); break;
    case DisplayMode::Pressure:   out_tex = sim->get_pressure_texture(); break;
    case DisplayMode::Divergence: out_tex = sim->get_divergence_texture(); break;
    case DisplayMode::Vorticity:  out_tex = sim->get_vorticity_texture(); break;
}
```

#### 域跟随

`FluidDisplay2D` 应该根据 `GPUStableFluids2D` 的 `follow_mode` 自动调整 transform：
- `Camera2D` 模式：FluidDisplay2D 的位置应该实时跟踪 Camera2D
- `Node2D` 模式：跟随指定节点
- `None` 模式：保持静态

### 工作量评估

约 4-6 小时：
1. GPUStableFluids2D 新增 3 个纹理 getter + tex_display 系列的 display 纹理创建
2. FluidDisplay2D display_mode 选择逻辑
3. FluidDisplay2D 域跟随 transform 同步

---

## 三、Obstacle Nodes（障碍物节点）

### 问题

1. FluidObstacle2D 和 FluidTileMapObstacle2D **当前都没有实现成功**
2. FluidObstacle2D 不应该以施加力的方式实现，应该是流体中的**静态刚体**——流体无法进入
3. 应该有导出属性：`shape: Shape2D`, `one_way_collision: bool`, `debug_color: Color`, `sim_target: GPUStableFluids2D`
4. 其他节点也应该用 `sim_target: GPUStableFluids2D` 而非 `sim_target_path: NodePath`
5. 移动的障碍物应该**挤压推动**流体

### 当前状态详解

#### FluidObstacle2D

**已实现的部分**:
- ✅ 节点注册、属性定义
- ✅ `IFluidObstacle` 接口（速度查询）
- ✅ 自动从父 RigidBody2D/CharacterBody2D 检测速度
- ✅ `"fluid_obstacles"` 组自动发现

**问题**:
- ❌ **没有 `Shape2D` 属性**——只有 `obstacle_texture: Texture2D`
- ❌ **没有 `sim_target: GPUStableFluids2D` 直接引用**——通过 group 系统间接关联
- ❌ **没有 `debug_color` 和 `one_way_collision`**
- ❌ **障碍物是力基的，不是屏障**：

`FluidObstacleDrawer` 将障碍物光栅化到 `tex_obstacle`：
```cpp
// fluid_obstacle_drawer.cpp:117-141
void FluidObstacleDrawer::_draw_obstacle_node(FluidObstacle2D *p_obs, IFluidObstacle *p_iface) {
    // 硬编码 32x32 矩形！
    Vector2 size(32, 32);
    Vector2 half = size * 0.5f;
    // 仅当有速度时编码速度信息
    if (has_velocity) {
        fill_rect_velocity(...);  // 编码线性+角速度
    } else {
        fill_rect(...);           // 仅标记为障碍物
    }
}
```

`obstacle_force.glsl` 在障碍物像素周围施加**排斥力**：
- 它不阻止流体进入障碍物区域
- 它只是在障碍物附近施加一个向外的力
- 这是"软"障碍物，不是"硬"屏障

#### FluidTileMapObstacle2D

**完全未实现**。`_rasterize_tile_map()` 函数体为空：
```cpp
// fluid_tile_map_obstacle_2d.cpp:50-53
void FluidTileMapObstacle2D::_rasterize_tile_map() {
    // Full implementation would iterate tile cells...
    // For now, the node itself acts as a marker in the "fluid_obstacles" group
}
```

### 应该如何实现——完整重新设计

#### 刚性屏障方案

障碍物的正确方法是**在着色器层面拒绝流体进入障碍物像素**：

1. **修改 `advect.glsl`**：在回追踪采样前，检查目标像素是否在障碍物中。如果是 → 使用障碍物速度推离而非采样流体速度。

2. **修改 `jacobi.glsl`**（扩散+压力）：在障碍物像素处设置狄利克雷边界条件（速度为 0，压力梯度为 0）。

3. **修改 `boundary.glsl`**：已经在处理边界条件——障碍物内部像素应被跳过或设置为零。

4. **修改 `obstacle_force.glsl`** → 重命名为 `obstacle_boundary.glsl`：不再计算排斥力，而是直接从障碍物纹理读取速度并编码到速度场。

5. **障碍物移动推动流体**：`tex_obstacle` 的 RG 通道已经编码障碍物速度。在 `advect.glsl` 中，当采样点落在障碍物区域内时，使用障碍物的速度而非流体速度。

#### FluidObstacle2D 新属性设计

```cpp
// 需要新增的属性：
Ref<Shape2D> shape;              // 2D 形状定义（类似 CollisionShape2D）
bool one_way_collision = false;  // 单向碰撞
Color debug_color = Color(1, 0, 0, 0.4); // 编辑器调试显示颜色
GPUStableFluids2D *sim_target = nullptr; // 直接引用（非 NodePath）

// 保留的属性：
bool auto_detect_velocity = true;
Vector2 velocity;               // 手动速度（auto_detect=false 时）
float angular_velocity;
Ref<Texture2D> obstacle_texture; // 自定义纹理形状（可选，替代 Shape2D）
```

#### sim_target_path → sim_target 全局迁移

所有 8 个子节点都需要将 `NodePath` 类型的引用改为 `GPUStableFluids2D *` 类型：

| 节点 | 当前属性 | 新属性 |
|------|---------|--------|
| FluidDisplay2D | `sim_source_path: NodePath` | `sim_target: GPUStableFluids2D*` |
| FluidMouseInteractor2D | `sim_target_path: NodePath` | `sim_target: GPUStableFluids2D*` |
| FluidEmitter2D | `sim_target_path: NodePath` | `sim_target: GPUStableFluids2D*` |
| FluidForceEmitter2D | `sim_target_path: NodePath` | `sim_target: GPUStableFluids2D*` |
| FluidLightOccluder2D | `sim_source_path: NodePath` | `sim_target: GPUStableFluids2D*` |
| FluidVorticityVisualizer2D | `sim_source_path: NodePath` | `sim_target: GPUStableFluids2D*` |
| FluidObstacle2D | (无，依赖 group) | `sim_target: GPUStableFluids2D*` |
| FluidTileMapObstacle2D | (无，依赖 group) | `sim_target: GPUStableFluids2D*` |

**注意**：Godot 的 `Object *` 类型导出属性在编辑器中会显示为资源选择器。如果 GDExtension 的 `PROPERTY_HINT_NODE_TYPE` 提示正确设置，编辑器会只允许选择 `GPUStableFluids2D` 类型的节点。

#### FluidTileMapObstacle2D 完整实现

```cpp
// 需要新增的属性：
TileMapLayer *tile_map_layer = nullptr;  // 直接引用（替代 tile_map_path NodePath）
GPUStableFluids2D *sim_target = nullptr;
// ... 保留 obstacle_mode, fill_mode, physics_layer_index, terrain_set

void FluidTileMapObstacle2D::_rasterize_tile_map() {
    // 遍历 TileMapLayer 的每个 cell
    // 对于每个非空 cell，获取其 tile 形状（从 TileSet 的碰撞/纹理信息）
    // 将 tile 多边形光栅化到障碍物缓冲区
    // 调用 sim_target->get_gpu_resources() 上传障碍物纹理
}
```

### 工作量评估

这是最大的改动，约 20-30 小时：
1. **4 个着色器修改**（advect, jacobi, boundary, obstacle_force → obstacle_boundary）: 8h
2. **FluidObstacle2D 重新设计**（新增 Shape2D 属性等）: 4h
3. **FluidObstacleDrawer 重写**（使用 Shape2D 光栅化替代简单矩形）: 4h
4. **FluidTileMapObstacle2D 完整实现**: 6h
5. **8 个节点的 sim_target_path → sim_target 迁移**: 4h
6. **测试和调试**: 4h

---

## 四、编辑器显示问题

### 问题

插件中所有新增的流体 Node2D 节点，都没有在编辑器里显示其导出变量的图形关系。类似于 `CollisionShape2D` 设置 shape 后在 2D 编辑器中立即显示 shape 框。需要为插件节点增加调试显示功能。

### 当前状态

**完全没有实现。** 搜索所有源码文件：
- 没有 `_notification(NOTIFICATION_EDITOR_PREVIEW)` 
- 没有 `NOTIFICATION_DRAW` 覆盖
- 没有 `_draw()` 实现
- 没有 `queue_redraw()` 调用
- 没有编辑器调试绘制

### 应该如何实现

每个节点应该在编辑器中绘制其影响范围：

```cpp
// 所有流体节点都应实现：
void _notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_EDITOR_PREVIEW:
        case NOTIFICATION_DRAW: {
            if (Engine::get_singleton()->is_editor_hint()) {
                _draw_debug();  // 仅在编辑器中绘制
            }
        } break;
    }
}
```

具体到每个节点：

| 节点 | 调试绘制内容 |
|------|------------|
| **GPUStableFluids2D** | 流体域边界矩形（浅蓝色虚线），显示 world_size |
| **FluidDisplay2D** | 显示区域矩形（白色虚线），标注 display_mode |
| **FluidObstacle2D** | Shape2D 轮廓（debug_color 颜色），带碰撞/单向图标 |
| **FluidEmitter2D** | 发射形状（点/圆形/矩形/线段），发射方向箭头 |
| **FluidForceEmitter2D** | 力场范围圆/形状，力的方向箭头 |
| **FluidTileMapObstacle2D** | TileMap 覆盖区域的虚线矩形 |
| **FluidLightOccluder2D** | 光照遮挡区域范围矩形 |
| **FluidVorticityVisualizer2D** | 调试数据显示区域矩形 |

**性能注意事项**：
- 编辑器绘制使用 `queue_redraw()` + `_draw()` 轻量级 2D 线条/矩形 API
- 仅在导出变量改变时触发重绘（通过 setter 中调用 `queue_redraw()`）
- 运行时关闭编辑器绘制（检查 `Engine::get_singleton()->is_editor_hint()`）

### 工作量评估

约 8-12 小时：每个节点约 1-2 小时（输出图形相对标准），但需要设计和实现一致的外观风格。

---

## 五、FluidVorticityVisualizer2D 调试节点

### 问题

这个节点应该和 FluidDisplay2D 一样工作，但用于调试输出。正常情况下 GPUStableFluids2D 不应该生成调试信息，**只有当场景中有 FluidVorticityVisualizer2D 节点时才有这部分调试信息**。

### 当前状态

**完全是存根（stub）代码。** 没有任何实际功能：

```cpp
// fluid_vorticity_visualizer_2d.cpp:31-40
void FluidVorticityVisualizer2D::_ready() {
    // Debug node — no heavy initialization needed
}

void FluidVorticityVisualizer2D::_process(double p_delta) {
    if (Engine::get_singleton()->is_editor_hint()) return;
    if (!_auto_update) return;
    // Debug visualization: could draw debug lines/rects based on field values
    // Placeholder for now — the node serves as an API anchor
}
```

### 你的理解完全正确

你的理解是准确的：

1. **GPUStableFluids2D 默认不应该产生调试开销**。涡度、散度、压力等内部场的调试输出是额外的计算和纹理复制开销。

2. **只有当场景中存在 FluidVorticityVisualizer2D 节点时**，GPUStableFluids2D 才应该执行额外的调试纹理复制步骤。

3. **调试节点应该像 FluidDisplay2D 一样工作**——接收 GPUStableFluids2D 的某个内部场纹理，将其显示在屏幕上。

### 应该如何实现

#### 第一步：GPUStableFluids2D 增加 on-demand 调试输出

```cpp
// GPU_stable_fluids_2D_init.h 新增:
class FluidVorticityVisualizer2D;
std::vector<FluidVorticityVisualizer2D*> _debug_visualizers;

public:
    void register_debug_visualizer(FluidVorticityVisualizer2D *p_vis);
    void unregister_debug_visualizer(FluidVorticityVisualizer2D *p_vis);
    bool has_debug_visualizers() const;
    
    // 调试纹理 getters
    Ref<Texture2DRD> get_velocity_texture() const;
    Ref<Texture2DRD> get_pressure_texture() const;
    Ref<Texture2DRD> get_divergence_texture() const;
```

在 `_gpu_process_on_render_thread()` 末尾，**仅当 `has_debug_visualizers()` 为 true 时**，才将内部场纹理复制到 display 纹理：

```cpp
// 每帧末尾：
texture_copy(tex_color, tex_display, ...);  // 始终复制颜色（主要输出）

if (has_debug_visualizers()) {
    // 额外复制调试数据（仅在需要时）
    for (auto *vis : _debug_visualizers) {
        switch (vis->get_display_mode()) {
            case DisplayMode::Velocity:   texture_copy(tex_velocity,   tex_debug, ...); break;
            case DisplayMode::Pressure:   texture_copy(tex_pressure,   tex_debug, ...); break;
            case DisplayMode::Divergence: texture_copy(tex_divergence, tex_debug, ...); break;
            case DisplayMode::Vorticity:  // 涡度没有单独纹理，需要额外计算或近似
                                          break;
        }
    }
}
```

#### 第二步：FluidVorticityVisualizer2D 继承 Sprite2D 并像 FluidDisplay2D 一样工作

```cpp
// 应改为继承 Sprite2D（与 FluidDisplay2D 相同）:
class FluidVorticityVisualizer2D : public Sprite2D {
    // 在 _ready() 中注册到 GPUStableFluids2D
    // 在 _process() 中更新显示的纹理
    // 在析构函数中取消注册
};
```

### 工作量评估

约 4-6 小时：
1. GPUStableFluids2D 新增调试纹理注册/注销机制：2h
2. FluidVorticityVisualizer2D 从存根升级为功能完整的 Sprite2D：2h
3. 调试纹理创建和管理：1h
4. 测试：1h

---

## 总结：优先级排序

| 优先级 | 任务 | 工作量 | 影响 |
|--------|------|--------|------|
| **P0** | 障碍物系统重新设计（力基→刚性屏障） | 20-30h | 核心功能缺失 |
| **P0** | FluidDisplay2D display_mode 修复 | 4-6h | 用户可见 Bug |
| **P1** | sim_target_path → sim_target 全局迁移 | 4h | API 一致性 |
| **P1** | GPUStableFluids2D 配置警告 (缺少FluidDisplay2D) | 1-2h | 用户体验 |
| **P1** | FluidTileMapObstacle2D 完整实现 | 6h | 核心功能缺失 |
| **P2** | FluidVorticityVisualizer2D 实现 | 4-6h | 调试工具 |
| **P2** | 编辑器调试可视化（所有节点） | 8-12h | 开发体验 |
| **P3** | FluidLightOccluder2D 完整 Marching Square | 10-15h | 高级功能 |
| **P3** | GPU力发射器管线集成 | 4-6h | 高级功能 |
