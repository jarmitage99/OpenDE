#include "compiler/asset_mesh_lod.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace dx::compiler {
    namespace {
        constexpr uint32_t PolyTwoSided = 0x00000100;
        constexpr uint32_t PolyTranslucent = 0x00000004;
        constexpr uint32_t PolyInvisible = 0x00000001;
        constexpr int32_t IndexNone = -1;

        template <typename T>
        int32_t findItemIndex(const std::vector<T>& items, const T& item)
        {
            for (size_t i = 0; i < items.size(); i++) {
                if (items[i] == item) {
                    return static_cast<int32_t>(i);
                }
            }
            return IndexNone;
        }

        template <typename T>
        int32_t addUniqueItem(std::vector<T>& items, const T& item)
        {
            int32_t found = findItemIndex(items, item);
            if (found != IndexNone) {
                return found;
            }
            items.push_back(item);
            return static_cast<int32_t>(items.size()) - 1;
        }

        template <typename T>
        void removeItem(std::vector<T>& items, const T& item)
        {
            for (size_t i = 0; i < items.size(); i++) {
                if (items[i] == item) {
                    items.erase(items.begin() + static_cast<std::ptrdiff_t>(i));
                    i--;
                }
            }
        }

        template <typename T>
        void removeAt(std::vector<T>& items, int32_t index)
        {
            items.erase(items.begin() + index);
        }

        struct FVec {
            double x = 0;
            double y = 0;
            double z = 0;
        };

        double roundFloat(double value)
        {
            return static_cast<double>(static_cast<float>(value));
        }

        struct TVertex;
        struct TFace;

        struct TWedge {
            int32_t originalIndex = 0;
            int32_t vertexIndex = 0;
            int32_t destinationIndex = 0;
            uint8_t u = 0;
            uint8_t v = 0;

            uint16_t w() const
            {
                return static_cast<uint16_t>(u | (v << 8));
            }

            void setW(uint16_t value)
            {
                u = static_cast<uint8_t>(value & 0xff);
                v = static_cast<uint8_t>(value >> 8);
            }
        };

        struct TVertex {
            FVec point;
            int32_t originalIndex = 0;
            std::vector<TVertex*> verts;
            std::vector<TFace*> faces;
            std::vector<TWedge*> wedges;
            int32_t flag = 0;
            float mergeCost = 0;
            TVertex* colTarget = nullptr;
            bool alive = true;
        };

        struct TFace {
            int32_t originalIndex = 0;
            TVertex* verts[3] = {};
            TWedge* wedges[3] = {};
            int32_t materialId = 0;
            FVec normal;
        };

        class TModel {
        public:
            std::vector<TFace*> faces;
            std::vector<TVertex*> verts;
            std::vector<TWedge> wedges;
            std::vector<TWedge> auxWedges;
            std::vector<int32_t> originalVertIdx;
            std::vector<uint16_t> faceLevel;
            std::vector<MeshMaterial> materials;
            std::vector<std::unique_ptr<TVertex>> vertexStore;
            std::vector<std::unique_ptr<TFace>> faceStore;
            int32_t style = 0;
            int32_t staleTargets = 0;

            TVertex* newVertex(FVec point, int32_t originalIndex, int32_t flag)
            {
                vertexStore.push_back(std::make_unique<TVertex>());
                TVertex* vertex = vertexStore.back().get();
                vertex->point = point;
                vertex->originalIndex = originalIndex;
                vertex->flag = flag;
                verts.push_back(vertex);
                return vertex;
            }

            static bool hasVertex(const TFace* face, const TVertex* vertex)
            {
                return vertex == face->verts[0] || vertex == face->verts[1] || vertex == face->verts[2];
            }

            static int32_t vertexIndex(const TFace* face, const TVertex* vertex)
            {
                if (vertex == face->verts[0]) {
                    return 0;
                }
                if (vertex == face->verts[1]) {
                    return 1;
                }
                if (vertex == face->verts[2]) {
                    return 2;
                }
                return 0;
            }

            static void computeNormal(TFace* face)
            {
                FVec v0 = face->verts[0]->point;
                FVec v1 = face->verts[1]->point;
                FVec v2 = face->verts[2]->point;
                FVec a = { v1.x - v0.x, v1.y - v0.y, v1.z - v0.z };
                FVec b = { v2.x - v1.x, v2.y - v1.y, v2.z - v1.z };
                FVec normal = { roundFloat(a.y * b.z - a.z * b.y), roundFloat(a.z * b.x - a.x * b.z), roundFloat(a.x * b.y - a.y * b.x) };
                double sizeSquared = normal.z * normal.z + normal.x * normal.x;
                sizeSquared = sizeSquared + normal.y * normal.y;
                double reciprocal = 1.0 / std::sqrt(sizeSquared);
                face->normal = { roundFloat(reciprocal * normal.x), roundFloat(reciprocal * normal.y), roundFloat(reciprocal * normal.z) };
            }

            void newFace(int32_t material, TWedge* w0, TWedge* w1, TWedge* w2, int32_t startIndex)
            {
                faceStore.push_back(std::make_unique<TFace>());
                TFace* face = faceStore.back().get();
                face->materialId = material;
                face->originalIndex = startIndex;
                face->wedges[0] = w0;
                face->wedges[1] = w1;
                face->wedges[2] = w2;
                face->verts[0] = verts[static_cast<size_t>(w0->vertexIndex)];
                face->verts[1] = verts[static_cast<size_t>(w1->vertexIndex)];
                face->verts[2] = verts[static_cast<size_t>(w2->vertexIndex)];
                if (face->verts[0] == face->verts[1] || face->verts[1] == face->verts[2] || face->verts[2] == face->verts[0]) {
                    return;
                }
                computeNormal(face);
                faces.push_back(face);
                for (int i = 0; i < 3; i++) {
                    face->verts[i]->faces.push_back(face);
                    face->verts[i]->wedges.push_back(face->wedges[i]);
                    for (int j = 0; j < 3; j++) {
                        if (i != j) {
                            addUniqueItem(face->verts[i]->verts, face->verts[j]);
                        }
                    }
                }
            }

            static void removeIfNonNeighbor(TVertex* vertex, TVertex* other)
            {
                if (findItemIndex(vertex->verts, other) == IndexNone) {
                    return;
                }
                for (TFace* face : vertex->faces) {
                    if (hasVertex(face, other)) {
                        return;
                    }
                }
                removeItem(vertex->verts, other);
            }

            void deleteFace(TFace* face)
            {
                removeItem(faces, face);
                faceLevel[static_cast<size_t>(face->originalIndex)] = static_cast<uint16_t>(verts.size());
                for (int i = 0; i < 3; i++) {
                    if (face->verts[i]) {
                        int32_t index = findItemIndex(face->verts[i]->faces, face);
                        removeAt(face->verts[i]->faces, index);
                        removeAt(face->verts[i]->wedges, index);
                    }
                }
                for (int i = 0; i < 3; i++) {
                    int i2 = (i + 1) % 3;
                    if (!face->verts[i] || !face->verts[i2]) {
                        continue;
                    }
                    removeIfNonNeighbor(face->verts[i], face->verts[i2]);
                    removeIfNonNeighbor(face->verts[i2], face->verts[i]);
                }
            }

            void deleteVertex(TVertex* vertex)
            {
                while (!vertex->verts.empty()) {
                    TVertex* neighbor = vertex->verts[0];
                    removeItem(neighbor->verts, vertex);
                    removeItem(vertex->verts, neighbor);
                }
                vertex->wedges.clear();
                removeItem(verts, vertex);
                vertex->alive = false;
            }

            void replaceVertex(TFace* face, TVertex* oldVertex, TVertex* newVertex)
            {
                int32_t slot = 2;
                if (oldVertex == face->verts[0]) {
                    slot = 0;
                } else if (oldVertex == face->verts[1]) {
                    slot = 1;
                }
                face->verts[slot] = newVertex;
                int32_t removeIndex = findItemIndex(oldVertex->faces, face);
                removeAt(oldVertex->faces, removeIndex);
                TWedge* oldWedge = oldVertex->wedges[static_cast<size_t>(removeIndex)];
                removeAt(oldVertex->wedges, removeIndex);
                newVertex->faces.push_back(face);
                newVertex->wedges.push_back(oldWedge);
                for (int i = 0; i < 3; i++) {
                    removeIfNonNeighbor(oldVertex, face->verts[i]);
                    removeIfNonNeighbor(face->verts[i], oldVertex);
                }
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        if (i != j) {
                            addUniqueItem(face->verts[i]->verts, face->verts[j]);
                        }
                    }
                }
                computeNormal(face);
            }

            static bool isBorder(const TVertex* vertex)
            {
                for (TVertex* neighbor : vertex->verts) {
                    int count = 0;
                    for (TFace* face : vertex->faces) {
                        if (hasVertex(face, neighbor)) {
                            count++;
                        }
                    }
                    if (count == 1) {
                        return true;
                    }
                }
                return false;
            }

            float edgeCollapseCost(TVertex* u, TVertex* v)
            {
                double dx = u->point.x - v->point.x;
                double dy = u->point.y - v->point.y;
                double dz = u->point.z - v->point.z;
                double sum = dz * dz + dy * dy;
                sum = sum + dx * dx;
                double edgeLength = roundFloat(std::sqrt(sum));
                double curvature = 0.0;
                double texRespect = 1.0;
                double transBias = 1.0;
                bool translucent = false;
                bool planar = true;
                std::vector<TFace*> sideFaces;
                std::vector<TWedge*> uniqueWedges;
                for (size_t i = 0; i < u->faces.size(); i++) {
                    if (hasVertex(u->faces[i], v)) {
                        sideFaces.push_back(u->faces[i]);
                        uint32_t flags = materials[static_cast<size_t>(u->faces[i]->materialId)].polyFlags;
                        if (!(flags & PolyTwoSided)) {
                            planar = false;
                        }
                        if (flags & PolyTranslucent) {
                            translucent = true;
                        }
                    }
                }
                for (TWedge* wedge : u->wedges) {
                    addUniqueItem(uniqueWedges, wedge);
                }
                if (uniqueWedges.size() > 1) {
                    double factor = static_cast<double>((style & 4) ? 2.1f : 0.7f);
                    texRespect = roundFloat(static_cast<double>(uniqueWedges.size()) * factor);
                }
                if ((style & 16) && translucent) {
                    transBias = static_cast<double>(1.7f);
                }
                if ((style & 2) && planar) {
                    curvature = static_cast<double>(0.35f);
                }
                if (isBorder(u) && sideFaces.size() > 1) {
                    curvature = 1.0;
                } else {
                    for (TFace* face : u->faces) {
                        double minCurvature = 1.0;
                        for (TFace* side : sideFaces) {
                            double dotProduct = side->normal.z * face->normal.z + side->normal.y * face->normal.y;
                            dotProduct = dotProduct + side->normal.x * face->normal.x;
                            double candidate = roundFloat((1.0 - dotProduct) * 0.5);
                            if (minCurvature > candidate) {
                                minCurvature = candidate;
                            }
                        }
                        if (curvature < minCurvature) {
                            curvature = minCurvature;
                        }
                    }
                }
                if (style & 8) {
                    curvature = 1.0;
                }
                if (style & 1) {
                    edgeLength = 1.0;
                }
                if (style & 32) {
                    edgeLength = edgeLength * edgeLength;
                }
                return static_cast<float>(transBias * texRespect * curvature * edgeLength);
            }

            void computeEdgeCostAtVertex(TVertex* vertex)
            {
                if (vertex->verts.empty()) {
                    vertex->colTarget = nullptr;
                    vertex->mergeCost = -0.01f;
                    return;
                }
                vertex->mergeCost = 1000000.0f;
                vertex->colTarget = nullptr;
                for (TVertex* neighbor : vertex->verts) {
                    float cost = edgeCollapseCost(vertex, neighbor);
                    if (cost < vertex->mergeCost) {
                        vertex->colTarget = neighbor;
                        vertex->mergeCost = cost;
                    }
                }
            }

            void collapse(TVertex* u, TVertex* v)
            {
                if (!v) {
                    if (u) {
                        for (size_t i = u->faces.size(); i-- > 0;) {
                            deleteFace(u->faces[i]);
                        }
                        deleteVertex(u);
                    }
                    return;
                }
                std::vector<TFace*> sideFaces;
                std::vector<TWedge> sideWedges;
                std::vector<TWedge*> nonSideWedges;
                std::vector<TWedge*> uniqueNonSideWedges;
                for (size_t i = 0; i < u->faces.size(); i++) {
                    if (hasVertex(u->faces[i], v)) {
                        sideFaces.push_back(u->faces[i]);
                        sideWedges.push_back(*u->wedges[i]);
                    } else {
                        nonSideWedges.push_back(u->wedges[i]);
                    }
                }
                for (TWedge* wedge : nonSideWedges) {
                    addUniqueItem(uniqueNonSideWedges, wedge);
                }
                for (TWedge* wedge : uniqueNonSideWedges) {
                    int32_t matched = IndexNone;
                    for (size_t w = 0; w < sideWedges.size(); w++) {
                        if (wedge->w() == sideWedges[w].w()) {
                            matched = static_cast<int32_t>(w);
                        }
                    }
                    auxWedges.push_back(*wedge);
                    if (matched == IndexNone) {
                        wedge->vertexIndex = v->originalIndex;
                    } else {
                        TFace* side = sideFaces[static_cast<size_t>(matched)];
                        TWedge* destination = side->wedges[vertexIndex(side, v)];
                        wedge->setW(destination->w());
                        wedge->vertexIndex = destination->vertexIndex;
                    }
                }
                std::vector<TVertex*> neighbors = u->verts;
                for (size_t i = u->faces.size(); i-- > 0;) {
                    if (hasVertex(u->faces[i], v)) {
                        deleteFace(u->faces[i]);
                    }
                }
                for (size_t i = u->faces.size(); i-- > 0;) {
                    replaceVertex(u->faces[i], u, v);
                }
                deleteVertex(u);
                for (TVertex* neighbor : neighbors) {
                    computeEdgeCostAtVertex(neighbor);
                }
            }

            TVertex* minimumCostEdge()
            {
                TVertex* best = verts[0];
                for (TVertex* vertex : verts) {
                    if (vertex->mergeCost < best->mergeCost) {
                        best = vertex;
                    }
                }
                return best;
            }
        };
    }

    LodProcessed meshLodProcess(std::vector<MeshTri> tris, const std::vector<MeshVert>& verts, int32_t frameVerts, int32_t animFrames, const LodSettings& settings, LodTrace* trace)
    {
        LodProcessed mesh;
        TModel model;
        model.style = settings.style;
        std::vector<int32_t> tempMaterials;
        std::vector<int32_t> specialCoordVerts;
        if (settings.noUVData) {
            for (MeshTri& tri : tris) {
                for (auto& uv : tri.uv) {
                    uv[0] = 0;
                    uv[1] = 0;
                }
            }
        }
        size_t frameOffset = static_cast<size_t>(frameVerts) * static_cast<size_t>(settings.sampleFrame);
        for (int32_t t = 0; t < frameVerts; t++) {
            const MeshVert& vert = verts[frameOffset + static_cast<size_t>(t)];
            model.newVertex({ static_cast<double>(vert.x), static_cast<double>(vert.y), static_cast<double>(vert.z) }, t, 0);
        }
        for (size_t p = tris.size(); p-- > 0;) {
            if (tris[p].polyFlags & PolyInvisible) {
                MeshFace face;
                for (int k = 0; k < 3; k++) {
                    face.wedge[k] = static_cast<uint16_t>(addUniqueItem(specialCoordVerts, static_cast<int32_t>(tris[p].vertex[k])));
                }
                mesh.specialFaces.push_back(face);
                tris.erase(tris.begin() + static_cast<std::ptrdiff_t>(p));
                continue;
            }
            for (int k = 0; k < 3; k++) {
                model.verts[tris[p].vertex[k]]->flag = 0xFFFF;
            }
        }
        mesh.specialVerts = static_cast<int32_t>(specialCoordVerts.size());
        for (const MeshTri& tri : tris) {
            int32_t materialIndex = -1;
            for (size_t m = 0; m < mesh.materials.size(); m++) {
                if (mesh.materials[m].polyFlags == tri.polyFlags && mesh.materials[m].textureIndex == tri.textureIndex) {
                    materialIndex = static_cast<int32_t>(m);
                    break;
                }
            }
            if (materialIndex < 0) {
                materialIndex = static_cast<int32_t>(mesh.materials.size());
                mesh.materials.push_back({ tri.polyFlags, tri.textureIndex });
            }
            MeshFace face;
            face.material = static_cast<uint16_t>(materialIndex);
            model.faceLevel.push_back(0xFFFF);
            mesh.faces.push_back(face);
        }
        for (size_t p = 0; p < tris.size(); p++) {
            for (int t = 0; t < 3; t++) {
                bool unique = true;
                for (size_t v = 0; v < mesh.wedges.size(); v++) {
                    if (mesh.wedges[v].vertex == tris[p].vertex[t] && mesh.wedges[v].u == tris[p].uv[t][0] && mesh.wedges[v].v == tris[p].uv[t][1] && tempMaterials[v] == mesh.faces[p].material) {
                        mesh.faces[p].wedge[t] = static_cast<uint16_t>(v);
                        unique = false;
                        break;
                    }
                }
                if (unique) {
                    MeshWedge wedge;
                    wedge.vertex = tris[p].vertex[t];
                    wedge.u = tris[p].uv[t][0];
                    wedge.v = tris[p].uv[t][1];
                    mesh.wedges.push_back(wedge);
                    tempMaterials.push_back(mesh.faces[p].material);
                    mesh.faces[p].wedge[t] = static_cast<uint16_t>(mesh.wedges.size() - 1);
                }
            }
        }
        if (!specialCoordVerts.empty()) {
            for (size_t p = model.verts.size(); p-- > 0;) {
                if (model.verts[p]->flag == 0) {
                    model.verts.erase(model.verts.begin() + static_cast<std::ptrdiff_t>(p));
                }
            }
            std::vector<int32_t> vertRemap(static_cast<size_t>(frameVerts));
            for (size_t t = 0; t < model.verts.size(); t++) {
                vertRemap[static_cast<size_t>(model.verts[t]->originalIndex)] = static_cast<int32_t>(t);
            }
            for (MeshWedge& wedge : mesh.wedges) {
                wedge.vertex = static_cast<uint16_t>(vertRemap[wedge.vertex]);
            }
        }
        int32_t trueFrameVertNum = static_cast<int32_t>(model.verts.size());
        for (size_t t = 0; t < model.verts.size(); t++) {
            model.originalVertIdx.push_back(model.verts[t]->originalIndex);
            model.verts[t]->originalIndex = static_cast<int32_t>(t);
        }
        for (size_t t = 0; t < mesh.wedges.size(); t++) {
            TWedge wedge;
            wedge.u = mesh.wedges[t].u;
            wedge.v = mesh.wedges[t].v;
            wedge.vertexIndex = mesh.wedges[t].vertex;
            wedge.destinationIndex = -1;
            wedge.originalIndex = static_cast<int32_t>(t);
            model.wedges.push_back(wedge);
        }
        for (size_t t = 0; t < mesh.faces.size(); t++) {
            const MeshFace& face = mesh.faces[t];
            model.newFace(face.material, &model.wedges[face.wedge[0]], &model.wedges[face.wedge[1]], &model.wedges[face.wedge[2]], static_cast<int32_t>(t));
        }
        model.materials = mesh.materials;
        for (TVertex* vertex : model.verts) {
            model.computeEdgeCostAtVertex(vertex);
        }
        std::vector<int32_t> permutation(static_cast<size_t>(trueFrameVertNum));
        mesh.collapsePointThus.assign(static_cast<size_t>(trueFrameVertNum), 0);
        size_t step = 0;
        while (!model.verts.empty()) {
            TVertex* chosen = model.minimumCostEdge();
            if (trace && trace->forcedOrder) {
                int32_t forced = (*trace->forcedOrder)[step];
                TVertex* wanted = nullptr;
                for (TVertex* vertex : model.verts) {
                    if (model.originalVertIdx[static_cast<size_t>(vertex->originalIndex)] == forced) {
                        wanted = vertex;
                    }
                }
                if (!wanted || wanted->mergeCost != chosen->mergeCost) {
                    trace->mismatches++;
                }
                if (wanted) {
                    for (TVertex* vertex : model.verts) {
                        if (vertex != wanted && vertex->mergeCost == wanted->mergeCost) {
                            trace->precedes.emplace_back(forced, model.originalVertIdx[static_cast<size_t>(vertex->originalIndex)]);
                        }
                    }
                    chosen = wanted;
                }
            }
            step++;
            permutation[static_cast<size_t>(chosen->originalIndex)] = static_cast<int32_t>(model.verts.size()) - 1;
            TVertex* target = chosen->colTarget;
            if (target && !target->alive) {
                model.staleTargets++;
            }
            mesh.collapsePointThus[model.verts.size() - 1] = target ? static_cast<uint16_t>(target->originalIndex) : 0xFFFF;
            model.collapse(chosen, target);
        }
        if (trace) {
            trace->staleTargets = model.staleTargets;
        }
        for (uint16_t& point : mesh.collapsePointThus) {
            point = point == 0xFFFF ? 0 : static_cast<uint16_t>(permutation[point]);
        }
        for (const TWedge& wedge : model.auxWedges) {
            model.wedges.push_back(wedge);
        }
        model.auxWedges.clear();

        struct WedgeSort {
            int32_t sortedWedgeIndex = 0;
            uint32_t key = 0;
        };

        std::vector<WedgeSort> wedgePermutation(model.wedges.size());
        for (size_t w = 0; w < model.wedges.size(); w++) {
            model.wedges[w].vertexIndex = permutation[static_cast<size_t>(model.wedges[w].vertexIndex)];
            wedgePermutation[w].sortedWedgeIndex = static_cast<int32_t>(w);
            wedgePermutation[w].key = model.wedges[w].w() + (static_cast<uint32_t>(model.wedges[w].vertexIndex) << 16);
        }
        msvcQsort(wedgePermutation.data(), wedgePermutation.size(), [](const WedgeSort& a, const WedgeSort& b) {
            return static_cast<int32_t>(a.key - b.key);
        });
        std::vector<TWedge> workWedges;
        for (const WedgeSort& sorted : wedgePermutation) {
            workWedges.push_back(model.wedges[static_cast<size_t>(sorted.sortedWedgeIndex)]);
        }
        model.wedges.clear();
        std::vector<int32_t> wedgeRemapList(workWedges.size());
        std::vector<int32_t> wedgeRemapToUnique;
        std::vector<TWedge> uniqueWedges;
        for (size_t w = 0; w < workWedges.size(); w++) {
            if (w == 0) {
                uniqueWedges.push_back(workWedges[w]);
            }
            if (w > 0 && (workWedges[w].w() != workWedges[w - 1].w() || workWedges[w].vertexIndex != workWedges[w - 1].vertexIndex)) {
                uniqueWedges.push_back(workWedges[w]);
            }
            wedgeRemapToUnique.push_back(static_cast<int32_t>(uniqueWedges.size()) - 1);
            wedgeRemapList[static_cast<size_t>(workWedges[w].originalIndex)] = static_cast<int32_t>(uniqueWedges.size()) - 1;
        }
        std::vector<int32_t> wedgeFlagger(workWedges.size(), -1);
        mesh.collapseWedgeThus.resize(uniqueWedges.size());
        for (size_t w = 0; w < uniqueWedges.size(); w++) {
            mesh.collapseWedgeThus[w] = static_cast<uint16_t>(w);
        }
        for (size_t w = 0; w < workWedges.size(); w++) {
            size_t original = static_cast<size_t>(workWedges[w].originalIndex);
            int32_t uniqueIndex = wedgeRemapToUnique[w];
            int32_t oldFlag = wedgeFlagger[original];
            wedgeFlagger[original] = uniqueIndex;
            if (oldFlag != -1) {
                mesh.collapseWedgeThus[static_cast<size_t>(uniqueIndex)] = static_cast<uint16_t>(oldFlag);
            }
        }
        mesh.wedges.assign(uniqueWedges.size(), MeshWedge());
        for (size_t w = 0; w < uniqueWedges.size(); w++) {
            mesh.wedges[w].u = uniqueWedges[w].u;
            mesh.wedges[w].v = uniqueWedges[w].v;
            mesh.wedges[w].vertex = static_cast<uint16_t>(uniqueWedges[w].vertexIndex);
        }
        for (size_t f = mesh.faces.size(); f-- > 0;) {
            if (model.faceLevel[f] == 0xFFFF) {
                mesh.faces.erase(mesh.faces.begin() + static_cast<std::ptrdiff_t>(f));
                model.faceLevel.erase(model.faceLevel.begin() + static_cast<std::ptrdiff_t>(f));
            } else {
                for (uint16_t& wedge : mesh.faces[f].wedge) {
                    wedge = static_cast<uint16_t>(wedgeRemapList[wedge]);
                }
            }
        }
        mesh.faceLevel = model.faceLevel;
        int32_t fullFrameVertNum = trueFrameVertNum + static_cast<int32_t>(specialCoordVerts.size());
        std::vector<int32_t> fullRemap(static_cast<size_t>(fullFrameVertNum));
        for (size_t t = 0; t < specialCoordVerts.size(); t++) {
            fullRemap[t] = specialCoordVerts[t];
        }
        for (size_t t = 0; t < static_cast<size_t>(trueFrameVertNum); t++) {
            fullRemap[specialCoordVerts.size() + static_cast<size_t>(permutation[t])] = model.originalVertIdx[t];
        }
        mesh.verts.resize(static_cast<size_t>(fullFrameVertNum) * static_cast<size_t>(animFrames));
        for (size_t f = 0; f < static_cast<size_t>(animFrames); f++) {
            size_t frameStart = f * static_cast<size_t>(frameVerts);
            size_t fullFrameStart = f * static_cast<size_t>(fullFrameVertNum);
            for (size_t v = 0; v < static_cast<size_t>(fullFrameVertNum); v++) {
                mesh.verts[fullFrameStart + v] = verts[static_cast<size_t>(fullRemap[v]) + frameStart];
            }
        }
        std::vector<int32_t> unFullRemap(static_cast<size_t>(frameVerts));
        for (size_t t = 0; t < static_cast<size_t>(fullFrameVertNum); t++) {
            unFullRemap[static_cast<size_t>(fullRemap[t])] = static_cast<int32_t>(t);
        }
        for (MeshTri& tri : tris) {
            for (uint16_t& vertex : tri.vertex) {
                vertex = static_cast<uint16_t>(unFullRemap[vertex]);
            }
        }
        mesh.tris = std::move(tris);
        mesh.frameVerts = fullFrameVertNum;
        mesh.modelVerts = trueFrameVertNum;
        return mesh;
    }
}
