#include "core/config.h"
#include "core/objects.h"
#include "core/vm.h"
#include "deusex/deusex.h"
#include "engine/input.h"
#include "engine/props_extension.h"
#include "engine/resources.h"
#include "engine/world.h"
#include "extension/extension.h"
#include "extension/windows.h"

#include <catch_amalgamated.hpp>

#include <string>
#include <vector>

namespace {
    void initializeEngine()
    {
        static bool initialized = false;
        if (!initialized) {
            dx::engine::initialize(DEUSEX_GAME_DATA);
            initialized = true;
        }
    }

    struct RecordedTile {
        dx::UTexture* texture = nullptr;
        float x = 0;
        float y = 0;
        float w = 0;
        float h = 0;
        float u = 0;
        float v = 0;
        float ul = 0;
        float vl = 0;
        dx::LinearColor color;
        dx::BlendStyle style = dx::BlendStyle::None;
    };

    class RecordingCanvas : public dx::Canvas2D {
    public:
        RecordingCanvas(float w, float h)
            : canvasWidth(w)
            , canvasHeight(h)
        {
        }

        float width() const override
        {
            return canvasWidth;
        }

        float height() const override
        {
            return canvasHeight;
        }

        void setClip(float, float, float, float) override
        {
        }

        void clearClip() override
        {
        }

        void drawTile(dx::UTexture* texture, float x, float y, float w, float h, float u, float v, float ul, float vl, dx::LinearColor color, dx::BlendStyle style, bool) override
        {
            tiles.push_back({ texture, x, y, w, h, u, v, ul, vl, color, style });
        }

        void drawRect(float, float, float, float, dx::LinearColor, dx::BlendStyle) override
        {
        }

        void drawLine(float, float, float, float, dx::LinearColor) override
        {
        }

        float canvasWidth;
        float canvasHeight;
        std::vector<RecordedTile> tiles;
    };

    dx::UObject* makeRoot(const char* rootClass = "Extension.RootWindow")
    {
        initializeEngine();
        dx::extension::reset();
        dx::config::system().set("Engine.Engine", "Root", rootClass);
        dx::UClass* playerClass = dx::objects::findClass("Extension.PlayerPawnExt");
        dx::UObject* player = dx::objects::newObject(playerClass, dx::objects::packageObject(dx::Name("Transient")), dx::Name(), 0);
        dx::vm::Call init(player, dx::Name("InitRootWindow"));
        REQUIRE(init.valid());
        init.run();
        dx::UObject* root = dx::extension::rootWindow();
        REQUIRE(root != nullptr);
        return root;
    }

    dx::UObject* newChild(dx::UObject* parent, const char* className)
    {
        dx::vm::Call call(parent, dx::Name("NewChild"));
        REQUIRE(call.valid());
        call.obj(dx::objects::findClass(className)).b(true);
        call.run();
        return *reinterpret_cast<dx::UObject**>(call.returnValue());
    }

    dx::UObject* objectCall(dx::UObject* object, const char* function)
    {
        dx::vm::Call call(object, dx::Name(function));
        REQUIRE(call.valid());
        call.run();
        return *reinterpret_cast<dx::UObject**>(call.returnValue());
    }

    void voidCall(dx::UObject* object, const char* function)
    {
        dx::vm::Call call(object, dx::Name(function));
        REQUIRE(call.valid());
        call.run();
    }

    void setSize(dx::UObject* window, float w, float h)
    {
        dx::vm::Call call(window, dx::Name("SetSize"));
        call.f(w).f(h);
        call.run();
    }

    void setPos(dx::UObject* window, float x, float y)
    {
        dx::vm::Call call(window, dx::Name("SetPos"));
        call.f(x).f(y);
        call.run();
    }

    void setText(dx::UObject* window, const std::string& text)
    {
        dx::vm::Call call(window, dx::Name("SetText"));
        call.s(text);
        call.run();
    }

    void layout(float w, float h)
    {
        RecordingCanvas canvas(w, h);
        dx::extension::render(canvas);
    }

    bool boolResult(dx::vm::Call& call)
    {
        call.run();
        return *reinterpret_cast<uint32_t*>(call.returnValue()) != 0;
    }
}

TEST_CASE("NewChild links windows in order and Raise, Lower and Destroy relink siblings", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* a = newChild(root, "Extension.Window");
    dx::UObject* b = newChild(root, "Extension.Window");
    dx::UObject* c = newChild(root, "Extension.Window");
    REQUIRE(a != nullptr);
    CHECK(objectCall(root, "GetBottomChild") == a);
    CHECK(objectCall(root, "GetTopChild") == c);
    CHECK(objectCall(b, "GetParent") == root);
    CHECK(objectCall(b, "GetRootWindow") == root);

    voidCall(b, "Raise");
    CHECK(objectCall(root, "GetTopChild") == b);
    CHECK(objectCall(a, "GetHigherSibling") == c);

    voidCall(c, "Lower");
    CHECK(objectCall(root, "GetBottomChild") == c);
    CHECK(objectCall(c, "GetHigherSibling") == a);

    voidCall(b, "Destroy");
    CHECK(objectCall(root, "GetTopChild") == a);
    CHECK(objectCall(a, "GetLowerSibling") == c);
    CHECK(objectCall(a, "GetHigherSibling") == nullptr);
}

TEST_CASE("Horizontal tile window lays out children with margins and spacing", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* tile = newChild(root, "Extension.TileWindow");
    {
        dx::vm::Call call(tile, dx::Name("SetMargins"));
        call.f(2).f(3);
        call.run();
    }
    {
        dx::vm::Call call(tile, dx::Name("SetMinorSpacing"));
        call.f(5);
        call.run();
    }
    dx::UObject* first = newChild(tile, "Extension.Window");
    dx::UObject* second = newChild(tile, "Extension.Window");
    dx::UObject* third = newChild(tile, "Extension.Window");
    setSize(first, 10, 20);
    setSize(second, 30, 15);
    setSize(third, 20, 25);
    layout(800, 600);

    CHECK(dx::window::Width(tile) == 74.0f);
    CHECK(dx::window::Height(tile) == 31.0f);
    CHECK(dx::window::X(first) == 2.0f);
    CHECK(dx::window::X(second) == 17.0f);
    CHECK(dx::window::X(third) == 52.0f);
    CHECK(dx::window::Y(first) == 3.0f);
    CHECK(dx::window::Y(third) == 3.0f);
    CHECK(dx::window::Width(second) == 30.0f);
    CHECK(dx::window::Height(third) == 25.0f);
}

TEST_CASE("Text window preferred size follows TechMedium metrics and wraps words", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* text = newChild(root, "Extension.TextWindow");
    setText(text, "Hello World");
    {
        dx::vm::Call call(text, dx::Name("QueryPreferredWidth"));
        call.f(0);
        call.run();
        CHECK(*reinterpret_cast<float*>(call.returnValue()) == 69.0f);
    }
    {
        dx::vm::Call call(text, dx::Name("SetTextMargins"));
        call.f(4).f(2);
        call.run();
    }
    {
        dx::vm::Call call(text, dx::Name("QueryPreferredSize"));
        call.f(0).f(0);
        call.run();
        CHECK(*reinterpret_cast<float*>(call.parameter(0)) == 71.0f);
        CHECK(*reinterpret_cast<float*>(call.parameter(1)) == 15.0f);
    }
    {
        dx::vm::Call call(text, dx::Name("SetWordWrap"));
        call.b(true);
        call.run();
    }
    {
        dx::vm::Call call(text, dx::Name("QueryPreferredHeight"));
        call.f(40);
        call.run();
        CHECK(*reinterpret_cast<float*>(call.returnValue()) == 26.0f);
    }
}

TEST_CASE("Window background is drawn at its position scaled by the virtual resolution", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* window = newChild(root, "Extension.Window");
    dx::UObject* texture = dx::objects::loadObject("Engine.DefaultTexture", nullptr);
    REQUIRE(dx::cast<dx::UTexture>(texture) != nullptr);
    {
        dx::vm::Call call(window, dx::Name("SetBackground"));
        call.obj(texture);
        call.run();
    }
    setPos(window, 10, 20);
    setSize(window, 64, 32);

    RecordingCanvas small(800, 600);
    dx::extension::render(small);
    REQUIRE(small.tiles.size() == 1);
    CHECK(small.tiles[0].texture == texture);
    CHECK(small.tiles[0].x == 10.0f);
    CHECK(small.tiles[0].y == 20.0f);
    CHECK(small.tiles[0].w == 64.0f);
    CHECK(small.tiles[0].h == 32.0f);
    CHECK(small.tiles[0].ul == 64.0f);
    CHECK(small.tiles[0].vl == 32.0f);
    CHECK(small.tiles[0].style == dx::BlendStyle::Normal);

    RecordingCanvas large(1600, 1200);
    dx::extension::render(large);
    REQUIRE(large.tiles.size() == 1);
    CHECK(large.tiles[0].x == 20.0f);
    CHECK(large.tiles[0].y == 40.0f);
    CHECK(large.tiles[0].w == 128.0f);
    CHECK(large.tiles[0].h == 64.0f);
    CHECK(large.tiles[0].ul == 64.0f);
}

TEST_CASE("Text window draws one glyph tile per character from the font texture", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* text = newChild(root, "Extension.TextWindow");
    setText(text, "He");
    setPos(text, 100, 50);
    setSize(text, 50, 20);
    RecordingCanvas canvas(800, 600);
    dx::extension::render(canvas);
    REQUIRE(canvas.tiles.size() == 2);
    CHECK(canvas.tiles[0].x == 118.0f);
    CHECK(canvas.tiles[0].y == 54.0f);
    CHECK(canvas.tiles[0].w == 7.0f);
    CHECK(canvas.tiles[0].h == 10.0f);
    CHECK(canvas.tiles[0].u == 10.0f);
    CHECK(canvas.tiles[0].v == 12.0f);
    CHECK(canvas.tiles[1].x == 125.0f);
    CHECK(canvas.tiles[1].u == 213.0f);
    CHECK(canvas.tiles[1].v == 12.0f);
    CHECK(canvas.tiles[0].style == dx::BlendStyle::Masked);
    CHECK(canvas.tiles[0].color.r == 0.0f);
    CHECK(canvas.tiles[0].color.g == 1.0f);
}

TEST_CASE("Clicking a checkbox inside a modal window toggles it", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* modal = newChild(root, "Extension.ModalWindow");
    {
        dx::vm::Call call(modal, dx::Name("SetWindowAlignments"));
        call.byte(3).byte(3);
        call.run();
    }
    dx::UObject* box = newChild(modal, "Extension.CheckboxWindow");
    setPos(box, 100, 100);
    setSize(box, 40, 20);
    layout(800, 600);
    CHECK(dx::extension::wantsMouse());

    dx::extension::mouseMove(300, 300);
    dx::extension::mouseButton(dx::IK_LeftMouse, true);
    dx::extension::mouseButton(dx::IK_LeftMouse, false);
    {
        dx::vm::Call call(box, dx::Name("GetToggle"));
        CHECK_FALSE(boolResult(call));
    }

    dx::extension::mouseMove(110, 110);
    CHECK(dx::extension::mouseButton(dx::IK_LeftMouse, true));
    CHECK(dx::extension::mouseButton(dx::IK_LeftMouse, false));
    {
        dx::vm::Call call(box, dx::Name("GetToggle"));
        CHECK(boolResult(call));
    }
}

TEST_CASE("FlagBase stores typed flags and deletes expired ones", "[data]")
{
    initializeEngine();
    dx::UObject* base = dx::objects::newObject(dx::objects::findClass("Extension.FlagBase"), dx::objects::packageObject(dx::Name("Transient")), dx::Name(), 0);
    {
        dx::vm::Call call(base, dx::Name("SetBool"));
        call.n(dx::Name("Ford")).b(true).b(true);
        CHECK(boolResult(call));
    }
    {
        dx::vm::Call call(base, dx::Name("GetBool"));
        call.n(dx::Name("Ford"));
        CHECK(boolResult(call));
    }
    {
        dx::vm::Call call(base, dx::Name("GetBool"));
        call.n(dx::Name("Missing"));
        CHECK_FALSE(boolResult(call));
    }
    {
        dx::vm::Call call(base, dx::Name("SetInt"));
        call.n(dx::Name("Count")).i(5).b(false);
        CHECK_FALSE(boolResult(call));
    }
    {
        dx::vm::Call call(base, dx::Name("SetInt"));
        call.n(dx::Name("Count")).i(7).b(true);
        CHECK(boolResult(call));
    }
    {
        dx::vm::Call call(base, dx::Name("SetFloat"));
        call.n(dx::Name("Count")).f(1.5f).b(true);
        CHECK(boolResult(call));
    }
    {
        dx::vm::Call call(base, dx::Name("GetInt"));
        call.n(dx::Name("Count"));
        call.run();
        CHECK(*reinterpret_cast<int32_t*>(call.returnValue()) == 7);
    }
    {
        dx::vm::Call call(base, dx::Name("GetFloat"));
        call.n(dx::Name("Count"));
        call.run();
        CHECK(*reinterpret_cast<float*>(call.returnValue()) == 1.5f);
    }
    {
        dx::vm::Call call(base, dx::Name("SetBool"));
        call.n(dx::Name("Old")).b(true).b(true).i(3);
        CHECK(boolResult(call));
    }
    {
        dx::vm::Call call(base, dx::Name("SetBool"));
        call.n(dx::Name("New")).b(true).b(true).i(5);
        CHECK(boolResult(call));
    }
    {
        dx::vm::Call call(base, dx::Name("DeleteExpiredFlags"));
        call.i(4);
        call.run();
    }
    auto exists = [base](const char* name) {
        dx::vm::Call call(base, dx::Name("CheckFlag"));
        call.n(dx::Name(name)).byte(0);
        call.run();
        return *reinterpret_cast<uint32_t*>(call.returnValue()) != 0;
    };
    CHECK_FALSE(exists("Old"));
    CHECK(exists("New"));
    CHECK(exists("Ford"));

    int32_t iterator = 0;
    {
        dx::vm::Call call(base, dx::Name("CreateIterator"));
        call.byte(0);
        call.run();
        iterator = *reinterpret_cast<int32_t*>(call.returnValue());
    }
    int count = 0;
    while (true) {
        dx::vm::Call call(base, dx::Name("GetNextFlagName"));
        call.i(iterator).n(dx::Name());
        if (!boolResult(call)) {
            break;
        }
        count++;
    }
    CHECK(count == 2);
}

TEST_CASE("List window sorts rows by a float column", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* list = newChild(root, "Extension.ListWindow");
    {
        dx::vm::Call call(list, dx::Name("SetNumColumns"));
        call.i(2);
        call.run();
    }
    {
        dx::vm::Call call(list, dx::Name("SetColumnType"));
        call.i(1).byte(1);
        call.run();
    }
    std::vector<int32_t> ids;
    for (const char* row : { "b;2", "a;10", "c;1" }) {
        dx::vm::Call call(list, dx::Name("AddRow"));
        call.s(row);
        call.run();
        ids.push_back(*reinterpret_cast<int32_t*>(call.returnValue()));
    }
    {
        dx::vm::Call call(list, dx::Name("SetSortColumn"));
        call.i(1);
        call.run();
    }
    voidCall(list, "Sort");
    auto rowAt = [list](int32_t index) {
        dx::vm::Call call(list, dx::Name("IndexToRowId"));
        call.i(index);
        call.run();
        return *reinterpret_cast<int32_t*>(call.returnValue());
    };
    CHECK(rowAt(0) == ids[2]);
    CHECK(rowAt(1) == ids[0]);
    CHECK(rowAt(2) == ids[1]);
    {
        dx::vm::Call call(list, dx::Name("GetField"));
        call.i(ids[1]).i(0);
        call.run();
        CHECK(*reinterpret_cast<std::string*>(call.returnValue()) == "a");
    }
}

TEST_CASE("Text parser tokenizes a DeusExText book", "[data]")
{
    initializeEngine();
    dx::UObject* parser = dx::objects::newObject(dx::objects::findClass("DeusExText.DeusExTextParser"), dx::objects::packageObject(dx::Name("Transient")), dx::Name(), 0);
    {
        dx::vm::Call call(parser, dx::Name("OpenText"));
        call.n(dx::Name("00_Book01"));
        REQUIRE(boolResult(call));
    }

    struct Token {
        uint8_t tag;
        std::string text;
    };

    std::vector<Token> tokens;
    for (int i = 0; i < 11; i++) {
        dx::vm::Call process(parser, dx::Name("ProcessText"));
        REQUIRE(boolResult(process));
        dx::vm::Call tag(parser, dx::Name("GetTag"));
        tag.run();
        dx::vm::Call text(parser, dx::Name("GetText"));
        text.run();
        tokens.push_back({ *tag.returnValue(), *reinterpret_cast<std::string*>(text.returnValue()) });
    }
    CHECK(tokens[0].tag == 15);
    CHECK(tokens[1].tag == 12);
    CHECK(tokens[2].tag == 19);
    CHECK(tokens[3].tag == 0);
    CHECK(tokens[3].text == "UNATCO TRAINING MANUAL");
    CHECK(tokens[4].tag == 20);
    CHECK(tokens[5].tag == 0);
    CHECK(tokens[5].text == "  ");
    CHECK(tokens[6].tag == 18);
    CHECK(tokens[7].text == "  ");
    CHECK(tokens[8].tag == 18);
    CHECK(tokens[9].tag == 12);
    CHECK(tokens[10].tag == 0);
    CHECK(tokens[10].text == "Section 3C: Multitools  ");
}

TEST_CASE("ConEvent sound length divides the data size by the first frame bitrate", "[data]")
{
    initializeEngine();
    dx::UObject* soundObject = dx::objects::newObject(dx::objects::findClass("Engine.Sound"), dx::objects::packageObject(dx::Name("Transient")), dx::Name(), 0);
    dx::USound* sound = dx::cast<dx::USound>(soundObject);
    REQUIRE(sound != nullptr);
    sound->bytes.assign(3 * 130, 0);
    for (size_t frame = 0; frame < 3; frame++) {
        sound->bytes[frame * 130 + 0] = 0xff;
        sound->bytes[frame * 130 + 1] = 0xfb;
        sound->bytes[frame * 130 + 2] = 0x20;
        sound->bytes[frame * 130 + 3] = 0xc4;
    }
    dx::UObject* event = dx::objects::newObject(dx::objects::findClass("ConSys.ConEvent"), dx::objects::packageObject(dx::Name("Transient")), dx::Name(), 0);
    dx::vm::Call call(event, dx::Name("GetSoundLength"));
    REQUIRE(call.valid());
    call.obj(sound);
    call.run();
    CHECK(*reinterpret_cast<float*>(call.returnValue()) == Catch::Approx(390.0 * 8.0 / 40000.0).epsilon(1e-5));
}

TEST_CASE("Text window wraps words without an explicit SetWordWrap", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* text = newChild(root, "Extension.TextWindow");
    setText(text, "Hello World");
    dx::vm::Call call(text, dx::Name("QueryPreferredHeight"));
    call.f(40);
    call.run();
    CHECK(*reinterpret_cast<float*>(call.returnValue()) == 28.0f);
}

TEST_CASE("Computer window prefers the size of its text grid", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* computer = newChild(root, "Extension.ComputerWindow");
    {
        dx::vm::Call call(computer, dx::Name("SetTextSize"));
        call.i(10).i(3);
        call.run();
    }
    {
        dx::vm::Call call(computer, dx::Name("SetTextFont"));
        call.obj(nullptr).i(6).i(10).skip();
        call.run();
    }
    dx::vm::Call call(computer, dx::Name("QueryPreferredSize"));
    call.f(0).f(0);
    call.run();
    CHECK(*reinterpret_cast<float*>(call.parameter(0)) == 60.0f);
    CHECK(*reinterpret_cast<float*>(call.parameter(1)) == 30.0f);
}

TEST_CASE("Computer window moves a word that does not fit to the next row", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* computer = newChild(root, "Extension.ComputerWindow");
    {
        dx::vm::Call call(computer, dx::Name("SetTextSize"));
        call.i(10).i(3);
        call.run();
    }
    {
        dx::vm::Call call(computer, dx::Name("Print"));
        call.s("hello wonderful world").b(false);
        call.run();
    }
    dx::extension::tick(1.0f);
    dx::extension::tick(1.0f);
    dx::extension::UComputerWindow* window = dx::cast<dx::extension::UComputerWindow>(computer);
    REQUIRE(window != nullptr);
    std::string rows(30, ' ');
    for (const dx::extension::UComputerWindow::Event& element : window->display) {
        REQUIRE(element.row >= 0);
        REQUIRE(element.row < 3);
        REQUIRE(element.col >= 0);
        REQUIRE(element.col < 10);
        rows[static_cast<size_t>(element.row * 10 + element.col)] = element.ch;
    }
    CHECK(rows.substr(0, 10) == "hello     ");
    CHECK(rows.substr(10, 10) == "wonderful ");
    CHECK(rows.substr(20, 10) == "world     ");
}

TEST_CASE("Computer window characters fade in over the fade speed", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* computer = newChild(root, "Extension.ComputerWindow");
    {
        dx::vm::Call call(computer, dx::Name("SetFadeSpeed"));
        call.f(1.0f);
        call.run();
    }
    {
        dx::vm::Call call(computer, dx::Name("Print"));
        call.s("a").b(false);
        call.run();
    }
    dx::extension::tick(0.5f);
    dx::extension::UComputerWindow* window = dx::cast<dx::extension::UComputerWindow>(computer);
    REQUIRE(window != nullptr);
    CHECK(window->display.empty());
    dx::extension::tick(0.25f);
    REQUIRE(window->display.size() == 1);
    CHECK(window->display[0].ch == 'a');
    CHECK(window->display[0].time == Catch::Approx(0.75f));
    dx::extension::tick(1.0f);
    CHECK(window->display[0].time == 0.0f);
}

TEST_CASE("Keys reach no window when nothing has focus and no modal is open", "[data]")
{
    dx::UObject* root = makeRoot("DeusEx.DeusExRootWindow");
    CHECK(root->cls->name == dx::Name("DeusExRootWindow"));
    CHECK_FALSE(dx::extension::keyEvent(dx::IK_Escape, dx::IST_Press, 0));
    CHECK_FALSE(dx::extension::keyEvent(dx::IK_Escape, dx::IST_Release, 0));
}

TEST_CASE("SetChildVisibility changes only the window it is called on", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* parent = newChild(root, "Extension.Window");
    dx::UObject* first = newChild(parent, "Extension.Window");
    dx::UObject* second = newChild(parent, "Extension.Window");
    voidCall(first, "Hide");
    voidCall(second, "Hide");
    {
        dx::vm::Call call(parent, dx::Name("SetChildVisibility"));
        call.b(true);
        call.run();
    }
    CHECK(dx::window::bIsVisible(first) == 0);
    CHECK(dx::window::bIsVisible(second) == 0);
    {
        dx::vm::Call call(first, dx::Name("SetChildVisibility"));
        call.b(true);
        call.run();
    }
    CHECK(dx::window::bIsVisible(first) == 1);
    CHECK(dx::window::bIsVisible(second) == 0);
}

TEST_CASE("Setting a slider value updates the value text of a menu slider", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* slider = newChild(root, "DeusEx.MenuUISliderButtonWindow");
    REQUIRE(slider != nullptr);
    {
        dx::vm::Call call(slider, dx::Name("SetTicks"));
        call.i(11).i(0).i(10);
        call.run();
    }
    dx::UObject* scale = *reinterpret_cast<dx::UObject**>(slider->propertyAddress(dx::Name("winSlider")));
    dx::UObject* text = *reinterpret_cast<dx::UObject**>(slider->propertyAddress(dx::Name("winScaleText")));
    REQUIRE(scale != nullptr);
    REQUIRE(text != nullptr);
    {
        dx::vm::Call call(scale, dx::Name("SetValue"));
        call.f(7);
        call.run();
    }
    CHECK(*reinterpret_cast<std::string*>(text->propertyAddress(dx::Name("buttonText"))) == "7");
}

TEST_CASE("Tab moves focus to the first window of the tab group and Down steps to the next", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* modal = newChild(root, "Extension.ModalWindow");
    dx::UObject* first = newChild(modal, "Extension.ButtonWindow");
    dx::UObject* second = newChild(modal, "Extension.ButtonWindow");
    setSize(modal, 200, 200);
    setPos(first, 10, 10);
    setSize(first, 100, 20);
    setPos(second, 10, 50);
    setSize(second, 100, 20);
    layout(800, 600);
    {
        dx::vm::Call call(modal, dx::Name("SetFocusWindow"));
        call.obj(second);
        call.run();
    }
    REQUIRE(objectCall(root, "GetFocusWindow") == second);
    dx::extension::keyEvent(dx::IK_Tab, dx::IST_Press, 0);
    dx::extension::keyEvent(dx::IK_Tab, dx::IST_Release, 0);
    CHECK(objectCall(root, "GetFocusWindow") == first);
    dx::extension::keyEvent(dx::IK_Down, dx::IST_Press, 0);
    dx::extension::keyEvent(dx::IK_Down, dx::IST_Release, 0);
    CHECK(objectCall(root, "GetFocusWindow") == second);
}

TEST_CASE("GC text extent with zero width measures without wrapping", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* window = newChild(root, "Extension.Window");
    dx::UObject* gc = objectCall(window, "GetGC");
    REQUIRE(gc != nullptr);
    dx::UClass* canvasClass = dx::objects::findClass("Engine.Canvas");
    REQUIRE(canvasClass != nullptr);
    dx::UProperty* smallFont = canvasClass->findProperty(dx::Name("SmallFont"));
    REQUIRE(smallFont != nullptr);
    dx::UObject* font = *reinterpret_cast<dx::UObject**>(canvasClass->defaultObject->data + smallFont->offset);
    REQUIRE(font != nullptr);
    {
        dx::vm::Call setFont(gc, dx::Name("SetFont"));
        REQUIRE(setFont.valid());
        setFont.obj(font);
        setFont.run();
    }
    {
        dx::vm::Call wrap(gc, dx::Name("EnableWordWrap"));
        REQUIRE(wrap.valid());
        wrap.b(true);
        wrap.run();
    }
    dx::vm::Call extent(gc, dx::Name("GetTextExtent"));
    REQUIRE(extent.valid());
    extent.f(0).f(0).f(0).s("Hello World");
    extent.run();
    CHECK(*reinterpret_cast<float*>(extent.parameter(1)) == 66.0f);
    CHECK(*reinterpret_cast<float*>(extent.parameter(2)) == 9.0f);
    dx::vm::Call release(window, dx::Name("ReleaseGC"));
    release.obj(gc);
    release.run();
}

TEST_CASE("modulated GC style leaves text drawing unmodulated", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* window = newChild(root, "Extension.Window");
    dx::UObject* gc = objectCall(window, "GetGC");
    REQUIRE(gc != nullptr);
    dx::vm::Call style(gc, dx::Name("SetStyle"));
    REQUIRE(style.valid());
    style.byte(4);
    style.run();
    CHECK(dx::gc::bModulated(gc) == 1);
    CHECK(dx::gc::textPolyFlags(gc) == 0);
    CHECK(dx::gc::bTextTranslucent(gc) == 0);
    dx::vm::Call release(window, dx::Name("ReleaseGC"));
    release.obj(gc);
    release.run();
}

TEST_CASE("Conversation sound length uses the first MP3 frame bitrate over the whole data size", "[data]")
{
    dx::engine::initialize(DEUSEX_GAME_DATA);
    dx::UObject* sound = dx::objects::loadObject("DeusExConAudioMission01.ConAudioMission01_576", nullptr);
    dx::UClass* eventClass = dx::objects::findClass("ConSys.ConEventSpeech");
    dx::objects::flushLoads();
    REQUIRE(sound != nullptr);
    REQUIRE(eventClass != nullptr);
    dx::UObject* event = dx::objects::newObject(eventClass, dx::objects::packageObject(dx::Name("Transient")), dx::Name("SoundLengthEvent"), 0);
    dx::vm::Call call(event, dx::Name("GetSoundLength"));
    call.obj(sound);
    call.run();
    CHECK(*reinterpret_cast<float*>(call.returnValue()) == Catch::Approx(61464.0f * 8.0f / 48000.0f));
}

TEST_CASE("Accelerator keys activate a button only while Alt is held", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* modal = newChild(root, "Extension.ModalWindow");
    dx::UObject* box = newChild(modal, "Extension.CheckboxWindow");
    setText(box, "|&Box");
    setSize(modal, 200, 200);
    setSize(box, 100, 20);
    layout(800, 600);
    dx::extension::charEvent('b');
    {
        dx::vm::Call call(box, dx::Name("GetToggle"));
        CHECK_FALSE(boolResult(call));
    }
    dx::extension::keyEvent(dx::IK_Alt, dx::IST_Press, 0);
    dx::extension::charEvent('b');
    dx::extension::keyEvent(dx::IK_Alt, dx::IST_Release, 0);
    {
        dx::vm::Call call(box, dx::Name("GetToggle"));
        CHECK(boolResult(call));
    }
}

TEST_CASE("Root window is the current modal while no modal window is open", "[data]")
{
    dx::UObject* root = makeRoot();
    {
        dx::vm::Call call(root, dx::Name("IsCurrentModal"));
        CHECK(boolResult(call));
    }
    dx::UObject* modal = newChild(root, "Extension.ModalWindow");
    {
        dx::vm::Call call(root, dx::Name("IsCurrentModal"));
        CHECK_FALSE(boolResult(call));
    }
    {
        dx::vm::Call call(modal, dx::Name("IsCurrentModal"));
        CHECK(boolResult(call));
    }
}

TEST_CASE("Edit window drops characters rejected by the script FilterChar", "[data]")
{
    dx::UObject* root = makeRoot("DeusEx.DeusExRootWindow");
    dx::UObject* modal = newChild(root, "Extension.ModalWindow");
    dx::UObject* edit = newChild(modal, "DeusEx.MenuUIEditWindow");
    REQUIRE(edit != nullptr);
    {
        dx::vm::Call call(edit, dx::Name("SetFilter"));
        call.s("ab");
        call.run();
    }
    {
        dx::vm::Call call(edit, dx::Name("InsertText"));
        call.s("abcab").b(true).b(false);
        CHECK(boolResult(call));
    }
    CHECK(dx::textwindow::Text(edit) == "abab");
    {
        dx::vm::Call call(edit, dx::Name("InsertText"));
        call.s("c").b(true).b(false);
        CHECK_FALSE(boolResult(call));
    }
    {
        dx::vm::Call call(root, dx::Name("SetFocusWindow"));
        call.obj(edit);
        call.run();
    }
    REQUIRE(objectCall(root, "GetFocusWindow") == edit);
    dx::extension::charEvent('x');
    dx::extension::charEvent('b');
    CHECK(dx::textwindow::Text(edit) == "ababb");
}

TEST_CASE("Showing a parent selects the first row of a list that became visible with it", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* parent = static_cast<dx::extension::UWindow*>(root)->newChild(dx::objects::findClass("Extension.Window"), false);
    CHECK(dx::window::bIsVisible(parent) == 0);
    REQUIRE(parent != nullptr);
    dx::UObject* list = newChild(parent, "Extension.ListWindow");
    int32_t firstRow = 0;
    {
        dx::vm::Call call(list, dx::Name("AddRow"));
        call.s("first").i(0);
        call.run();
        firstRow = *reinterpret_cast<int32_t*>(call.returnValue());
    }
    {
        dx::vm::Call call(list, dx::Name("AddRow"));
        call.s("second").i(0);
        call.run();
    }
    {
        dx::vm::Call call(list, dx::Name("GetSelectedRow"));
        call.run();
        CHECK(*reinterpret_cast<int32_t*>(call.returnValue()) == 0);
    }
    static_cast<dx::extension::UWindow*>(parent)->show(true);
    {
        dx::vm::Call call(list, dx::Name("GetSelectedRow"));
        call.run();
        CHECK(*reinterpret_cast<int32_t*>(call.returnValue()) == firstRow);
    }
}

TEST_CASE("VisibilityChanged reaches a window created visible and its descendants when an ancestor is shown", "[data]")
{
    dx::UObject* root = makeRoot("DeusEx.DeusExRootWindow");
    dx::UObject* compass = newChild(root, "DeusEx.HUDCompassDisplay");
    CHECK(dx::window::bTickEnabled(compass) == 1);
    dx::UObject* parent = static_cast<dx::extension::UWindow*>(root)->newChild(dx::objects::findClass("Extension.Window"), false);
    CHECK(dx::window::bIsVisible(parent) == 0);
    dx::UObject* inner = newChild(parent, "DeusEx.HUDCompassDisplay");
    CHECK(dx::window::bTickEnabled(inner) == 0);
    static_cast<dx::extension::UWindow*>(parent)->show(true);
    CHECK(dx::window::bTickEnabled(inner) == 1);
    static_cast<dx::extension::UWindow*>(parent)->show(false);
    CHECK(dx::window::bTickEnabled(inner) == 0);
}

TEST_CASE("ConvertVectorToCoordinates succeeds for points in front of the view even when they fall outside the window", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::extension::setViewportSize(800, 600);
    layout(800, 600);
    dx::UObject* frame = newChild(root, "Extension.Window");
    setPos(frame, 10, 20);
    setSize(frame, 100, 100);
    layout(800, 600);
    dx::extension::setProjector([](const dx::Vector& location, float& pixelX, float& pixelY) {
        pixelX = location.x;
        pixelY = location.y;
        return location.z > 0;
    });
    float outX = 0;
    float outY = 0;
    {
        dx::vm::Call call(frame, dx::Name("ConvertVectorToCoordinates"));
        call.v({ -500.75f, 3000.5f, 1 });
        *reinterpret_cast<float*>(call.parameter(1)) = 0;
        *reinterpret_cast<float*>(call.parameter(2)) = 0;
        CHECK(boolResult(call));
        outX = *reinterpret_cast<float*>(call.parameter(1));
        outY = *reinterpret_cast<float*>(call.parameter(2));
    }
    CHECK(outX == -510.0f);
    CHECK(outY == 2980.0f);
    {
        dx::vm::Call call(frame, dx::Name("ConvertVectorToCoordinates"));
        call.v({ 50, 50, -1 });
        CHECK_FALSE(boolResult(call));
        CHECK(*reinterpret_cast<float*>(call.parameter(1)) == 390.0f);
        CHECK(*reinterpret_cast<float*>(call.parameter(2)) == 280.0f);
    }
    dx::extension::setProjector(nullptr);
}

TEST_CASE("Checkbox text uses the colors set with SetTextColors", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* box = newChild(root, "Extension.CheckboxWindow");
    setText(box, "He");
    setPos(box, 100, 50);
    setSize(box, 80, 20);
    {
        dx::vm::Call call(box, dx::Name("SetTextColors"));
        dx::Color red { 255, 0, 0, 255 };
        for (int i = 0; i < 6; i++) {
            *reinterpret_cast<dx::Color*>(call.parameter(static_cast<size_t>(i))) = red;
        }
        call.run();
    }
    RecordingCanvas canvas(800, 600);
    dx::extension::render(canvas);
    int red = 0;
    int green = 0;
    for (const RecordedTile& tile : canvas.tiles) {
        if (tile.color.r == 1.0f && tile.color.g == 0.0f) {
            red++;
        }
        if (tile.color.r == 0.0f && tile.color.g == 1.0f) {
            green++;
        }
    }
    CHECK(red == 2);
    CHECK(green == 0);
}

TEST_CASE("Edit window Up and Down keep the caret column across rows", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* edit = newChild(root, "Extension.EditWindow");
    setSize(edit, 300, 100);
    layout(800, 600);
    setText(edit, "abcdef\nab");
    auto move = [&](uint8_t where) {
        dx::vm::Call call(edit, dx::Name("MoveInsertionPoint"));
        call.byte(where).b(false);
        call.run();
    };
    auto position = [&]() {
        dx::vm::Call call(edit, dx::Name("GetInsertionPoint"));
        call.run();
        return *reinterpret_cast<int32_t*>(call.returnValue());
    };
    move(11);
    REQUIRE(position() == 9);
    move(0);
    CHECK(position() == 2);
    move(3);
    move(3);
    CHECK(position() == 4);
    move(1);
    CHECK(position() == 9);
    move(0);
    CHECK(position() == 4);
    move(1);
    move(6);
    CHECK(position() == 7);
}

TEST_CASE("Edit window SetText clears undo, marks the text changed and puts the caret at the start", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* edit = newChild(root, "Extension.EditWindow");
    setText(edit, "hello");
    {
        dx::vm::Call call(edit, dx::Name("GetInsertionPoint"));
        call.run();
        CHECK(*reinterpret_cast<int32_t*>(call.returnValue()) == 0);
    }
    {
        dx::vm::Call call(edit, dx::Name("HasTextChanged"));
        CHECK(boolResult(call));
    }
    voidCall(edit, "Undo");
    CHECK(dx::textwindow::Text(edit) == "hello");
}

TEST_CASE("Undoing back to the unchanged text clears the edit window changed flag", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* edit = newChild(root, "Extension.EditWindow");
    setText(edit, "ab");
    voidCall(edit, "ClearTextChangedFlag");
    {
        dx::vm::Call call(edit, dx::Name("SetInsertionPoint"));
        call.i(2).b(false);
        call.run();
    }
    {
        dx::vm::Call call(edit, dx::Name("InsertText"));
        call.s("c").b(true).b(false);
        CHECK(boolResult(call));
    }
    CHECK(dx::textwindow::Text(edit) == "abc");
    {
        dx::vm::Call call(edit, dx::Name("HasTextChanged"));
        CHECK(boolResult(call));
    }
    voidCall(edit, "Undo");
    CHECK(dx::textwindow::Text(edit) == "ab");
    {
        dx::vm::Call call(edit, dx::Name("HasTextChanged"));
        CHECK_FALSE(boolResult(call));
    }
}

TEST_CASE("Opening the first modal window forgets keys held before it, but not mouse buttons", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::extension::keyEvent(dx::IK_Shift, dx::IST_Press, 0);
    dx::extension::keyEvent(dx::IK_LeftMouse, dx::IST_Press, 0);
    auto down = [&](uint8_t key) {
        dx::vm::Call call(root, dx::Name("IsKeyDown"));
        call.byte(key);
        return boolResult(call);
    };
    REQUIRE(down(dx::IK_Shift));
    newChild(root, "Extension.ModalWindow");
    CHECK_FALSE(down(dx::IK_Shift));
    CHECK(down(dx::IK_LeftMouse));
    dx::extension::keyEvent(dx::IK_Shift, dx::IST_Press, 0);
    newChild(root, "Extension.ModalWindow");
    CHECK(down(dx::IK_Shift));
    dx::extension::keyEvent(dx::IK_Shift, dx::IST_Release, 0);
    dx::extension::keyEvent(dx::IK_LeftMouse, dx::IST_Release, 0);
}

TEST_CASE("Edit window caret stays on for the blink start delay then toggles every half period", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* edit = newChild(root, "Extension.EditWindow");
    setSize(edit, 200, 20);
    {
        dx::vm::Call call(root, dx::Name("SetFocusWindow"));
        call.obj(edit);
        call.run();
    }
    REQUIRE(objectCall(root, "GetFocusWindow") == edit);
    dx::extension::tick(5.0f);
    {
        dx::vm::Call call(edit, dx::Name("InsertText"));
        call.s("a").b(true).b(false);
        call.run();
    }
    auto showing = [&](float advance) {
        dx::extension::tick(advance);
        layout(800, 600);
        return dx::editwindow::bCursorShowing(edit) != 0;
    };
    CHECK(showing(0.0f));
    CHECK(showing(0.7f));
    CHECK_FALSE(showing(0.1f));
    CHECK(showing(0.5f));
    CHECK_FALSE(showing(0.5f));
}

TEST_CASE("A new modal window focuses its top tab group even before it has been drawn", "[data]")
{
    dx::UObject* root = makeRoot();
    layout(800, 600);
    dx::UObject* modal = newChild(root, "Extension.ModalWindow");
    setSize(modal, 400, 400);
    dx::UObject* button = newChild(modal, "Extension.ButtonWindow");
    setPos(button, 10, 350);
    setSize(button, 100, 20);
    dx::UObject* clip = newChild(modal, "Extension.ClipWindow");
    setPos(clip, 10, 10);
    setSize(clip, 300, 200);
    dx::UObject* list = newChild(clip, "Extension.ListWindow");
    {
        dx::vm::Call call(list, dx::Name("AddRow"));
        call.s("row").i(0);
        call.run();
    }
    dx::extension::tick(0.01f);
    CHECK(objectCall(root, "GetFocusWindow") == list);
}

TEST_CASE("Keys only reach windows while a modal window has grabbed the keyboard", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* box = newChild(root, "Extension.CheckboxWindow");
    setSize(box, 40, 20);
    layout(800, 600);
    {
        dx::vm::Call call(root, dx::Name("SetFocusWindow"));
        call.obj(box);
        call.run();
    }
    REQUIRE(objectCall(root, "GetFocusWindow") == box);
    CHECK_FALSE(dx::extension::keyEvent(dx::IK_Enter, dx::IST_Press, 0));
    dx::extension::keyEvent(dx::IK_Enter, dx::IST_Release, 0);
    {
        dx::vm::Call call(box, dx::Name("GetToggle"));
        CHECK_FALSE(boolResult(call));
    }
    dx::UObject* modal = newChild(root, "Extension.ModalWindow");
    setSize(modal, 200, 200);
    dx::UObject* inner = newChild(modal, "Extension.CheckboxWindow");
    setSize(inner, 40, 20);
    layout(800, 600);
    {
        dx::vm::Call call(root, dx::Name("SetFocusWindow"));
        call.obj(inner);
        call.run();
    }
    REQUIRE(objectCall(root, "GetFocusWindow") == inner);
    CHECK(dx::extension::keyEvent(dx::IK_Enter, dx::IST_Press, 0));
    dx::extension::keyEvent(dx::IK_Enter, dx::IST_Release, 0);
    {
        dx::vm::Call call(inner, dx::Name("GetToggle"));
        CHECK(boolResult(call));
    }
}

TEST_CASE("A cursor set without a hot spot is centered on the mouse position", "[data]")
{
    dx::UObject* root = makeRoot("DeusEx.DeusExRootWindow");
    voidCall(root, "DestroyAllChildren");
    newChild(root, "Extension.ModalWindow");
    layout(800, 600);
    dx::extension::mouseMove(100, 200);
    RecordingCanvas canvas(800, 600);
    dx::extension::render(canvas);
    const RecordedTile* cursor = nullptr;
    for (const RecordedTile& tile : canvas.tiles) {
        if (tile.texture && tile.texture->name == dx::Name("DeusExCursor1")) {
            cursor = &tile;
        }
    }
    REQUIRE(cursor != nullptr);
    CHECK(cursor->x == 69.0f);
    CHECK(cursor->y == 169.0f);
}

namespace {
    int32_t buttonY(dx::UObject* menu, int index)
    {
        dx::UStruct* button = menu->cls->findProperty(dx::Name("buttonDefaults"))->structType;
        return *reinterpret_cast<int32_t*>(menu->propertyAddress(dx::Name("buttonDefaults"), index) + button->findProperty(dx::Name("Y"))->offset);
    }

    dx::UObject* buttonInvoke(dx::UObject* menu, int index)
    {
        dx::UStruct* button = menu->cls->findProperty(dx::Name("buttonDefaults"))->structType;
        return *reinterpret_cast<dx::UObject**>(menu->propertyAddress(dx::Name("buttonDefaults"), index) + button->findProperty(dx::Name("Invoke"))->offset);
    }

    dx::UTexture* clientTexture(dx::UObject* menu, int index)
    {
        dx::UObject* texture = *reinterpret_cast<dx::UObject**>(menu->propertyAddress(dx::Name("clientTextures"), index));
        dx::objects::ensureLoaded(texture);
        return dx::cast<dx::UTexture>(texture);
    }

    void checkRow(dx::UTexture* result, int resultY, dx::UTexture* source, int sourceY)
    {
        std::vector<uint8_t> resultPixels = result->rgba(0);
        std::vector<uint8_t> sourcePixels = source->rgba(0);
        REQUIRE(result->width() == source->width());
        int mismatches = 0;
        for (uint32_t x = 0; x < source->width(); x++) {
            size_t from = (static_cast<size_t>(sourceY) * source->width() + x);
            size_t to = (static_cast<size_t>(resultY) * result->width() + x) * 4;
            bool transparent = source->mips[0].data[from] == 0;
            if (transparent ? resultPixels[to + 3] != 0 : (resultPixels[to + 3] != 255 || resultPixels[to] != sourcePixels[from * 4] || resultPixels[to + 1] != sourcePixels[from * 4 + 1] || resultPixels[to + 2] != sourcePixels[from * 4 + 2])) {
                mismatches++;
            }
        }
        CHECK(mismatches == 0);
    }
}

TEST_CASE("Inserting a menu button shifts the later buttons down one slot and repeats a slot of the background", "[data]")
{
    initializeEngine();
    dx::UClass* settings = dx::objects::findClass("DeusEx.MenuSettings");
    dx::UClass* target = dx::objects::findClass("DeusEx.MenuScreenOptions");
    dx::UObject* menu = dx::objects::createSubclass(settings, dx::Name("TestSettingsMenu"))->defaultObject;
    std::vector<dx::UTexture*> source;
    for (int i = 0; i < 4; i++) {
        source.push_back(clientTexture(menu, i));
    }
    dx::deusex::insertMenuButton(menu, 3, "Extra", target);

    CHECK(*reinterpret_cast<std::string*>(menu->propertyAddress(dx::Name("ButtonNames"), 2)) == "Game Options");
    CHECK(*reinterpret_cast<std::string*>(menu->propertyAddress(dx::Name("ButtonNames"), 3)) == "Extra");
    CHECK(*reinterpret_cast<std::string*>(menu->propertyAddress(dx::Name("ButtonNames"), 4)) == "Display");
    CHECK(*reinterpret_cast<std::string*>(menu->propertyAddress(dx::Name("ButtonNames"), 7)) == "Previous Menu");
    CHECK(buttonY(menu, 2) == 85);
    CHECK(buttonY(menu, 3) == 121);
    CHECK(buttonY(menu, 4) == 157);
    CHECK(buttonY(menu, 7) == 302);
    CHECK(buttonInvoke(menu, 3) == target);
    CHECK(buttonInvoke(menu, 4) == dx::objects::findClass("DeusEx.MenuScreenDisplay"));
    CHECK(*reinterpret_cast<int32_t*>(menu->propertyAddress(dx::Name("ClientHeight"))) == 344);
    CHECK(*reinterpret_cast<int32_t*>(menu->propertyAddress(dx::Name("textureRows"))) == 2);

    dx::UTexture* top = clientTexture(menu, 0);
    dx::UTexture* bottom = clientTexture(menu, 2);
    REQUIRE(top != source[0]);
    CHECK(top->height() == 256);
    CHECK(bottom->height() == 100);
    CHECK(clientTexture(menu, 1)->width() == 64);
    checkRow(top, 50, source[0], 50);
    checkRow(top, 130, source[0], 94);
    checkRow(top, 200, source[0], 164);
    checkRow(bottom, 44, source[2], 8);
    CHECK(clientTexture(settings->defaultObject, 0) == source[0]);
}

TEST_CASE("The modal mouse focus mode decides whether entering and leaving windows moves the focus", "[data]")
{
    dx::UObject* root = makeRoot();
    dx::UObject* modal = newChild(root, "Extension.ModalWindow");
    {
        dx::vm::Call call(modal, dx::Name("SetWindowAlignments"));
        call.byte(3).byte(3);
        call.run();
    }
    dx::UObject* first = newChild(modal, "Extension.ButtonWindow");
    setPos(first, 100, 100);
    setSize(first, 80, 20);
    dx::UObject* second = newChild(modal, "Extension.ButtonWindow");
    setPos(second, 100, 200);
    setSize(second, 80, 20);
    layout(800, 600);
    auto setMode = [modal](uint8_t mode) {
        dx::vm::Call call(modal, dx::Name("SetMouseFocusMode"));
        call.byte(mode);
        call.run();
    };

    dx::extension::mouseMove(110, 210);
    CHECK(objectCall(root, "GetFocusWindow") == second);
    dx::extension::mouseMove(110, 110);
    CHECK(objectCall(root, "GetFocusWindow") == first);
    dx::extension::mouseMove(400, 400);
    CHECK(objectCall(root, "GetFocusWindow") == first);

    setMode(3);
    dx::extension::mouseMove(110, 210);
    CHECK(objectCall(root, "GetFocusWindow") == second);
    dx::extension::mouseMove(400, 400);
    CHECK(objectCall(root, "GetFocusWindow") == nullptr);

    setMode(1);
    dx::extension::mouseMove(110, 110);
    CHECK(objectCall(root, "GetFocusWindow") == nullptr);
}
