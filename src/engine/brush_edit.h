#pragma once

#include "engine/resources.h"

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace dx {
    namespace brushedit {
        Vector mainScale(UObject* brush);
        void setMainScale(UObject* brush, const Vector& scale);
        void mirror(UObject* brush, int axis);
        void resetScale(UObject* brush);
        void resetRotation(UObject* brush);
        void resetAll(UObject* brush);
        void applyTransform(UObject* brush);
        Vector toWorld(UObject* brush, const Vector& local);
        Vector toLocal(UObject* brush, const Vector& world);
        std::vector<Poly> worldPolys(UObject* brush);
        std::vector<Vector> localVertices(const UModel& model);
        std::vector<std::pair<Vector, Vector>> localEdges(const UModel& model);
        void snapVertices(UObject* brush, const Vector& grid);
        void setPivot(UObject* brush, const Vector& world);
        void resetPivot(UObject* brush);
        size_t moveVertices(UModel& model, const std::vector<Vector>& from, const Vector& offset);
        size_t transformVertices(UModel& model, const std::vector<Vector>& from, const std::function<Vector(const Vector&)>& transform);
        int32_t findPoly(const UModel& model, const std::vector<Vector>& vertices);
        bool extrudeFace(UModel& model, size_t index, const Vector& offset);
        int32_t rayPoly(const std::vector<Poly>& polys, const Vector& start, const Vector& direction, float& distance);
        void validate(UModel& model);
        std::vector<Poly> clipPolys(const std::vector<Poly>& polys, const Vector& base, const Vector& normal);
        bool clipBrush(UModel& model, const Vector& base, const Vector& normal);
        bool clipPlane(UObject* brush, const std::vector<Vector>& markers, int viewAxis, Vector& base, Vector& normal);
        void showKey(UObject* mover, int key);
        void moverMoved(UObject* mover);
    }
}
