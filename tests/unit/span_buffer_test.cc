#include "render/span_buffer.h"

#include <catch_amalgamated.hpp>

#include <vector>

namespace {
    std::vector<dx::ScreenPoint> quad(float x0, float y0, float x1, float y1)
    {
        return { { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } };
    }

    bool visible(const dx::SpanBuffer& spans, const std::vector<dx::ScreenPoint>& points)
    {
        return spans.polygonVisible(points.data(), static_cast<int32_t>(points.size()), nullptr);
    }

    void occlude(dx::SpanBuffer& spans, const std::vector<dx::ScreenPoint>& points)
    {
        spans.occlude(points.data(), static_cast<int32_t>(points.size()));
    }
}

TEST_CASE("span buffer hides polygons behind an occluder and keeps the rest visible")
{
    dx::SpanBuffer spans;
    spans.reset(100, 100);
    occlude(spans, quad(10, 10, 60, 60));
    CHECK_FALSE(visible(spans, quad(20, 20, 50, 50)));
    CHECK(visible(spans, quad(70, 20, 90, 50)));
    CHECK(visible(spans, quad(50, 20, 80, 50)));
    CHECK_FALSE(spans.rectVisible(15, 15, 55, 55));
    CHECK(spans.rectVisible(15, 15, 65, 55));
    CHECK_FALSE(spans.full());
}

TEST_CASE("span buffer occluders that share an edge leave no gap along it")
{
    dx::SpanBuffer spans;
    spans.reset(100, 100);
    occlude(spans, { { 10.3f, 10.7f }, { 90.2f, 11.1f }, { 89.6f, 90.4f } });
    occlude(spans, { { 10.3f, 10.7f }, { 89.6f, 90.4f }, { 10.9f, 89.8f } });
    CHECK_FALSE(visible(spans, quad(14, 14, 86, 86)));
    CHECK(visible(spans, quad(14, 14, 86, 95)));
}

TEST_CASE("span buffer restricted to open spans reports only the visible part of a polygon")
{
    std::vector<dx::ScreenSpan> open;
    for (int32_t y = 10; y < 20; y++) {
        open.push_back({ y, 20, 40 });
    }
    dx::SpanBuffer spans;
    spans.reset(100, 100, open);
    std::vector<dx::ScreenSpan> collected;
    std::vector<dx::ScreenPoint> screen = quad(0, 0, 100, 100);
    REQUIRE(spans.polygonVisible(screen.data(), 4, &collected));
    REQUIRE(collected.size() == 10);
    for (size_t i = 0; i < collected.size(); i++) {
        CHECK(collected[i].y == 10 + static_cast<int32_t>(i));
        CHECK(collected[i].x0 == 20);
        CHECK(collected[i].x1 == 40);
    }
    CHECK_FALSE(visible(spans, quad(50, 50, 60, 60)));
    CHECK_FALSE(visible(spans, quad(45, 5, 60, 30)));
}

TEST_CASE("span buffer becomes full once every row is covered")
{
    dx::SpanBuffer spans;
    spans.reset(64, 48);
    occlude(spans, quad(0, 0, 32, 48));
    CHECK_FALSE(spans.full());
    CHECK_FALSE(spans.rectVisible(0, 0, 32, 48));
    CHECK(spans.rectVisible(31, 0, 33, 1));
    occlude(spans, quad(32, 0, 64, 48));
    CHECK(spans.full());
    CHECK_FALSE(spans.rectVisible(0, 0, 64, 48));
    CHECK_FALSE(visible(spans, quad(1, 1, 63, 47)));
}
