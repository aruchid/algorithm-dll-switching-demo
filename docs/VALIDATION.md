# 最小插件闭环验证

验证日期：2026-10-31（America/Phoenix）。默认 C++17 CLI 运行时，不包含原 Qt 应用或真实推理后端。

## 本地验收

macOS / AppleClang 21 / CMake 4.4.4：

- Release 构建：`runtime_contract`、`cli_switch` 均通过。
- Debug + AddressSanitizer / UndefinedBehaviorSanitizer：相同两组测试通过。
- 手动 CLI：插件 A 输出 `1 2 3`，插件 B 输出 `2 4 6`。

函数表采用宿主提供缓冲区，避免跨库借用静态函数表；实例销毁函数始终在库卸载前调用。并发测试验证处理结果属于完整的 A 或 B，不出现混合帧；当前使用互斥串行化，未承诺无停顿或特定延迟。

错误路径覆盖：文件不存在、入口缺失、ABI 版本不匹配、函数表过短、API 查询拒绝、必需函数为空、创建失败、预执行失败、空/非有限输入、输出溢出。候选失败后检查原插件仍可执行。另覆盖重复释放、释放后调用拒绝及重新加载。

## 跨平台回归

`.github/workflows/native.yml` 在 Windows、Linux、macOS 上配置、构建并运行 CTest。实际运行结论以对应提交的 GitHub Actions 为准。

## 未验证范围

- Qt 旧接口迁移、相机数据链、TensorRT/OpenVINO/CUDA 与真实模型。
- 图像/检测/分割等复杂 ABI、模型配置、设备能力协商。
- 崩溃隔离、取消超时、持续吞吐和长时间负载。
- 独立发布 SDK 的编译器/运行库组合兼容矩阵。
