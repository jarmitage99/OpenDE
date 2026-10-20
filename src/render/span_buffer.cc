#include "render/span_buffer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dx {
    void SpanBuffer::reset(int32_t width, int32_t height)
    {
        columns = std::max(width, 0);
        rows.resize(static_cast<size_t>(std::max(height, 0)));
        for (std::vector<Interval>& row : rows) {
            row.clear();
            if (columns > 0) {
                row.push_back({ 0, columns });
            }
        }
        openRows = columns > 0 ? static_cast<int32_t>(rows.size()) : 0;
    }

    void SpanBuffer::reset(int32_t width, int32_t height, const std::vector<ScreenSpan>& open)
    {
        columns = std::max(width, 0);
        rows.resize(static_cast<size_t>(std::max(height, 0)));
        for (std::vector<Interval>& row : rows) {
            row.clear();
        }
        for (const ScreenSpan& span : open) {
            int32_t x0 = std::max(span.x0, 0);
            int32_t x1 = std::min(span.x1, columns);
            if (span.y < 0 || static_cast<size_t>(span.y) >= rows.size() || x0 >= x1) {
                continue;
            }
            rows[static_cast<size_t>(span.y)].push_back({ x0, x1 });
        }
        openRows = 0;
        for (std::vector<Interval>& row : rows) {
            if (row.empty()) {
                continue;
            }
            std::sort(row.begin(), row.end(), [](const Interval& a, const Interval& b) {
                return a.x0 < b.x0;
            });
            size_t kept = 0;
            for (size_t i = 1; i < row.size(); i++) {
                if (row[i].x0 <= row[kept].x1) {
                    row[kept].x1 = std::max(row[kept].x1, row[i].x1);
                } else {
                    row[++kept] = row[i];
                }
            }
            row.resize(kept + 1);
            openRows++;
        }
    }

    bool SpanBuffer::full() const
    {
        return openRows <= 0;
    }

    bool SpanBuffer::rectVisible(int32_t x0, int32_t y0, int32_t x1, int32_t y1) const
    {
        x0 = std::max(x0, 0);
        x1 = std::min(x1, columns);
        y0 = std::max(y0, 0);
        y1 = std::min(y1, static_cast<int32_t>(rows.size()));
        if (x0 >= x1) {
            return false;
        }
        for (int32_t y = y0; y < y1; y++) {
            for (const Interval& gap : rows[static_cast<size_t>(y)]) {
                if (gap.x0 >= x1) {
                    break;
                }
                if (gap.x1 > x0) {
                    return true;
                }
            }
        }
        return false;
    }

    bool SpanBuffer::polygonVisible(const ScreenPoint* points, int32_t count, std::vector<ScreenSpan>* visible) const
    {
        float top = points[0].y;
        float bottom = points[0].y;
        for (int32_t i = 1; i < count; i++) {
            top = std::min(top, points[i].y);
            bottom = std::max(bottom, points[i].y);
        }
        int32_t first = std::max(static_cast<int32_t>(std::floor(top)) - 1, 0);
        int32_t last = std::min(static_cast<int32_t>(std::ceil(bottom)) + 1, static_cast<int32_t>(rows.size()));
        bool found = false;
        for (int32_t y = first; y < last; y++) {
            const std::vector<Interval>& row = rows[static_cast<size_t>(y)];
            if (row.empty()) {
                continue;
            }
            float low = std::clamp(static_cast<float>(y), top, bottom);
            float high = std::clamp(static_cast<float>(y + 1), top, bottom);
            float left = std::numeric_limits<float>::max();
            float right = std::numeric_limits<float>::lowest();
            for (int32_t i = 0, j = count - 1; i < count; j = i++) {
                const ScreenPoint& a = points[j];
                const ScreenPoint& b = points[i];
                float edgeLow = std::min(a.y, b.y);
                float edgeHigh = std::max(a.y, b.y);
                if (edgeHigh < low || edgeLow > high) {
                    continue;
                }
                if (edgeHigh == edgeLow) {
                    left = std::min({ left, a.x, b.x });
                    right = std::max({ right, a.x, b.x });
                    continue;
                }
                float slope = (b.x - a.x) / (b.y - a.y);
                float start = a.x + (std::max(low, edgeLow) - a.y) * slope;
                float end = a.x + (std::min(high, edgeHigh) - a.y) * slope;
                left = std::min({ left, start, end });
                right = std::max({ right, start, end });
            }
            if (left > right) {
                continue;
            }
            int32_t x0 = std::max(static_cast<int32_t>(std::floor(left)) - 1, 0);
            int32_t x1 = std::min(static_cast<int32_t>(std::ceil(right)) + 1, columns);
            for (const Interval& gap : row) {
                if (gap.x0 >= x1) {
                    break;
                }
                if (gap.x1 <= x0) {
                    continue;
                }
                if (!visible) {
                    return true;
                }
                found = true;
                visible->push_back({ y, std::max(gap.x0, x0), std::min(gap.x1, x1) });
            }
        }
        return found;
    }

    void SpanBuffer::occlude(const ScreenPoint* points, int32_t count)
    {
        float top = points[0].y;
        float bottom = points[0].y;
        for (int32_t i = 1; i < count; i++) {
            top = std::min(top, points[i].y);
            bottom = std::max(bottom, points[i].y);
        }
        int32_t first = std::max(static_cast<int32_t>(std::ceil(top - 0.5f)), 0);
        int32_t last = std::min(static_cast<int32_t>(std::ceil(bottom - 0.5f)), static_cast<int32_t>(rows.size()));
        for (int32_t y = first; y < last; y++) {
            if (rows[static_cast<size_t>(y)].empty()) {
                continue;
            }
            float center = static_cast<float>(y) + 0.5f;
            float left = std::numeric_limits<float>::max();
            float right = std::numeric_limits<float>::lowest();
            for (int32_t i = 0, j = count - 1; i < count; j = i++) {
                const ScreenPoint& lower = points[j].y <= points[i].y ? points[j] : points[i];
                const ScreenPoint& upper = points[j].y <= points[i].y ? points[i] : points[j];
                if (center < lower.y || center >= upper.y) {
                    continue;
                }
                float x = lower.x + (center - lower.y) * (upper.x - lower.x) / (upper.y - lower.y);
                left = std::min(left, x);
                right = std::max(right, x);
            }
            if (left > right) {
                continue;
            }
            int32_t x0 = std::max(static_cast<int32_t>(std::ceil(left - 0.5f)), 0);
            int32_t x1 = std::min(static_cast<int32_t>(std::ceil(right - 0.5f)), columns);
            if (x0 < x1) {
                cover(y, x0, x1);
            }
        }
    }

    void SpanBuffer::cover(int32_t y, int32_t x0, int32_t x1)
    {
        std::vector<Interval>& row = rows[static_cast<size_t>(y)];
        for (size_t i = 0; i < row.size(); i++) {
            Interval gap = row[i];
            if (gap.x0 >= x1) {
                break;
            }
            if (gap.x1 <= x0) {
                continue;
            }
            if (gap.x0 < x0 && gap.x1 > x1) {
                row[i].x1 = x0;
                row.insert(row.begin() + static_cast<std::ptrdiff_t>(i) + 1, Interval { x1, gap.x1 });
                return;
            }
            if (gap.x0 < x0) {
                row[i].x1 = x0;
                continue;
            }
            if (gap.x1 > x1) {
                row[i].x0 = x1;
                return;
            }
            row.erase(row.begin() + static_cast<std::ptrdiff_t>(i));
            i--;
        }
        if (row.empty()) {
            openRows--;
        }
    }
}
