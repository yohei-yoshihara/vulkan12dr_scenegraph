# Vulkan1.2 Triangle + Dynamic Rendering + Sync2 + Extended Dynamic State

KhronosのサンプルコードTriangle 1.3を参考にし、Vulkan 1.2でエクステンションを有効にすることで動作するようにし、さらに、シーングラフを表示できるようにしたサンプルである。

Khronosのサンプルからの違いは以下の部分である。

* Vulkan 1.3からVulkan 1.2 + Extensionで動作するようにした。このため、mac上でも動作する。Extensionはvolkでロードしている。
* Vk-Bootstrapを用いるように修正した。
* VMA(Vulkan Memory Allocator)を用いるように修正した。
* Nodeを追加することでシーンに任意のシェイプを表示できるようにした。
