# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

Luna is a C++20 rendering engine for Windows with two graphics backends, **DirectX 12** and **Vulkan** (1.4), behind a common abstraction. `Apps/` holds ~35 small sample apps (Bloom, ShadowMapping, MeshletCull, VariableRateShading, ...), each exercising one technique on both backends. `TODO.txt` tracks planned and finished engine work.

## Build

- Visual Studio 2022+ solution: `Luna.slnx` (`Luna.sln` is the legacy form). Configurations: `Debug`, `Profile`, `Release`, platform `x64`.
- Prerequisites: the **Vulkan SDK** with `VULKAN_SDK` set (engine includes `$(VULKAN_SDK)\Include`; the shader compiler uses `$(VULKAN_SDK)\Bin\dxc.exe` for SPIR-V), vcpkg (manifest mode via `vcpkg.json` / `Directory.Build.props`, installs to `vcpkg_installed/`), NuGet restore (`packages.config`: D3D12 Agility SDK, glfw, CLI11, WIL, WinPixEventRuntime), and the .NET 8 SDK.
- Build one app from the command line (MSBuild from a VS Developer prompt):
  ```
  msbuild Luna.slnx /t:Bloom /p:Configuration=Debug /p:Platform=x64 /restore
  ```
- Every C++ project depends on `Tools/ShaderCompiler` (C# .NET 8). Shader compile steps run `Tools/ShaderCompiler/bin/Release/net8.0/ShaderCompiler.exe` and are **silently skipped if that exe doesn't exist**, so build ShaderCompiler in Release first if shaders aren't being produced.
- There are no unit tests or linters; verify changes by running the relevant sample apps on both backends.

## Running apps

- Output goes to `Apps/<App>/Bin/<Configuration>/`. Command-line flags (parsed in `Application::ProcessCommandLine`): `--dx12` (default) or `--vk`/`--vulkan`, `--width`/`--resx`, `--height`/`--resy`.
- Assets are found via the file system search paths in `Application::SetDefaultSearchPaths`: `Data/...` and `../Data/...` relative to the working dir (shared models/textures live in `Apps/Data`, per-app compiled shaders in `Apps/<App>/Data/Shaders/{DXIL,SPIRV}`).

## Creating a new app

```
MakeNewApp.bat <AppName>
```
Run from the repo root (needs Python 3). It copies `Programs/AppGen/Templates` into `Apps/<AppName>`; you must then add the `.vcxproj` to `Luna.slnx` yourself (with a build dependency on ShaderCompiler, like the other apps).

## Architecture

**Application lifecycle** (`Engine/Application.h`): apps subclass `Luna::Application` and override `Configure` (pre-device), `Startup`, `Shutdown`, `Update`, `UpdateUI` (ImGui via `m_uiOverlay`), `Render`, `CreateDeviceDependentResources`, and `CreateWindowSizeDependentResources`. `main` constructs the app, calls `ProcessCommandLine`, then `Luna::Run(&app)`. The base class provides `m_camera`, `m_inputSystem`, `m_timer`, a default depth buffer, grid, and UI overlay.

**Graphics abstraction** (`Engine/Graphics/`): API-neutral interfaces (`IDevice`, `IGpuBuffer`, `IColorBuffer`, `IDepthBuffer`, `IRootSignature`, `IDescriptorSet`, pipelines, `ITexture`, query heaps, samplers) with `...Ptr` shared-pointer aliases. Resources are created from `...Desc` structs (designated-initializer style) via `IDevice`, or helper free functions like `CreateConstantBuffer`/`CreateVertexBuffer` in `GpuBuffer.h`. Backend implementations live in `Graphics/DX12/*12.{h,cpp}` and `Graphics/Vulkan/*VK.{h,cpp}`, with matching file names — a change to one backend almost always needs the mirrored change in the other. `DeviceManager12`/`DeviceManagerVK` own device creation, swapchain, and present. `DeviceCaps` exposes per-backend feature support.

**Command recording**: `GraphicsContext::Begin("name")` / `ComputeContext` → explicit `TransitionResource` calls → `Finish()`. `CommandContext.h` is the common front end dispatching to `CommandContext12` / `CommandContextVK`. Dynamic descriptors (`SetSRV`/`SetUAV`/etc.) go through `DynamicDescriptorHeap*`; per-frame transient data uses the linear allocators.

**Binding model**: HLSL-style root signatures are used on both APIs. `RootSignatureVK` maps them to Vulkan descriptor set layouts / pipeline layouts; descriptor sets are allocated from a root signature for lower-frequency bindings.

**Shaders**: HLSL only. Each app's `Shaders/shaders.cfg` lists one shader per line (`File.hlsl -T <stage> -E <entry>`); `LunaShaderCfg.targets` compiles every entry to DXIL (SM 6.5, `DX12=1`) and SPIR-V (`VK=1`, DX memory layout, register shifts t=0, s=128, b=256, u=384). Code references shaders by base name (e.g. `.shaderFile = "ColorPassVS"`). Engine shaders in `Engine/Graphics/Shaders` are compiled by a post-build step in `Engine.vcxproj` into `Apps/Data/Shaders`; shared includes are in `Engine/Graphics/Shaders/Common`. Add new shaders to `shaders.cfg` or they won't be built.

**Assets**: models are glTF (via assimp, `Graphics/Model.h`); textures via DDS/KTX/stb loaders in `Graphics/Loaders`. Math is DirectXMath-based (`Engine/Core/Math`, namespace `Math`).

**Profiling**: FramePro (`External/FramePro`, `LunaFramePro.h`) and PIX events (`ScopedEvent`, `Core/Profiling.h`).

## Conventions

- Files carry the MIT license header with `Author: David Elder`; each project uses a `Stdafx.h` precompiled header and `#include "Stdafx.h"` first.
- Tabs for indentation, `m_` member prefix, `Luna` namespace, Allman braces, two blank lines between function definitions.
