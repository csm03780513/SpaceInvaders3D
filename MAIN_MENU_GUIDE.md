# Main Menu (Start Button + Title/Logo) – Integration Guide

Use this guide to add a proper main‐menu screen to this Vulkan/NDK project. It follows the code that already exists in `app/src/main/cpp` (renderer, states, input) and keeps changes local to the current architecture.

## What you have today
- `Game` runs a simple state machine (`MainMenu`, `Playing`, `Won`, `Lost`).
- `MainMenuState` immediately jumps to `Playing` on any touch by enqueuing `RestartGameCommand`.
- Rendering already supports textured quads (main pipeline), a font atlas, and an overlay quad.
- Text “Space Endure v0.0.3” is loaded as `GameText::Title` in `Renderer::loadText()`.

## Assets to prepare
1. Create two PNGs and drop them in `app/src/main/assets/textures/`  
   - `logo.png` – transparent background; ~1024px wide gives crisp results on high‑dpi phones.  
   - `start_button.png` – the button body; size around 512×192 works well.
2. Keep them uncompressed (PNG) so the existing `loadTexture` helper can read them.

## 1) Register the new textures
Update the enums and texture plumbing so the renderer can sample the new images.

- In `app/src/main/cpp/GameObjectData.h` extend the texture enum:
  ```cpp
  enum class GameTextureType {
      Ship,
      Alien,
      ShipBullet,
      FontAtlas,
      Overlay,
      PowerUp,
      Logo,          // new
      StartButton    // new
  };
  ```
- In `Renderer` (private fields near the other images) add storage for both textures (`VkImage`, `VkDeviceMemory`, `VkImageView`, `VkSampler`).
- In `Renderer::loadAllTextures()` load the files:
  ```cpp
  loadTexture("logo.png",        logoImage_,  logoMemory_,  logoView_,  logoSampler_,  GameTextureType::Logo);
  loadTexture("start_button.png",startBtnImage_,startBtnMemory_,startBtnView_,startBtnSampler_,GameTextureType::StartButton);
  ```
- Increase the main descriptor texture array from 5 to 7 in `createMainDescriptor` and include the new `VkDescriptorImageInfo` entries in the same order you declared in the enum.  
  Remember to bump the `textureCount` constant and enlarge the descriptor pool counts.
- Update the shader array size in `app/src/main/cpp/shaders/main/main.frag`:
  ```glsl
  layout (set = 0, binding = 1) uniform sampler2D textures[7];
  ```
  Recompile shaders (see “Build & verify” below).

## 2) Draw the menu UI
Render the logo and start button as textured quads using the existing main pipeline.

1. Add a helper method to `Renderer`, e.g. `void drawMainMenuUI(VkCommandBuffer cmd);`.
2. Call it from the command-buffer recording path right after the non‑playing overlay is drawn (the block that currently tints the screen when `gameState != GameState::Playing`).
3. Example body (values can be tweaked):
   ```cpp
   void Renderer::drawMainMenuUI(VkCommandBuffer cmd) {
       VkDeviceSize offsets[] = {0};
       vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipeline_);
       vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mainPipelineLayout_,
                               0, 1, &shipDescriptorSet_, 0, nullptr);

       // Logo at the top-center
       MainPushConstants logo{};
       logo.pos = {0.0f, -0.6f};       // x,y in NDC
       logo.scale = {2.4f, 0.9f};      // widen/narrow logo quad
       logo.texturePos = static_cast<uint>(GameTextureType::Logo);
       vkCmdPushConstants(cmd, mainPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0,
                          sizeof(MainPushConstants), &logo);
       vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer_, offsets);
       vkCmdDraw(cmd, 6, 1, 0, 0);

       // Start button at mid-screen
       MainPushConstants start{};
       start.pos = {0.0f, 0.15f};
       start.scale = {1.8f, 0.7f};
       start.texturePos = static_cast<uint>(GameTextureType::StartButton);
       start.canPulse = 1; // subtle breathing effect already in shader
       start.time = GameTime::time;
       vkCmdPushConstants(cmd, mainPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0,
                          sizeof(MainPushConstants), &start);
       vkCmdDraw(cmd, 6, 1, 0, 0);
   }
   ```
4. If you do not want the colored tint during the menu, guard the existing overlay draw with `gameState == GameState::Lost || gameState == GameState::Won` so the main menu stays clear.

## 3) Update the title text
- In `Renderer::loadText()` change the string to your final title and re-center it:
  ```cpp
  std::vector<Vertex> titleVertices = fontManager_->buildTextVertices(
      "Space Invaders 3D", -0.55f, -0.75f, 0.0f, 0.0032f);
  ```
- You can add a subtitle (e.g., “Tap START to play”) by inserting another `buildTextVertices` block after the title and storing it in `allTextVertices` with a new `GameText` entry if desired.

## 4) Route input to the start button instead of “tap anywhere”
The current `MainMenuState` never sees coordinates because `handle_input` always enqueues a `RestartGameCommand`. Route touches into the state and gate the transition on a hit test.

1. In `main.cpp` change the non‑playing touch branch:
   ```cpp
   case AMOTION_EVENT_ACTION_DOWN:
   case AMOTION_EVENT_ACTION_POINTER_DOWN:
       if (state == GameState::Playing) {
           command = std::make_unique<MoveShipCommand>(...);
       } else {
           InputEvent evt{InputEventType::TouchDown, normalizedX, normalizedY};
           context->game->handleInput(evt); // forward to MainMenuState
           return 1; // skip enqueuing RestartGameCommand
       }
       break;
   ```
2. In `MainMenuState::handleInput` perform a simple AABB hit test using the same center/scale you used when drawing the button:
   ```cpp
   void MainMenuState::handleInput(const InputEvent &event) {
       if (event.type != InputEventType::TouchDown) return;
       const float halfW = 0.18f * 1.8f; // quad half-width (0.18 from quadVerts * scale.x)
       const float halfH = 0.045f * 0.7f;
       const float cx = 0.0f, cy = 0.15f;
       bool inside = std::abs(event.normalizedX - cx) <= halfW &&
                     std::abs(event.normalizedY - cy) <= halfH;
       if (inside) {
           game_.requestState(GameState::Playing);
       }
   }
   ```
3. Remove or ignore the `RestartGameCommand` path for the menu so taps outside the button do nothing.

## 5) Build & verify
- Rebuild shaders (outputs go to `.spv` files referenced by `PipelineBuilder`):
  ```powershell
  cd app/src/main/cpp/shaders
  .\\compile.bat
  ```
- Recompile the app:
  ```powershell
  cd E:\\carlo\\SpaceInvaders3D
  .\\gradlew assembleDebug
  ```
- Install and launch. On the main menu you should see logo + START button; only tapping START should transition to gameplay; overlay tint should still appear on win/loss screens if you kept it.

## Troubleshooting tips
- If the button is invisible, confirm `textures[]` size in the fragment shader matches the count in `createMainDescriptor`.
- If tapping START never works, log the received normalized coords in `MainMenuState::handleInput` to verify your hit box matches the rendered location.
- If colors look off, check that `start_button.png` includes an alpha channel; the overlay shader multiplies RGB by the push‑constant color.

Once these steps are in place you have a dedicated main menu with a branded title, logo, and a click‑targeted start button that fits the project’s existing Vulkan rendering and state setup.
