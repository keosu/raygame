#pragma once
static void TestPointerGesture() {
    auto viewport = CanvasViewport::Fit(1280, 720, 1280, 720);
    auto frame = [](std::initializer_list<InputTouch> points) {
        InputFrame f;
        for (auto p : points)
            f.touches[f.touchCount++] = p;
        return f;
    };
    PointerGesture pointer;
    pointer.Update(frame({{11, {100, 100}}}), viewport);
    CHECK(pointer.pressed && pointer.held && pointer.IsTouch() && !pointer.dragging);
    pointer.Update(frame({{22, {400, 400}}, {11, {130, 100}}}), viewport);
    CHECK(!pointer.pressed && pointer.dragging && Approx(pointer.position.x, 130));
    pointer.Update(frame({{22, {410, 410}}}), viewport);
    CHECK(pointer.released && !pointer.held && Approx(pointer.position.x, 130));
    pointer.Update(frame({{22, {420, 420}}}), viewport);
    CHECK(!pointer.pressed && !pointer.held);
    pointer.Update(frame({}), viewport);
    pointer.Update(frame({{33, {200, 200}}}), viewport);
    auto lost = frame({{33, {200, 200}}});
    lost.focused = false;
    pointer.Update(lost, viewport);
    CHECK(pointer.cancelled && !pointer.released && !pointer.held);
    pointer.Update(frame({{33, {200, 200}}}), viewport);
    CHECK(!pointer.pressed && !pointer.held);
    pointer.Update(frame({}), viewport);
    pointer.Update(frame({{44, {200, 200}}}), viewport);
    pointer.Update(frame({{44, {220, 200}}}), CanvasViewport::Fit(1280, 720, 1600, 900));
    CHECK(pointer.cancelled && !pointer.released && !pointer.held);
    pointer.Reset();
    auto bad = frame({{55, {NAN, 0}}});
    pointer.Update(bad, viewport);
    CHECK(!pointer.pressed && !pointer.held);
    InputFrame mouse;
    mouse.mousePosition = {100, 100};
    mouse.mouse.held[0] = mouse.mouse.pressed[0] = true;
    pointer.Update(mouse, viewport);
    CHECK(pointer.held && pointer.pressed && !pointer.IsTouch());
    mouse.mouse.pressed[0] = false;
    mouse.mousePosition = {400, 200};
    pointer.Update(mouse, viewport);
    CHECK(pointer.dragging && Approx(pointer.position.x, 400));
    mouse.mouse.held[0] = false;
    mouse.mouse.released[0] = true;
    pointer.Update(mouse, viewport);
    CHECK(pointer.released && !pointer.held && pointer.dragging);
    mouse.mouse.released[0] = false;
    pointer.Update(mouse, viewport);
    CHECK(!pointer.released && !pointer.dragging);
}
