# Algorithm DLL Switching Demo

**C++ 算法部署与动态库切换示例。** 将算法实现放入独立动态库，由宿主程序通过统一接口加载、调用和释放，探索在不改动业务流程的情况下替换算法实现或推理后端。

主要面向 Windows DLL 部署场景。现有代码使用 Qt `QLibrary`，但这不代表当前工程已通过 Windows、Linux 或 macOS 的跨平台构建验证。

## 为什么做这个项目

相机采集、界面、通信与算法经常一起编译。一旦更换模型或推理引擎，就需要重编译整个应用，还容易把设备逻辑与算法依赖绑在一起。本项目希望把边界收敛到宿主与算法插件之间：宿主负责输入、调度与展示，插件负责模型资源与推理实现。

核心研究问题是：**如何在可预测的资源与并发边界内替换算法，并在替换失败时维持可用状态？**

当前仓库是已有实现与接口草稿的集合，不是已经完成的生产级热更新框架，也未提供可下载即运行的算法 DLL 与模型包。

## 已有代码与实际状态

| 部分 | 源码入口 | 当前状态 |
|---|---|---|
| Qt 应用入口 | `main.cpp` | 加载配置并创建窗口；完整应用未验证 |
| 算法类型与数据结构 | `Controller/InferProcess/Infer_base/infer_process_define.h` | 定义检测、分割、分类、OCR 类型及配置、输入、结果结构 |
| 通用加载器 | `Controller/InferProcess/Infer_base/infer_process.cpp` | 使用 `QLibrary` 解析 `CreateInferImpl` / `DeleteInferImpl`，调用实现对象 |
| 算法实现接口 | `Controller/InferProcess/Infer_base/infer_process_impl.h` | `Load`、`Infer`、`BatchSize`、`GetModelInfo` 纯虚接口 |
| 旧版后端路由 | `Controller/txr_algo_dlm/` | 包含多种 YOLO/TensorRT/OpenVINO 库名映射，不能据此视为后端全部可用 |
| 配置 | `Data/ConfigManager.*`、`config/` | YAML 配置及默认/开发/生产配置文件 |
| 采集、线程、显示、通信 | `Controller/`、`Interface/` | 存在接口或实现片段，完整链路尚未验证 |

当前类型映射：

| 类型 | 模型路径扩展名 | 动态库基名 |
|---|---|---|
| 检测 | `.det` | `infer_process_detection` |
| 分割 | `.seg` | `infer_process_segmentation` |
| 分类 | `.cls` | `infer_process_classification` |
| OCR | `.ocr` | `infer_process_ocr` |

这些扩展名用于当前代码的类型路由，不是通用模型格式标准。类型也可以由配置显式指定；不能直接假定 `.engine` 会被自动识别。

## 当前调用链

```mermaid
flowchart LR
    A[宿主配置] --> B[类型选择与库名映射]
    B --> C[QLibrary 加载]
    C --> D[CreateInferImpl]
    D --> E[实现对象 Load]
    E --> F[Infer 批量调用]
    F --> G[结果消费]
    G --> H[DeleteInferImpl]
    H --> I[卸载动态库]
```

当前 `Load()` 首先调用 `Release()`。因此新算法加载失败时，旧算法已经释放；它实现的是释放后重新加载，不是事务式切换。`Infer()`、`Release()` 等路径使用全局互斥量；这会串行化相关实例的调用，且不能自动证明所有状态访问都线程安全。

## 构建现状

根目录使用 CMake 3.16+ 和 C++17，依赖发现当前**无条件要求** Qt5（Core/Widgets/Gui/Network）、OpenCV、Boost（system/thread/filesystem）、CUDA、TensorRT 和 yaml-cpp。

可用以下命令开始诊断；这不是已验证的一键构建承诺：

```sh
cmake -S . -B build -DBUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

主要待处理项：

- 默认 `BUILD_TESTS=ON` 引用的根目录 `tests/` 尚不存在；关闭测试只避开该入口，不解决其他构建问题。
- 默认配置包含 `.engine` 路径，而自动类型推断映射使用 `.det/.seg/.cls/.ocr`，需明确显式类型与后端配置如何衔接。
- 部分控制器、窗口及数据管理只有声明；旧版加载器还引用未随仓库提供的 `tk_utility` 路径。
- 根目录递归收集源码，未按宿主与插件目标明确分离；部分深层插件目录未必被当前 glob 模式覆盖。
- 仓库中有历史 `cmake-build-debug` 产物，不应作为构建成功的证据或复用的本机缓存。
- 本轮仅核查源码和文档；当前环境未找到 `cmake`，未执行编译、DLL 加载或性能测试。

## 下一阶段框架

[架构设计与实施计划](docs/ARCHITECTURE.md) 提出以下分层：

1. **SDK**：固定宽度数据类型、版本化 C ABI、明确内存所有权。
2. **插件运行时**：库加载、符号解析、能力查询、实例生命周期。
3. **切换管理器**：预加载、预热、请求切换边界、失败回滚与旧实例回收。
4. **流水线**：有界队列、帧标识、背压与采集/推理解耦。
5. **应用层**：配置、状态显示、结果展示与诊断日志。

建议先用两个无模型依赖的 CPU 示例插件证明完整切换协议，再接入 TensorRT/OpenVINO。这样可先验证宿主协议，避免模型与驱动问题掩盖生命周期问题。

## 验收顺序

- **M1 可构建**：独立宿主和两个示例动态库，运行时确实加载不同库。
- **M2 可诊断**：缺失库、缺失符号、ABI 不匹配、初始化失败有明确错误；资源释放可重复验证。
- **M3 可切换**：候选插件先验证和预热；失败保留旧插件；在途请求只访问有效实例。
- **M4 可部署**：接入一个真实后端与固定模型，记录输入输出基准、延迟、切换耗时和部署依赖。

这些是计划，不是已完成结果。尚无性能数字、零停机承诺或完整硬件验证结论。

## 文档与贡献

新增后端时应同时描述接口版本、输入输出、设备依赖、资源归属和失败行为，并补充错误路径验证。当前仓库尚未建立统一许可证声明；公开可见不等同于明确授予复用许可。
