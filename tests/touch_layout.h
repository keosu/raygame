#pragma once

static void TestTouchLayout() {
    CanvasLayout layout{{1280, 720}, {20, 10, 1680, 900}};
    auto left = layout.Place(Rectangle{40, 540, 96, 96}, {0, 1});
    auto right = layout.Place(Rectangle{1112, 516, 112, 120}, {1, 1});
    CHECK(Approx(left.x, 60) && Approx(left.y, 730));
    CHECK(Approx(right.x, 1532) && Approx(right.y, 706));
    CHECK(Approx(layout.Offset({.5f, .5f}).x, 220));

    auto viewport = CanvasViewport::Fit(1280, 720, 2400, 1080);
    auto frame = [&](std::initializer_list<InputTouch> points) {
        InputFrame input;
        for (auto point : points) {
            point.position = viewport.ToScreen(point.position);
            input.touches[input.touchCount++] = point;
        }
        return input;
    };
    TouchTracker touches;
    touches.Update(frame({{7, {100, 200}}, {42, {900, 500}}}), viewport);
    CHECK(touches.Contacts()[0].id == 7 && touches.Contacts()[0].pressed);
    CHECK(touches.Contacts()[1].id == 42 && touches.Contacts()[1].pressed);
    touches.Update(frame({{42, {920, 500}}, {7, {110, 200}}}), viewport);
    CHECK(touches.Contacts()[0].id == 7 && !touches.Contacts()[0].pressed);
    CHECK(Approx(touches.Contacts()[0].position.x, 110));
    CHECK(Approx(touches.Contacts()[0].start.x, 100));
    touches.Update(frame({{42, {930, 500}}, {99, {300, 300}}}), viewport);
    CHECK(touches.Released()[0].id == 7 && touches.Contacts()[0].id == 99);
    CHECK(touches.Contacts()[0].pressed && !touches.Contacts()[1].pressed);
    auto held = frame({{42, {930, 500}}, {99, {300, 300}}});
    touches.Reset(held);
    touches.Update(held, viewport);
    CHECK(!touches.Contacts()[0].Active() && !touches.Contacts()[1].Active());
    touches.Update(frame({}), viewport);
    touches.Update(frame({{42, {800, 500}}}), viewport);
    CHECK(touches.Contacts()[0].Active());
    InputFrame unfocused;
    unfocused.focused = false;
    touches.Update(unfocused, viewport);
    CHECK(touches.Cancelled() && !touches.Released()[0].Active());
    touches.Update(held, viewport);
    CHECK(touches.Cancelled() && !touches.Contacts()[0].Active());
    touches.Update(frame({}), viewport);
    touches.Update(frame({{12, {-100, 300}}}), viewport);
    touches.Update(frame({{12, {100, 300}}}), viewport);
    CHECK(!touches.Contacts()[0].Active());
    touches.Update(frame({}), viewport);
    touches.Update(frame({{15, {100, 300}}, {15, {200, 300}}}), viewport);
    CHECK(touches.Contacts()[0].id == 15 && touches.Contacts()[1].id == -1);
    touches.Update(frame({{15, {NAN, 300}}}), viewport);
    CHECK(!touches.Released()[0].Active());
    touches.Update(frame({{15, {100, 300}}}), viewport);
    CHECK(!touches.IsActive(15));
    touches.Update(frame({}), viewport);
    touches.Update(held, viewport);
    auto changed = viewport;
    changed.logicalSize.x += 10;
    touches.Update(held, changed);
    CHECK(touches.Cancelled() && !touches.Contacts()[0].Active());
}
