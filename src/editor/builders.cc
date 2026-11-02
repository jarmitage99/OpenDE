#include "editor/editor.h"

namespace dx::editor {
    namespace {
        const int ShapeGrids[5] = { 4, 8, 16, 32, 64 };

        ImVec2 shapeToScreen(const dx::shapeeditor::Point& point, const ImVec2& center, float zoom)
        {
            return ImVec2(center.x + static_cast<float>(point.x) * zoom, center.y - static_cast<float>(point.y) * zoom);
        }

        dx::shapeeditor::Point screenToShape(const ImVec2& screen, const ImVec2& center, float zoom)
        {
            return { (screen.x - center.x) / zoom, (center.y - screen.y) / zoom };
        }

        float distance(const ImVec2& a, const ImVec2& b)
        {
            return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
        }

        void copyText(std::array<char, 256>& target, const std::string& text)
        {
            std::snprintf(target.data(), target.size(), "%s", text.c_str());
        }
    }

    bool Editor::applyBuilder(const std::string& label, const std::function<bool(dx::UObject*, std::string*)>& build)
    {
        dx::UObject* builder = builderBrush();
        if (!builder) {
            status = "no builder brush";
            return false;
        }
        dx::UModel* brush = dx::cast<dx::UModel>(dx::actor::Brush(builder));
        history.begin(level, { builder, brush, brush->polys }, label);
        std::string error;
        bool done = build(builder, &error);
        if (done) {
            history.commit();
        } else {
            history.cancel();
        }
        brushCache.erase(builder);
        status = done ? "built " + label : error;
        return done;
    }

    void Editor::builderMenuItems()
    {
        ImGui::Separator();
        if (ImGui::MenuItem("Add Special...")) {
            command("addspecialdialog 1");
        }
        if (ImGui::BeginMenu("Advanced")) {
            for (const dx::specialbrush::Preset& zone : dx::specialbrush::advancedZones()) {
                if (ImGui::MenuItem(zone.name.c_str())) {
                    command("addzone " + zone.name);
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Height Map...")) {
            command("heightmapdialog 1");
        }
    }

    void Editor::builderToolsMenuItems()
    {
        ImGui::MenuItem("2D Shape Editor", nullptr, &showShapeEditor);
        ImGui::MenuItem("Floor Lofter", nullptr, &showFloorLofter);
    }

    void Editor::builderWindows()
    {
        shapeEditorWindow();
        floorLofterWindow();
        heightMapWindow();
        addSpecialWindow();
    }

    bool Editor::builderCommand(const std::string& verb, const std::string& rest)
    {
        std::istringstream input(rest);
        if (verb == "shapeeditor") {
            showShapeEditor = rest != "0";
            return true;
        }
        if (verb == "floorlofter") {
            showFloorLofter = rest != "0";
            return true;
        }
        if (verb == "heightmapdialog") {
            showHeightMap = rest != "0";
            return true;
        }
        if (verb == "addspecialdialog") {
            showAddSpecial = rest != "0";
            return true;
        }
        if (verb == "addzone") {
            for (const dx::specialbrush::Preset& zone : dx::specialbrush::advancedZones()) {
                if (lower(zone.name) == lower(rest)) {
                    bool done = command("csgadd " + std::to_string(zone.flags));
                    if (done) {
                        status = zone.name + " added; it takes effect the next time geometry is rebuilt";
                    }
                    return done;
                }
            }
            status = "no advanced zone " + rest;
            return false;
        }
        if (verb == "heightmap") {
            std::string file;
            double cell = heightMapCell;
            double scale = heightMapScale;
            double base = heightMapBase;
            input >> file >> cell >> scale >> base;
            return applyBuilder("Height Map", [&](dx::UObject* builder, std::string* error) {
                dx::brushbuilder::GreyImage image;
                if (!dx::brushbuilder::loadGreyImage(file, image, error)) {
                    return false;
                }
                return dx::brushbuilder::buildFloor(dx::brushbuilder::heightMapGrid(image, cell, scale, base), builder, error);
            });
        }
        if (verb == "floor") {
            return floorCommand(input);
        }
        if (verb == "shape") {
            return shapeCommand(input);
        }
        return false;
    }

    bool Editor::floorCommand(std::istringstream& input)
    {
        std::string action;
        input >> action;
        dx::brushbuilder::resizeFloor(floorGrid);
        if (action == "new") {
            dx::brushbuilder::FloorGrid grid;
            input >> grid.sizeX >> grid.sizeY >> grid.cellSize;
            grid.sizeX = std::clamp(grid.sizeX, 1, dx::brushbuilder::FloorMaxCells);
            grid.sizeY = std::clamp(grid.sizeY, 1, dx::brushbuilder::FloorMaxCells);
            grid.renderScale = floorGrid.renderScale;
            grid.baseDepth = floorGrid.baseDepth;
            dx::brushbuilder::resizeFloor(grid);
            floorGrid = grid;
            floorRow = 0;
            return true;
        }
        if (action == "height" || action == "reverse") {
            int column = -1;
            int row = -1;
            double value = 0;
            input >> column >> row >> value;
            int columns = action == "height" ? floorGrid.sizeX + 1 : floorGrid.sizeX;
            int rows = action == "height" ? floorGrid.sizeY + 1 : floorGrid.sizeY;
            if (column < 0 || row < 0 || column >= columns || row >= rows) {
                status = "no floor " + std::string(action == "height" ? "vertex" : "cell") + " there";
                return false;
            }
            if (action == "height") {
                dx::brushbuilder::floorHeight(floorGrid, column, row) = value;
            } else {
                uint8_t& flag = floorGrid.reversed[static_cast<size_t>(row * floorGrid.sizeX + column)];
                flag = flag ? 0 : 1;
            }
            return true;
        }
        if (action == "base") {
            input >> floorGrid.baseDepth;
            return true;
        }
        if (action == "scale") {
            input >> floorGrid.renderScale;
            return true;
        }
        if (action == "row") {
            input >> floorRow;
            floorRow = std::clamp(floorRow, 0, floorGrid.sizeY);
            return true;
        }
        if (action == "heightmap") {
            std::string file;
            double scale = 16;
            input >> file >> scale;
            dx::brushbuilder::GreyImage image;
            std::string error;
            if (!dx::brushbuilder::loadGreyImage(file, image, &error)) {
                status = error;
                return false;
            }
            dx::brushbuilder::applyHeightMap(floorGrid, image, scale);
            status = "applied height map " + file;
            return true;
        }
        if (action == "build") {
            return applyBuilder("Floor Lofter", [&](dx::UObject* builder, std::string* error) {
                return dx::brushbuilder::buildFloor(floorGrid, builder, error);
            });
        }
        status = "unknown floor command " + action;
        return false;
    }

    bool Editor::shapeCommand(std::istringstream& input)
    {
        std::string action;
        input >> action;
        auto vertexAt = [&](size_t& shape, size_t& vertex) {
            input >> shape >> vertex;
            if (shape >= shapeDocument.shapes.size() || vertex >= shapeDocument.shapes[shape].vertices.size()) {
                status = "no shape vertex there";
                return false;
            }
            return true;
        };
        size_t shape = 0;
        size_t vertex = 0;
        if (action == "new") {
            shapeDocument = dx::shapeeditor::newDocument();
            shapeSelection.clear();
            shapeFile.fill(0);
            return true;
        }
        if (action == "addshape") {
            shapeDocument.shapes.push_back(dx::shapeeditor::defaultShape());
            return true;
        }
        if (action == "move" || action == "insert") {
            double x = 0;
            double y = 0;
            if (!vertexAt(shape, vertex)) {
                return false;
            }
            input >> x >> y;
            dx::shapeeditor::Point point = dx::shapeeditor::snap({ x, y }, shapeGrid);
            std::vector<dx::shapeeditor::Point>& vertices = shapeDocument.shapes[shape].vertices;
            if (action == "move") {
                vertices[vertex] = point;
            } else {
                vertices.insert(vertices.begin() + static_cast<std::ptrdiff_t>(vertex + 1), point);
            }
            shapeSelection.clear();
            return true;
        }
        if (action == "split") {
            if (!vertexAt(shape, vertex)) {
                return false;
            }
            dx::shapeeditor::splitSide(shapeDocument.shapes[shape], vertex, shapeGrid);
            shapeSelection.clear();
            return true;
        }
        if (action == "delete") {
            if (!vertexAt(shape, vertex)) {
                return false;
            }
            if (!dx::shapeeditor::deleteVertex(shapeDocument.shapes[shape], vertex)) {
                status = "a shape needs at least 3 vertices";
                return false;
            }
            shapeSelection.clear();
            return true;
        }
        if (action == "deleteshape") {
            input >> shape;
            if (shape >= shapeDocument.shapes.size()) {
                status = "no shape " + std::to_string(shape);
                return false;
            }
            shapeDocument.shapes.erase(shapeDocument.shapes.begin() + static_cast<std::ptrdiff_t>(shape));
            shapeSelection.clear();
            return true;
        }
        if (action == "select") {
            if (!vertexAt(shape, vertex)) {
                return false;
            }
            shapeSelection.push_back({ shape, vertex });
            return true;
        }
        if (action == "origin") {
            double x = 0;
            double y = 0;
            input >> x >> y;
            shapeDocument.origin = dx::shapeeditor::snap({ x, y }, shapeGrid);
            return true;
        }
        if (action == "grid") {
            input >> shapeGrid;
            shapeGrid = std::max(shapeGrid, 1);
            return true;
        }
        if (action == "flip") {
            std::string axis;
            input >> axis;
            dx::shapeeditor::flip(shapeDocument, axis == "h");
            shapeSelection.clear();
            return true;
        }
        if (action == "save" || action == "load") {
            std::string file;
            std::getline(input >> std::ws, file);
            std::string error;
            bool done = action == "save" ? dx::shapeeditor::save(shapeDocument, file, &error) : dx::shapeeditor::load(shapeDocument, file, &error);
            status = done ? action + "d " + file : error;
            if (done) {
                copyText(shapeFile, file);
                shapeSelection.clear();
            }
            return done;
        }
        if (action == "sheet") {
            return applyBuilder("2D Sheet", [&](dx::UObject* builder, std::string* error) {
                return dx::shapeeditor::sheet(shapeDocument, builder, error);
            });
        }
        if (action == "extrude") {
            input >> shapeDepth;
            return applyBuilder("2D Extrude", [&](dx::UObject* builder, std::string* error) {
                return dx::shapeeditor::extrude(shapeDocument, shapeDepth, builder, error);
            });
        }
        if (action == "extrudepoint") {
            input >> shapePointDepth;
            return applyBuilder("2D Extrude to Point", [&](dx::UObject* builder, std::string* error) {
                return dx::shapeeditor::extrudeToPoint(shapeDocument, shapePointDepth, builder, error);
            });
        }
        if (action == "bevel") {
            input >> shapeBevelDepth >> shapeBevelCap;
            return applyBuilder("2D Extrude to Bevel", [&](dx::UObject* builder, std::string* error) {
                return dx::shapeeditor::extrudeToBevel(shapeDocument, shapeBevelDepth, shapeBevelCap, builder, error);
            });
        }
        if (action == "revolve") {
            input >> shapeRevolveTotal >> shapeRevolveSides;
            return applyBuilder("2D Revolve", [&](dx::UObject* builder, std::string* error) {
                return dx::shapeeditor::revolve(shapeDocument, shapeRevolveTotal, shapeRevolveSides, builder, error);
            });
        }
        status = "unknown shape command " + action;
        return false;
    }

    void Editor::shapeEditorWindow()
    {
        if (!showShapeEditor) {
            return;
        }
        ImGui::SetNextWindowSize(ImVec2(720, 560), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(300, 90), ImGuiCond_FirstUseEver);
        std::string title = "2D Shape Editor";
        if (shapeFile[0]) {
            title += std::string(" - [") + shapeFile.data() + "]";
        }
        if (!ImGui::Begin((title + "###shapeEditor").c_str(), &showShapeEditor)) {
            ImGui::End();
            return;
        }
        if (ImGui::Button("New")) {
            command("shape new");
        }
        ImGui::SameLine();
        if (ImGui::Button("New Shape")) {
            command("shape addshape");
        }
        ImGui::SameLine();
        if (ImGui::Button("Flip H")) {
            command("shape flip h");
        }
        ImGui::SameLine();
        if (ImGui::Button("Flip V")) {
            command("shape flip v");
        }
        ImGui::SameLine();
        bool any = !shapeSelection.empty();
        ImGui::BeginDisabled(!any);
        if (ImGui::Button("Split Side")) {
            splitSelectedSides();
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete")) {
            deleteSelectedVertices();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::SetNextItemWidth(70);
        if (ImGui::BeginCombo("Grid", std::to_string(shapeGrid).c_str())) {
            for (int grid : ShapeGrids) {
                if (ImGui::Selectable(std::to_string(grid).c_str(), grid == shapeGrid)) {
                    command("shape grid " + std::to_string(grid));
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SetNextItemWidth(260);
        ImGui::InputText("##shapeFile", shapeFile.data(), shapeFile.size());
        ImGui::SameLine();
        if (ImGui::Button("Open") && shapeFile[0]) {
            command(std::string("shape load ") + shapeFile.data());
        }
        ImGui::SameLine();
        if (ImGui::Button("Save") && shapeFile[0]) {
            command(std::string("shape save ") + shapeFile.data());
        }
        if (ImGui::Button("Sheet")) {
            command("shape sheet");
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::InputInt("##extrudeDepth", &shapeDepth, 0);
        ImGui::SameLine();
        if (ImGui::Button("Extrude")) {
            command("shape extrude " + std::to_string(shapeDepth));
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::InputInt("##pointDepth", &shapePointDepth, 0);
        ImGui::SameLine();
        if (ImGui::Button("Extrude to Point")) {
            command("shape extrudepoint " + std::to_string(shapePointDepth));
        }
        ImGui::SetNextItemWidth(60);
        ImGui::InputInt("Depth##bevel", &shapeBevelDepth, 0);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60);
        ImGui::InputInt("Cap##bevel", &shapeBevelCap, 0);
        ImGui::SameLine();
        if (ImGui::Button("Extrude to Bevel")) {
            command("shape bevel " + std::to_string(shapeBevelDepth) + " " + std::to_string(shapeBevelCap));
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50);
        ImGui::InputInt("Per 360##revolve", &shapeRevolveTotal, 0);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50);
        ImGui::InputInt("Sides##revolve", &shapeRevolveSides, 0);
        ImGui::SameLine();
        if (ImGui::Button("Revolve")) {
            command("shape revolve " + std::to_string(shapeRevolveTotal) + " " + std::to_string(shapeRevolveSides));
        }
        shapeCanvas();
        ImGui::End();
    }

    void Editor::splitSelectedSides()
    {
        std::vector<std::pair<size_t, size_t>> picks = shapeSelection;
        std::sort(picks.rbegin(), picks.rend());
        for (const auto& [shape, vertex] : picks) {
            command("shape split " + std::to_string(shape) + " " + std::to_string(vertex));
        }
    }

    void Editor::deleteSelectedVertices()
    {
        std::vector<std::pair<size_t, size_t>> picks = shapeSelection;
        std::sort(picks.rbegin(), picks.rend());
        for (const auto& [shape, vertex] : picks) {
            command("shape delete " + std::to_string(shape) + " " + std::to_string(vertex));
        }
    }

    void Editor::shapeCanvas()
    {
        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImVec2 size = ImGui::GetContentRegionAvail();
        size.x = std::max(size.x, 64.0f);
        size.y = std::max(size.y, 64.0f);
        ImGui::InvisibleButton("shapeCanvas", size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
        bool hovered = ImGui::IsItemHovered();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 end(origin.x + size.x, origin.y + size.y);
        ImVec2 center(origin.x + size.x / 2 + shapePan.x, origin.y + size.y / 2 + shapePan.y);
        draw->PushClipRect(origin, end, true);
        draw->AddRectFilled(origin, end, IM_COL32(24, 24, 28, 255));
        float step = static_cast<float>(shapeGrid) * shapeZoom;
        while (step < 6.0f) {
            step *= 2;
        }
        float startX = center.x - std::floor((center.x - origin.x) / step) * step;
        for (float x = startX; x < end.x; x += step) {
            draw->AddLine(ImVec2(x, origin.y), ImVec2(x, end.y), IM_COL32(48, 48, 56, 255));
        }
        float startY = center.y - std::floor((center.y - origin.y) / step) * step;
        for (float y = startY; y < end.y; y += step) {
            draw->AddLine(ImVec2(origin.x, y), ImVec2(end.x, y), IM_COL32(48, 48, 56, 255));
        }
        draw->AddLine(ImVec2(center.x, origin.y), ImVec2(center.x, end.y), IM_COL32(80, 80, 96, 255));
        draw->AddLine(ImVec2(origin.x, center.y), ImVec2(end.x, center.y), IM_COL32(80, 80, 96, 255));
        auto isSelected = [&](size_t shape, size_t vertex) {
            return std::find(shapeSelection.begin(), shapeSelection.end(), std::make_pair(shape, vertex)) != shapeSelection.end();
        };
        for (size_t s = 0; s < shapeDocument.shapes.size(); s++) {
            const std::vector<dx::shapeeditor::Point>& vertices = shapeDocument.shapes[s].vertices;
            for (size_t v = 0; v < vertices.size(); v++) {
                ImVec2 a = shapeToScreen(vertices[v], center, shapeZoom);
                ImVec2 b = shapeToScreen(vertices[(v + 1) % vertices.size()], center, shapeZoom);
                bool active = isSelected(s, v);
                draw->AddLine(a, b, active ? IM_COL32(255, 220, 80, 255) : IM_COL32(220, 220, 220, 255), 1.5f);
            }
            for (size_t v = 0; v < vertices.size(); v++) {
                ImVec2 p = shapeToScreen(vertices[v], center, shapeZoom);
                ImU32 color = isSelected(s, v) ? IM_COL32(255, 220, 80, 255) : IM_COL32(120, 200, 255, 255);
                draw->AddRectFilled(ImVec2(p.x - 3, p.y - 3), ImVec2(p.x + 3, p.y + 3), color);
            }
        }
        ImVec2 pivot = shapeToScreen(shapeDocument.origin, center, shapeZoom);
        ImU32 pivotColor = shapeOriginSelected ? IM_COL32(255, 220, 80, 255) : IM_COL32(255, 80, 80, 255);
        draw->AddLine(ImVec2(pivot.x - 6, pivot.y), ImVec2(pivot.x + 6, pivot.y), pivotColor, 2.0f);
        draw->AddLine(ImVec2(pivot.x, pivot.y - 6), ImVec2(pivot.x, pivot.y + 6), pivotColor, 2.0f);
        draw->PopClipRect();
        ImVec2 mouse = ImGui::GetIO().MousePos;
        dx::shapeeditor::Point world = screenToShape(mouse, center, shapeZoom);
        if (hovered) {
            ImGui::SetTooltip("%.0f, %.0f", dx::shapeeditor::snap(world, shapeGrid).x, dx::shapeeditor::snap(world, shapeGrid).y);
            float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0) {
                shapeZoom = std::clamp(shapeZoom * (wheel > 0 ? 1.25f : 0.8f), 0.05f, 16.0f);
            }
        }
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            bool additive = ImGui::GetIO().KeyCtrl;
            std::pair<size_t, size_t> hit { SIZE_MAX, SIZE_MAX };
            for (size_t s = 0; s < shapeDocument.shapes.size(); s++) {
                for (size_t v = 0; v < shapeDocument.shapes[s].vertices.size(); v++) {
                    if (distance(shapeToScreen(shapeDocument.shapes[s].vertices[v], center, shapeZoom), mouse) < 7.0f) {
                        hit = { s, v };
                    }
                }
            }
            bool originHit = hit.first == SIZE_MAX && distance(pivot, mouse) < 8.0f;
            if (!additive && !(hit.first != SIZE_MAX && isSelected(hit.first, hit.second))) {
                shapeSelection.clear();
                shapeOriginSelected = false;
            }
            if (hit.first != SIZE_MAX && !isSelected(hit.first, hit.second)) {
                shapeSelection.push_back(hit);
            }
            shapeOriginSelected = shapeOriginSelected || originHit;
            shapeDragStart = world;
            shapeDragOrigins.clear();
            for (const auto& [s, v] : shapeSelection) {
                shapeDragOrigins.push_back(shapeDocument.shapes[s].vertices[v]);
            }
            shapeDragPivot = shapeDocument.origin;
            shapeDragging = hit.first != SIZE_MAX || originHit;
        }
        if (shapeDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            double dx = world.x - shapeDragStart.x;
            double dy = world.y - shapeDragStart.y;
            for (size_t i = 0; i < shapeSelection.size() && i < shapeDragOrigins.size(); i++) {
                const auto& [s, v] = shapeSelection[i];
                shapeDocument.shapes[s].vertices[v] = dx::shapeeditor::snap({ shapeDragOrigins[i].x + dx, shapeDragOrigins[i].y + dy }, shapeGrid);
            }
            if (shapeOriginSelected) {
                shapeDocument.origin = dx::shapeeditor::snap({ shapeDragPivot.x + dx, shapeDragPivot.y + dy }, shapeGrid);
            }
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            shapeDragging = false;
        }
        if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            shapePan.x += delta.x;
            shapePan.y += delta.y;
        }
        if (hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
            shapeContextPoint = dx::shapeeditor::snap(world, shapeGrid);
            ImGui::OpenPopup("shapeContext");
        }
        if (ImGui::BeginPopup("shapeContext")) {
            bool any = !shapeSelection.empty();
            if (ImGui::MenuItem("Split Side(s)", nullptr, false, any)) {
                splitSelectedSides();
            }
            if (ImGui::MenuItem("Delete", nullptr, false, any)) {
                deleteSelectedVertices();
            }
            if (ImGui::MenuItem("Set Origin Here")) {
                command("shape origin " + std::to_string(shapeContextPoint.x) + " " + std::to_string(shapeContextPoint.y));
            }
            if (ImGui::MenuItem("Zoom to Fit")) {
                shapeZoom = 1.0f;
                shapePan = ImVec2(0, 0);
            }
            ImGui::EndPopup();
        }
    }

    void Editor::floorLofterWindow()
    {
        if (!showFloorLofter) {
            return;
        }
        dx::brushbuilder::resizeFloor(floorGrid);
        ImGui::SetNextWindowSize(ImVec2(560, 440), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(760, 90), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Floor Lofter", &showFloorLofter)) {
            ImGui::End();
            return;
        }
        ImGui::SetNextItemWidth(80);
        ImGui::InputInt("Grid X", &floorNewSize[0]);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::InputInt("Grid Y", &floorNewSize[1]);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::InputInt("Cell Size", &floorNewSize[2], 0);
        ImGui::SameLine();
        if (ImGui::Button("New")) {
            command("floor new " + std::to_string(floorNewSize[0]) + " " + std::to_string(floorNewSize[1]) + " " + std::to_string(floorNewSize[2]));
        }
        ImGui::SetNextItemWidth(80);
        ImGui::InputDouble("Render Scale", &floorGrid.renderScale);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::InputDouble("Base Depth", &floorGrid.baseDepth);
        ImGui::SetNextItemWidth(200);
        ImGui::SliderInt("Level", &floorRow, 0, floorGrid.sizeY);
        floorRow = std::clamp(floorRow, 0, floorGrid.sizeY);
        if (floorRow < floorGrid.sizeY) {
            ImGui::TextUnformatted("Reverse triangles:");
            for (int column = 0; column < floorGrid.sizeX; column++) {
                ImGui::SameLine();
                bool reversed = floorGrid.reversed[static_cast<size_t>(floorRow * floorGrid.sizeX + column)] != 0;
                ImGui::PushID(column);
                if (ImGui::Checkbox("##reverse", &reversed)) {
                    command("floor reverse " + std::to_string(column) + " " + std::to_string(floorRow));
                }
                ImGui::PopID();
            }
        }
        ImGui::SetNextItemWidth(260);
        ImGui::InputText("##floorImage", floorImage.data(), floorImage.size());
        ImGui::SameLine();
        if (ImGui::Button("Get Height Map") && floorImage[0]) {
            command(std::string("floor heightmap ") + floorImage.data() + " 16");
        }
        ImGui::SameLine();
        if (ImGui::Button("Build")) {
            command("floor build");
        }
        ImVec2 origin = ImGui::GetCursorScreenPos();
        ImVec2 size = ImGui::GetContentRegionAvail();
        size.x = std::max(size.x, 64.0f);
        size.y = std::max(size.y, 64.0f);
        ImGui::InvisibleButton("floorCanvas", size);
        bool hovered = ImGui::IsItemHovered();
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 end(origin.x + size.x, origin.y + size.y);
        draw->AddRectFilled(origin, end, IM_COL32(24, 24, 28, 255));
        double width = floorGrid.sizeX * floorGrid.cellSize;
        float zoom = (size.x - 40) / static_cast<float>(std::max(width, 1.0));
        ImVec2 center(origin.x + size.x / 2, origin.y + size.y / 2);
        auto toScreen = [&](int column, double height) {
            double x = column * floorGrid.cellSize - width / 2;
            return ImVec2(center.x + static_cast<float>(x) * zoom, center.y - static_cast<float>(height) * zoom);
        };
        float baseY = center.y + static_cast<float>(floorGrid.baseDepth) * zoom;
        draw->AddLine(ImVec2(origin.x, baseY), ImVec2(end.x, baseY), IM_COL32(200, 120, 60, 255), 1.5f);
        draw->AddLine(ImVec2(origin.x, center.y), ImVec2(end.x, center.y), IM_COL32(60, 60, 72, 255));
        for (int column = 0; column <= floorGrid.sizeX; column++) {
            ImVec2 p = toScreen(column, dx::brushbuilder::floorHeight(floorGrid, column, floorRow));
            draw->AddLine(ImVec2(p.x, p.y), ImVec2(p.x, baseY), IM_COL32(70, 70, 84, 255));
            if (column < floorGrid.sizeX) {
                ImVec2 q = toScreen(column + 1, dx::brushbuilder::floorHeight(floorGrid, column + 1, floorRow));
                draw->AddLine(p, q, IM_COL32(220, 220, 220, 255), 1.5f);
            }
            ImU32 color = column == floorDragVertex ? IM_COL32(255, 220, 80, 255) : IM_COL32(120, 200, 255, 255);
            draw->AddRectFilled(ImVec2(p.x - 3, p.y - 3), ImVec2(p.x + 3, p.y + 3), color);
        }
        ImVec2 mouse = ImGui::GetIO().MousePos;
        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            floorDragVertex = -1;
            for (int column = 0; column <= floorGrid.sizeX; column++) {
                if (distance(toScreen(column, dx::brushbuilder::floorHeight(floorGrid, column, floorRow)), mouse) < 8.0f) {
                    floorDragVertex = column;
                }
            }
        }
        if (floorDragVertex >= 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            double height = (center.y - mouse.y) / zoom;
            height = std::round(height / 8.0) * 8.0;
            dx::brushbuilder::floorHeight(floorGrid, floorDragVertex, floorRow) = height;
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            floorDragVertex = -1;
        }
        ImGui::End();
    }

    void Editor::heightMapWindow()
    {
        if (!showHeightMap) {
            return;
        }
        ImGui::SetNextWindowSize(ImVec2(380, 180), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(340, 640), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Height Map", &showHeightMap)) {
            ImGui::End();
            return;
        }
        ImGui::TextWrapped("Builds a terrain brush from a greyscale BMP or PCX image: one vertex per pixel.");
        ImGui::InputText("Image", heightMapFile.data(), heightMapFile.size());
        ImGui::InputDouble("Step Size", &heightMapCell);
        ImGui::InputDouble("Height Scale", &heightMapScale);
        ImGui::InputDouble("Base Depth", &heightMapBase);
        if (ImGui::Button("Build") && heightMapFile[0]) {
            command(std::string("heightmap ") + heightMapFile.data() + " " + std::to_string(heightMapCell) + " " + std::to_string(heightMapScale) + " " + std::to_string(heightMapBase));
        }
        ImGui::End();
    }

    void Editor::addSpecialWindow()
    {
        if (!showAddSpecial) {
            return;
        }
        ImGui::SetNextWindowSize(ImVec2(340, 300), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(760, 560), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Add Special Brush", &showAddSpecial)) {
            ImGui::End();
            return;
        }
        if (ImGui::BeginCombo("Predefined", specialPresets[specialChoice].name.c_str())) {
            for (size_t i = 0; i < specialPresets.size(); i++) {
                if (ImGui::Selectable(specialPresets[i].name.c_str(), i == specialChoice)) {
                    specialChoice = i;
                    specialOptions = specialPresets[i].options;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SeparatorText("Masking");
        if (ImGui::RadioButton("Regular", !specialOptions.masked)) {
            specialOptions.masked = false;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Masked", specialOptions.masked)) {
            specialOptions.masked = true;
        }
        ImGui::SeparatorText("Effects");
        ImGui::Checkbox("Invisible", &specialOptions.invisible);
        ImGui::SameLine();
        ImGui::Checkbox("Zone Portal", &specialOptions.zonePortal);
        ImGui::SameLine();
        ImGui::Checkbox("Transparent", &specialOptions.transparent);
        ImGui::SeparatorText("Solidity");
        const char* solidity[3] = { "Solid", "Semi-solid", "Non-solid" };
        for (int i = 0; i < 3; i++) {
            if (i > 0) {
                ImGui::SameLine();
            }
            if (ImGui::RadioButton(solidity[i], static_cast<int>(specialOptions.solidity) == i)) {
                specialOptions.solidity = static_cast<dx::specialbrush::Solidity>(i);
            }
        }
        ImGui::SeparatorText("Visibility");
        if (ImGui::RadioButton("1-Sided", !specialOptions.twoSided)) {
            specialOptions.twoSided = false;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("2-Sided", specialOptions.twoSided)) {
            specialOptions.twoSided = true;
        }
        uint32_t flags = dx::specialbrush::flags(specialOptions);
        ImGui::Text("Poly flags 0x%08x", flags);
        if (ImGui::Button("Add Special")) {
            command("csgadd " + std::to_string(flags));
        }
        ImGui::End();
    }
}
