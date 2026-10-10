#pragma once
#include "../src/TouchControls.h"

static void TestTouchControls() {
    using RayGame::InputFrame;
    auto viewport = RayGame::CanvasViewport::Fit(1280, 720, 2400, 1080);
    auto frame = [&](std::initializer_list<RayGame::InputTouch> points) {
        InputFrame input;
        for (auto point : points) {
            point.position = viewport.ToScreen(point.position);
            input.touches[input.touchCount++] = point;
        }
        return input;
    };
    TouchControls touch;
    touch.Update(frame({{11, {200, 590}}, {29, {1168, 570}}, {37, {1028, 590}}}), viewport, true);
    auto first = touch.Consume(), next = touch.Consume();
    Expect(first.move == 1 && first.jump && first.jumpHeld && first.dash,
           "three fingers move, jump and dash together");
    Expect(next.move == 1 && next.jumpHeld && !next.jump && !next.dash,
           "fixed ticks consume touch edges once and retain holds");
    touch.Update(frame({{37, {1028, 590}}, {29, {1168, 570}}, {11, {90, 590}}}), viewport, true);
    auto reordered = touch.Consume();
    Expect(reordered.move == -1 && reordered.jumpHeld && !reordered.jump && !reordered.dash,
           "contact reordering and direction slides preserve IDs");
    touch.Update(frame({{11, {90, 590}}}), viewport, true);
    Expect(!touch.Consume().jumpHeld && !touch.dashHeld,
           "lifting one finger releases only its controls");
    touch.Update(frame({}), viewport, true);
    touch.Update(frame({{44, {1168, 570}}}), viewport, true);
    touch.Update(frame({}), viewport, true);
    auto quick = touch.Consume();
    Expect(quick.jump && !quick.jumpHeld && !touch.Consume().jump,
           "a tap between fixed ticks is retained once");
    touch.Update(frame({{45, {320, 590}}, {46, {1168, 570}}}), viewport, true);
    touch.Update(frame({}), viewport, true);
    auto drop = touch.Consume();
    Expect(drop.jump && drop.down && !touch.Consume().down,
           "short down-plus-jump retains the drop modifier until consumed");
    auto held = frame({{44, {1168, 570}}});
    touch.Update(held, viewport, true);
    touch.Reset(held);
    touch.Update(held, viewport, true);
    Expect(!touch.Consume().jump && !touch.Held().jumpHeld,
           "page transitions reject existing fingers until release");
    touch.Update(frame({}), viewport, true);
    touch.Update(held, viewport, true);
    Expect(touch.Consume().jump, "a fresh touch works after the transition barrier");
    InputFrame lost;
    lost.focused = false;
    touch.Update(lost, viewport, true);
    Expect(!touch.Consume().jumpHeld, "focus loss clears touch holds and buffered input");
    touch.Update(held, viewport, true);
    Expect(!touch.Consume().jumpHeld, "regaining focus cannot synthesize a held jump");
    touch.Update(frame({}), viewport, true);
    InputFrame margin;
    margin.touchCount = 1;
    margin.touches[0] = {50, {20, 800}};
    touch.Update(margin, viewport, true);
    Expect(touch.Consume().move == 0, "letterbox touches do not control the game");
    touch.Update(frame({{50, {90, 590}}}), viewport, true);
    Expect(touch.Consume().move == 0,
           "dragging a letterbox finger into a control cannot acquire it");
    touch.Update(frame({}), viewport, true);
    touch.Update(frame({{53, {1188, 122}}}), viewport, true);
    Expect(touch.pausePressed && !touch.Consume().jump, "pause has its own touch region");
    touch.Update(frame({{53, {1188, 122}}}), viewport, true);
    Expect(!touch.pausePressed, "holding pause does not toggle repeatedly");
    touch.Reset(frame({}));
    Rectangle button{90, 429, 405, 52};
    touch.Update(frame({{61, {200, 452}}, {62, {1100, 670}}}), viewport, false);
    Expect(touch.pointerPressed && touch.pointerHeld, "menu captures one primary finger");
    touch.Update(frame({{62, {1100, 670}}}), viewport, false);
    Expect(touch.Click(button) && !touch.Click(button),
           "menu release clicks exactly once in the original button");
    touch.Update(frame({{62, {200, 452}}}), viewport, false);
    Expect(!touch.pointerPressed && !touch.pointerHeld,
           "another held finger is never promoted into a menu click");
    touch.Update(frame({}), viewport, false);
    touch.Update(frame({{65, {200, 452}}}), viewport, false);
    touch.Update(frame({{65, {700, 452}}}), viewport, false);
    touch.Update(frame({}), viewport, false);
    Expect(!touch.Click(button), "dragging out cancels menu activation");
    touch.Update(frame({{66, {1168, 570}}}), viewport, true);
    auto resized = RayGame::CanvasViewport::Fit(1280, 720, 1280, 960);
    touch.Update(frame({{66, {1168, 570}}}), resized, true);
    Expect(!touch.Consume().jump && !touch.Held().jumpHeld,
           "surface resizing cancels pending and held gestures");
    InputFrame invalid;
    invalid.touchCount = 1;
    invalid.touches[0] = {70, {NAN, INFINITY}};
    touch.Update(invalid, resized, true);
    Expect(touch.Consume().move == 0 && !touch.Held().jumpHeld,
           "invalid touch positions cannot acquire controls");
    auto wide =
        RayGame::CanvasViewport::Calculate(1280, 720, 2400, 1080, RayGame::CanvasScaleMode::Expand);
    touch.Update(InputFrame{}, wide, true);
    InputFrame mobile;
    mobile.touchCount = 3;
    mobile.touches[0] = {101, wide.ToScreen({200, 590})};
    mobile.touches[1] = {102, wide.ToScreen({1168 + wide.logicalSize.x - 1280, 570})};
    mobile.touches[2] = {103, wide.ToScreen({1028 + wide.logicalSize.x - 1280, 590})};
    touch.Update(mobile, wide, true);
    auto actions = touch.Consume();
    Expect(actions.move == 1 && actions.jump && actions.dash,
           "phone aspect anchors both hands to the screen edges");
    auto jumpBounds = touch.Bounds(TouchControls::JumpButton);
    Expect(std::fabs(wide.logicalSize.x - jumpBounds.x - jumpBounds.width - 56) < 0.01f,
           "jump keeps a stable right margin after canvas expansion");
    mobile.touches[1].position = {NAN, 0};
    touch.Update(mobile, wide, true);
    Expect(!touch.Held().jumpHeld && touch.Held().move == 1,
           "invalid jump contact cancels without dropping other fingers");
}
