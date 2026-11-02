#include "editor/editor.h"

namespace dx::editor {
    namespace {
        const std::unordered_set<std::string>& scriptKeywords()
        {
            static const std::unordered_set<std::string> keywords = {
                "abstract", "array", "assert", "auto", "bool", "break", "byte", "case", "class", "coerce", "config", "const", "continue",
                "default", "defaultproperties", "do", "editconst", "else", "enum", "event", "exec", "expands", "export", "extends", "false",
                "final", "float", "for", "foreach", "function", "global", "globalconfig", "goto", "guid", "if", "ignores", "input", "int",
                "intrinsic", "iterator", "latent", "local", "localized", "name", "native", "nativereplication", "new", "noexport", "none",
                "nousercreate", "operator", "optional", "out", "perobjectconfig", "postoperator", "preoperator", "private", "reliable",
                "replication", "return", "safereplace", "self", "simulated", "singular", "skip", "state", "static", "stop", "string",
                "struct", "super", "switch", "transient", "travel", "true", "unreliable", "until", "var", "while", "within"
            };
            return keywords;
        }

        template <typename Emit>
        void highlightLine(const char* begin, const char* end, bool& inComment, Emit emit)
        {
            const ImU32 commentColor = IM_COL32(110, 190, 110, 255);
            const char* p = begin;
            while (p < end) {
                if (inComment) {
                    const char* close = p;
                    while (close + 1 < end && !(close[0] == '*' && close[1] == '/')) {
                        close++;
                    }
                    if (close + 1 < end) {
                        emit(p, close + 2, commentColor);
                        p = close + 2;
                        inComment = false;
                    } else {
                        emit(p, end, commentColor);
                        p = end;
                    }
                    continue;
                }
                char c = *p;
                if (c == '/' && p + 1 < end && p[1] == '/') {
                    emit(p, end, commentColor);
                    return;
                }
                if (c == '/' && p + 1 < end && p[1] == '*') {
                    inComment = true;
                    continue;
                }
                if (c == '"' || c == '\'') {
                    const char* q = p + 1;
                    while (q < end && *q != c) {
                        if (*q == '\\' && q + 1 < end) {
                            q++;
                        }
                        q++;
                    }
                    if (q < end) {
                        q++;
                    }
                    emit(p, q, c == '"' ? IM_COL32(230, 170, 100, 255) : IM_COL32(200, 160, 230, 255));
                    p = q;
                    continue;
                }
                if (std::isdigit(static_cast<unsigned char>(c))) {
                    const char* q = p;
                    while (q < end && (std::isalnum(static_cast<unsigned char>(*q)) || *q == '.')) {
                        q++;
                    }
                    emit(p, q, IM_COL32(180, 220, 140, 255));
                    p = q;
                    continue;
                }
                if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                    const char* q = p;
                    while (q < end && (std::isalnum(static_cast<unsigned char>(*q)) || *q == '_')) {
                        q++;
                    }
                    if (scriptKeywords().count(lower(std::string(p, q)))) {
                        emit(p, q, IM_COL32(110, 170, 255, 255));
                    }
                    p = q;
                    continue;
                }
                if (c == '#') {
                    bool lineStart = std::all_of(begin, p, [](char space) {
                        return space == ' ' || space == '\t';
                    });
                    if (lineStart) {
                        emit(p, end, IM_COL32(220, 130, 220, 255));
                        return;
                    }
                }
                p++;
            }
        }

        void indexScript(ScriptDocument& document)
        {
            document.lineStarts.assign(1, 0);
            for (size_t i = 0; i < document.text.size(); i++) {
                if (document.text[i] == '\n') {
                    document.lineStarts.push_back(i + 1);
                }
            }
            document.commentState.clear();
            document.width = 0;
            bool inComment = false;
            ImFont* font = ImGui::GetFont();
            float fontSize = ImGui::GetFontSize();
            const char* text = document.text.c_str();
            for (size_t line = 0; line < document.lineStarts.size(); line++) {
                document.commentState.push_back(inComment ? 1 : 0);
                const char* begin = text + document.lineStarts[line];
                const char* end = line + 1 < document.lineStarts.size() ? text + document.lineStarts[line + 1] - 1 : text + document.text.size();
                highlightLine(begin, end, inComment, [](const char*, const char*, ImU32) {
                });
                document.width = std::max(document.width, font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, begin, end).x);
            }
        }

        std::string editorText(const std::string& script)
        {
            std::string text;
            for (char c : script) {
                if (c != '\r') {
                    text += c;
                }
            }
            return text;
        }

        std::string scriptText(const std::string& text)
        {
            std::string script;
            for (char c : text) {
                if (c == '\n') {
                    script += '\r';
                }
                script += c;
            }
            return script;
        }

        int scriptInputCallback(ImGuiInputTextCallbackData* data)
        {
            ScriptDocument* document = static_cast<ScriptDocument*>(data->UserData);
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                document->text.resize(static_cast<size_t>(data->BufTextLen));
                data->Buf = document->text.data();
                return 0;
            }
            if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways) {
                if (document->jumpLine > 0) {
                    size_t line = std::min(static_cast<size_t>(document->jumpLine - 1), document->lineStarts.size() - 1);
                    int position = static_cast<int>(std::min(document->lineStarts[line], static_cast<size_t>(data->BufTextLen)));
                    data->CursorPos = position;
                    data->SelectionStart = position;
                    data->SelectionEnd = position;
                    document->jumpLine = 0;
                }
                document->cursor = data->CursorPos;
            }
            return 0;
        }
    }

    void Editor::openDocument(dx::UClass* cls, int line)
    {
        size_t index = 0;
        while (index < documents.size() && documents[index].cls != cls) {
            index++;
        }
        if (index == documents.size()) {
            ScriptDocument document;
            document.cls = cls;
            document.text = editorText(cls->scriptText->text);
            document.compiled = document.text;
            indexScript(document);
            documents.push_back(std::move(document));
        }
        if (line > 0) {
            documents[index].jumpLine = line;
            documents[index].scrollPending = true;
        }
        selectDocument = static_cast<int>(index);
        showScripts = true;
    }

    void Editor::loadDocumentText(dx::UClass* cls, int line, const std::string& script)
    {
        openDocument(cls, line);
        ScriptDocument& document = documents[static_cast<size_t>(selectDocument)];
        document.text = editorText(script);
        indexScript(document);
    }

    void Editor::refreshClasses()
    {
        buildClassTree();
        ImGui::ClearActiveID();
        for (ScriptDocument& document : documents) {
            document.text = editorText(document.cls->scriptText->text);
            document.compiled = document.text;
            indexScript(document);
        }
    }

    bool Editor::compileScripts(bool all)
    {
        std::map<dx::UClass*, std::string> edits;
        for (const ScriptDocument& document : documents) {
            if (document.text != document.compiled) {
                edits[document.cls] = scriptText(document.text);
            }
        }
        bool done = scripts.compile(edits, all);
        showScripts = true;
        if (done) {
            refreshClasses();
        } else {
            for (const dx::editor::CompileMessage& message : scripts.messages()) {
                if (message.error && message.cls) {
                    openDocument(message.cls, message.line);
                    break;
                }
            }
        }
        status = scripts.messages().empty() ? std::string() : scripts.messages().back().text;
        return done;
    }

    void Editor::scriptView(ScriptDocument& document, float height)
    {
        const ImGuiStyle& style = ImGui::GetStyle();
        ImFont* font = ImGui::GetFont();
        float lineHeight = ImGui::GetFontSize();
        ImGui::BeginChild("view", ImVec2(0, height), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
        float visible = ImGui::GetWindowHeight() - style.ScrollbarSize;
        if (document.scrollPending) {
            ImGui::SetScrollY(std::max(0.0f, static_cast<float>(document.jumpLine - 1) * lineHeight - visible * 0.5f));
            ImGui::SetKeyboardFocusHere();
            document.scrollPending = false;
        }
        float lines = static_cast<float>(document.lineStarts.size() + 2);
        ImVec2 size(std::max(document.width + style.FramePadding.x * 2.0f + lineHeight * 2.0f, ImGui::GetContentRegionAvail().x), std::max(lines * lineHeight + style.FramePadding.y * 2.0f, ImGui::GetContentRegionAvail().y));
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_NoHorizontalScroll;
        bool edited = ImGui::InputTextMultiline("##source", document.text.data(), document.text.capacity() + 1, size, flags, scriptInputCallback, &document);
        bool active = ImGui::IsItemActive();
        ImVec2 origin(ImGui::GetItemRectMin().x + style.FramePadding.x, ImGui::GetItemRectMin().y + style.FramePadding.y);
        if (edited) {
            indexScript(document);
        }
        const char* text = document.text.c_str();
        if (active && document.cursor != document.lastCursor) {
            size_t line = static_cast<size_t>(std::upper_bound(document.lineStarts.begin(), document.lineStarts.end(), static_cast<size_t>(document.cursor)) - document.lineStarts.begin()) - 1;
            float y = static_cast<float>(line) * lineHeight + style.FramePadding.y;
            float x = font->CalcTextSizeA(lineHeight, FLT_MAX, 0.0f, text + document.lineStarts[line], text + document.cursor).x + style.FramePadding.x;
            if (y < ImGui::GetScrollY()) {
                ImGui::SetScrollY(y);
            } else if (y + lineHeight > ImGui::GetScrollY() + visible) {
                ImGui::SetScrollY(y + lineHeight - visible);
            }
            float across = ImGui::GetWindowWidth() - style.ScrollbarSize;
            if (x < ImGui::GetScrollX()) {
                ImGui::SetScrollX(std::max(0.0f, x - lineHeight * 4.0f));
            } else if (x + lineHeight > ImGui::GetScrollX() + across) {
                ImGui::SetScrollX(x + lineHeight * 4.0f - across);
            }
            document.lastCursor = document.cursor;
        }
        ImGuiWindow* view = ImGui::GetCurrentWindow();
        if (!view->DC.ChildWindows.empty()) {
            ImGuiWindow* inner = view->DC.ChildWindows.back();
            ImDrawList* list = inner->DrawList;
            list->PushClipRect(inner->ClipRect.Min, inner->ClipRect.Max, false);
            size_t first = static_cast<size_t>(std::max(0.0f, (ImGui::GetScrollY() - style.FramePadding.y) / lineHeight));
            size_t last = std::min(document.lineStarts.size(), first + static_cast<size_t>(visible / lineHeight) + 2);
            for (size_t line = first; line < last; line++) {
                const char* begin = text + document.lineStarts[line];
                const char* end = line + 1 < document.lineStarts.size() ? text + document.lineStarts[line + 1] - 1 : text + document.text.size();
                bool inComment = document.commentState[line] != 0;
                float y = origin.y + static_cast<float>(line) * lineHeight;
                highlightLine(begin, end, inComment, [&](const char* from, const char* to, ImU32 color) {
                    float x = origin.x + font->CalcTextSizeA(lineHeight, FLT_MAX, 0.0f, begin, from).x;
                    list->AddText(font, lineHeight, ImVec2(x, y), color, from, to);
                });
            }
            list->PopClipRect();
        }
        ImGui::EndChild();
    }

    void Editor::scriptWindow()
    {
        classDefaultsWindow();
        if (!showScripts) {
            return;
        }
        ImGui::SetNextWindowPos(ImVec2(300, 80), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(860, 620), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Script Editor", &showScripts)) {
            ImGui::End();
            return;
        }
        if (ImGui::Button("Compile Changed")) {
            pendingCommand = "compilescripts changed";
        }
        ImGui::SameLine();
        if (ImGui::Button("Compile All")) {
            pendingCommand = "compilescripts all";
        }
        ImGui::SameLine();
        if (ImGui::Button("Export Changed")) {
            pendingCommand = "exportscripts changed";
        }
        if (activeDocument >= 0 && activeDocument < static_cast<int>(documents.size())) {
            const ScriptDocument& document = documents[static_cast<size_t>(activeDocument)];
            size_t line = static_cast<size_t>(std::upper_bound(document.lineStarts.begin(), document.lineStarts.end(), static_cast<size_t>(document.cursor)) - document.lineStarts.begin());
            ImGui::SameLine();
            ImGui::Text("%s  Line %zu", document.cls->pathName().c_str(), line);
        }
        float resultsHeight = ImGui::GetTextLineHeightWithSpacing() * 7.0f;
        activeDocument = -1;
        int closed = -1;
        if (ImGui::BeginTabBar("scripts")) {
            for (size_t i = 0; i < documents.size(); i++) {
                ScriptDocument& document = documents[i];
                bool open = true;
                std::string label = document.cls->name.str() + (document.text != document.compiled ? " *" : "") + "###" + document.cls->pathName();
                ImGuiTabItemFlags flags = selectDocument == static_cast<int>(i) ? ImGuiTabItemFlags_SetSelected : 0;
                if (ImGui::BeginTabItem(label.c_str(), &open, flags)) {
                    activeDocument = static_cast<int>(i);
                    scriptView(document, std::max(ImGui::GetFontSize() * 4.0f, ImGui::GetContentRegionAvail().y - resultsHeight));
                    ImGui::EndTabItem();
                }
                if (!open) {
                    closed = static_cast<int>(i);
                }
            }
            ImGui::EndTabBar();
        }
        selectDocument = -1;
        if (closed >= 0) {
            documents.erase(documents.begin() + closed);
        }
        ImGui::BeginChild("results", ImVec2(0, 0), ImGuiChildFlags_Borders);
        int index = 0;
        for (const dx::editor::CompileMessage& message : scripts.messages()) {
            ImGui::PushID(index++);
            if (message.error) {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 110, 110, 255));
            }
            if (ImGui::Selectable(message.text.c_str()) && message.cls) {
                openDocument(message.cls, message.line);
            }
            if (message.error) {
                ImGui::PopStyleColor();
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::End();
    }
}
