# Vulkan1.2 Triangle + Dynamic Rendering + Sync2 + Extended Dynamic State

![スクリーンショット 2025-06-19 8 31 05](https://github.com/user-attachments/assets/63c468ed-2194-478d-a377-d25e3524c8d5)

KhronosのサンプルコードTriangle 1.3をVulkan 1.2でエクステンションを有効にすることで動作するようにし、さらに、シーングラフを表示できるようにしたサンプルである。

Khronosのサンプルから変更した部分は以下である。

* Vulkan 1.3からVulkan 1.2 + Extensionで動作するようにした。このため、mac上でも動作する。
* Vk-Bootstrapを用いるように修正した。
* VMA(Vulkan Memory Allocator)を用いるように修正した。
* Nodeを追加することでシーンに任意のシェイプを表示できるようにした。
