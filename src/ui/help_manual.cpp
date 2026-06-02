#include "ui/help_manual.h"
#include "imgui.h"

namespace Core {

namespace {

constexpr const char* OVERVIEW_PARAGRAPHS[] = {
    "Capital Vice is a data-driven mafia simulation set across the five boroughs of New York City and parts of New Jersey. "
    "You create a character, enter a procedurally generated world, and observe simulation systems as they are built out.",
    "The current build is a foundational layer: character creation, world generation, map navigation, simulation timing, "
    "and save/load. Gameplay systems such as rackets, heat, and crew management are planned but not yet playable.",
    "Use Help > Manual from the top menu bar on any screen to open this guide. Topics on the left explain each system; "
    "select one to read how it works.",
};

constexpr const char* MAIN_MENU_PARAGRAPHS[] = {
    "The main menu appears when you launch the game. From here you can start a new game, load a saved game, or exit.",
    "New Game opens Character Creation. Load Game restores capitalvice_save.dat if it exists in the working directory; "
    "the button is disabled when no save file is found.",
    "Options is reserved for future settings. Exit Game closes the application.",
};

constexpr const char* NEW_GAME_FLOW_PARAGRAPHS[] = {
    "Starting a new game takes you through Character Creation, then into the simulation with a freshly generated world.",
    "Fill in your character on the left panel and review the live preview on the right. When ready, click Start New Game. "
    "The world is generated from a fixed seed, the simulation clock starts, and dockable panels appear for map and stats.",
    "Your character draft and derived player profile are stored for the session and included in save files.",
};

constexpr const char* CHARACTER_CREATION_PARAGRAPHS[] = {
    "Character Creation is a two-panel screen. The form on the left collects your choices; the preview on the right "
    "updates immediately with a written description and generation role summary.",
    "Fields: Name, Nationality, Heritage, Generation, Age (16–25), Background, and Starting Borough Preference. "
    "Each choice feeds the profile builder that runs in the background.",
    "Generation is not a power ladder—each generation trades strengths and weaknesses. Read the preview text to see "
    "how your picks shape your starting position before you commit.",
};

constexpr const char* GENERATION_TRADEOFFS_PARAGRAPHS[] = {
    "First Generation characters often have stronger ethnic network ties but weaker institutional legitimacy. "
    "Second Generation balances street knowledge with some mainstream access. Third Generation and beyond gain "
    "legitimacy and business paths but may lose raw street loyalty and in-group negotiation edge.",
    "The profile builder applies these tradeoffs as numeric modifiers on five trait axes. No generation is strictly "
    "better; each opens different long-term paths once gameplay systems connect to these values.",
};

constexpr const char* PLAYER_PROFILE_PARAGRAPHS[] = {
    "The Player Profile is derived from your Character Draft. It is foundational data—not yet wired to full gameplay—but "
    "defines how your character will interact with future systems.",
    "Five axes: Network Access (ethnic network, political machine, law enforcement channel, business association, "
    "import pipeline), Legitimacy (police attention decay, shell company ease, public job access, mainstream suspicion), "
    "Loyalty Bias (faction resistance, kin alliance, detection risk, individualistic loyalty), Cultural Competency "
    "(in-group and out-group negotiation, cross-ethnic penalty, language access, translate bonus), and Opportunity Paths "
    "(street crime, organizer, institutional, corporate).",
    "Values are floats adjusted by nationality, heritage, generation, age, background, and borough preference. "
    "They will drive rackets, alliances, and law enforcement reactions when those systems ship.",
};

constexpr const char* PROFILE_BUILDER_PARAGRAPHS[] = {
    "buildPlayerProfile() runs whenever you are on Character Creation or when a game starts or loads. "
    "It copies your draft into PlayerProfile and applies lookup tables for each trait axis.",
    "The written character description and generation role summary in the preview use the same draft data. "
    "Changing any combo or slider rebuilds the profile on the next frame—no manual refresh needed.",
    "In development builds, the dev console can dump or override profile fields for testing (see Developer Tools).",
};

constexpr const char* SIM_CLOCK_PARAGRAPHS[] = {
    "The simulation clock advances the game in fixed ticks, defaulting to 20 Hz (20 ticks per real-time second at 1x speed). "
    "Rendering runs every frame; simulation ticks only when enough accumulated time has passed.",
    "Pause (Space or Simulation menu) stops tick advancement. Step One Tick (S key or menu) advances exactly one tick "
    "while paused. Speed multiplier (0.25x to 4x) scales how fast real time converts to sim time.",
    "The Simulation panel shows tick count, ticks this frame, pause state, and speed. Use it to monitor timing during development.",
};

constexpr const char* SYSTEM_REGISTRY_PARAGRAPHS[] = {
    "Registered simulation systems receive onTick() each time the clock advances. The System Registry holds a flat list "
    "of ISimSystem implementations and runs them in order every tick.",
    "Currently only DebugSystem is registered—it records the last processed tick for verification. Future systems "
    "(economy, heat, AI, rackets) will register here without changing the clock loop.",
    "The Simulation panel lists active systems and their last tick count so you can confirm the pipeline is running.",
};

constexpr const char* WORLD_DATA_PARAGRAPHS[] = {
    "The world is a 512×512 tile grid split into 32×32 chunks (16×16 chunks total). Tile data uses a structure-of-arrays "
    "layout in ChunkStore: region, terrain, elevation, and flags per tile.",
    "Coordinates are world-space (x, y). Chunks activate as data is generated or loaded. The world bounds and chunk "
    "math live in WorldConfig.",
    "Five playable borough regions plus water and non-playable areas are stored per tile. The map renderer and Tile "
    "Inspector read from this store—there is no separate visual map file at runtime.",
};

constexpr const char* PROCGEN_PARAGRAPHS[] = {
    "On new game, WorldGenerator fills ChunkStore from a world seed. A baked run-length-encoded borough mask defines "
    "which region each tile belongs to, based on a reference NYC layout.",
    "Streets and elevation passes run after the mask. Terrain types include water, road, building, park, plaza, and open land. "
    "The same seed always produces the same world.",
    "Procgen runs once at start (or restore from save on load)—not during the frame loop.",
};

constexpr const char* MAP_VIEWPORT_PARAGRAPHS[] = {
    "The Map Viewport panel draws visible tiles colored by borough. MapCamera controls pan, zoom, and screen-to-tile picking.",
    "Scroll the mouse wheel over the map to zoom toward the cursor. Drag with left, middle, or right mouse button to pan. "
    "Hover highlights a tile; left-click selects it for the Tile Inspector.",
    "On first open, the camera fits the full world to the panel. Zoom level is shown in the on-map hint overlay.",
};

constexpr const char* TILE_INSPECTOR_PARAGRAPHS[] = {
    "Tile Inspector shows data for the hovered or selected map tile. Click a tile in the Map Viewport to lock selection; "
    "otherwise it follows the mouse.",
    "Displayed fields: world coordinates, chunk index and local tile coords, borough name, terrain type, elevation, "
    "and whether the chunk is active.",
    "Use this panel to verify procgen output and borough boundaries while exploring the map.",
};

constexpr const char* BOROUGHS_PANEL_PARAGRAPHS[] = {
    "The Boroughs panel lists all playable regions with full and short names (Manhattan, Brooklyn, Queens, Bronx, Staten Island, "
    "and New Jersey entries as defined in RegionTable).",
    "It is a reference list for now—selecting a borough does not move the camera. Future versions may filter map highlights "
    "or jump the camera to borough centers.",
};

constexpr const char* DOCKING_UI_PARAGRAPHS[] = {
    "In-game panels (Simulation, Boroughs, Tile Inspector, Map Viewport, and this Manual) use ImGui docking. "
    "Drag panel title bars to dock, tab, or float windows.",
    "Layout persists to capitalvice_layout.ini in the working directory between sessions. "
    "Minimum window size is 960×540 to keep panels usable.",
};

constexpr const char* VIEW_MENU_PARAGRAPHS[] = {
    "The View menu (in-game only) provides Reset Panel Layout, which clears the saved docking layout and restores defaults.",
    "Use this if panels are lost off-screen or you want a clean arrangement. Reset does not affect save game data.",
};

constexpr const char* SAVE_LOAD_PARAGRAPHS[] = {
    "Save and load use a binary file capitalvice_save.dat in the working directory. Format version 2 (magic CVSV). "
    "Version 1 saves are rejected.",
    "Saved state includes: world seed, full Character Draft, simulation tick count, pause/speed/accumulator, map camera, "
    "and all tile region/terrain/elevation/flags data.",
    "Save: File > Save Game or Ctrl+S while in-game. Load: File > Load Game in-game, or Load Game from the main menu. "
    "Status messages appear in the Simulation panel after save/load attempts.",
};

#if defined(CAPITALVICE_DEV_CONSOLE)
constexpr const char* DEV_CONSOLE_PARAGRAPHS[] = {
    "The developer console is available only in dev builds. Press F12 to toggle a separate ImGui viewport window.",
    "Type commands in the input line and press Enter. Commands include help, log clear, profile dump, draft show, "
    "profile set <field> <value>, and network show. Tab completion is not implemented—type full command names.",
    "Use the console to inspect or tweak profile values during character creation testing. It does not ship in release builds.",
};
#endif

constexpr const char* COMING_SOON_PARAGRAPHS[] = {
    "Planned systems not yet in this build include: racket ownership and profit, heat and law enforcement pressure, "
    "crew and gangster management, extortion and territory control, and narrative events tied to profile traits.",
    "The Player Profile axes and world map are designed to support these features. New manual topics will appear here "
    "as each system is implemented.",
};

struct HelpManualTopicEntry {
    const char* id;
    const char* chapterTitle;
    const char* title;
    const char* summary;
    const char* const* paragraphs;
    int32_t paragraphCount;
};

constexpr HelpManualTopicEntry HELP_MANUAL_TOPICS[] = {
    {"overview", "Getting Started", "Overview", "What Capital Vice is today",
        OVERVIEW_PARAGRAPHS, static_cast<int32_t>(sizeof(OVERVIEW_PARAGRAPHS) / sizeof(OVERVIEW_PARAGRAPHS[0]))},
    {"main_menu", "Getting Started", "Main Menu", "Launch screen actions",
        MAIN_MENU_PARAGRAPHS, static_cast<int32_t>(sizeof(MAIN_MENU_PARAGRAPHS) / sizeof(MAIN_MENU_PARAGRAPHS[0]))},
    {"new_game", "Getting Started", "New Game Flow", "From creation to simulation",
        NEW_GAME_FLOW_PARAGRAPHS, static_cast<int32_t>(sizeof(NEW_GAME_FLOW_PARAGRAPHS) / sizeof(NEW_GAME_FLOW_PARAGRAPHS[0]))},
    {"char_creation", "Character and Profile", "Character Creation", "Draft fields and preview",
        CHARACTER_CREATION_PARAGRAPHS, static_cast<int32_t>(sizeof(CHARACTER_CREATION_PARAGRAPHS) / sizeof(CHARACTER_CREATION_PARAGRAPHS[0]))},
    {"generation", "Character and Profile", "Generation Tradeoffs", "Not a power ladder",
        GENERATION_TRADEOFFS_PARAGRAPHS, static_cast<int32_t>(sizeof(GENERATION_TRADEOFFS_PARAGRAPHS) / sizeof(GENERATION_TRADEOFFS_PARAGRAPHS[0]))},
    {"player_profile", "Character and Profile", "Player Profile", "Five trait axes",
        PLAYER_PROFILE_PARAGRAPHS, static_cast<int32_t>(sizeof(PLAYER_PROFILE_PARAGRAPHS) / sizeof(PLAYER_PROFILE_PARAGRAPHS[0]))},
    {"profile_builder", "Character and Profile", "Profile Builder", "Draft to derived traits",
        PROFILE_BUILDER_PARAGRAPHS, static_cast<int32_t>(sizeof(PROFILE_BUILDER_PARAGRAPHS) / sizeof(PROFILE_BUILDER_PARAGRAPHS[0]))},
    {"sim_clock", "Simulation", "Simulation Clock", "Fixed tick rate and controls",
        SIM_CLOCK_PARAGRAPHS, static_cast<int32_t>(sizeof(SIM_CLOCK_PARAGRAPHS) / sizeof(SIM_CLOCK_PARAGRAPHS[0]))},
    {"system_registry", "Simulation", "System Registry", "Tick pipeline",
        SYSTEM_REGISTRY_PARAGRAPHS, static_cast<int32_t>(sizeof(SYSTEM_REGISTRY_PARAGRAPHS) / sizeof(SYSTEM_REGISTRY_PARAGRAPHS[0]))},
    {"world_data", "World and Map", "World Data", "512x512 chunk store",
        WORLD_DATA_PARAGRAPHS, static_cast<int32_t>(sizeof(WORLD_DATA_PARAGRAPHS) / sizeof(WORLD_DATA_PARAGRAPHS[0]))},
    {"procgen", "World and Map", "World Generation", "Borough mask and terrain",
        PROCGEN_PARAGRAPHS, static_cast<int32_t>(sizeof(PROCGEN_PARAGRAPHS) / sizeof(PROCGEN_PARAGRAPHS[0]))},
    {"map_viewport", "World and Map", "Map Viewport", "Pan, zoom, and pick",
        MAP_VIEWPORT_PARAGRAPHS, static_cast<int32_t>(sizeof(MAP_VIEWPORT_PARAGRAPHS) / sizeof(MAP_VIEWPORT_PARAGRAPHS[0]))},
    {"tile_inspector", "World and Map", "Tile Inspector", "Per-tile details",
        TILE_INSPECTOR_PARAGRAPHS, static_cast<int32_t>(sizeof(TILE_INSPECTOR_PARAGRAPHS) / sizeof(TILE_INSPECTOR_PARAGRAPHS[0]))},
    {"boroughs", "World and Map", "Boroughs Panel", "Region reference list",
        BOROUGHS_PANEL_PARAGRAPHS, static_cast<int32_t>(sizeof(BOROUGHS_PANEL_PARAGRAPHS) / sizeof(BOROUGHS_PANEL_PARAGRAPHS[0]))},
    {"docking", "Interface", "Docking Panels", "Layout and persistence",
        DOCKING_UI_PARAGRAPHS, static_cast<int32_t>(sizeof(DOCKING_UI_PARAGRAPHS) / sizeof(DOCKING_UI_PARAGRAPHS[0]))},
    {"view_menu", "Interface", "View Menu", "Reset panel layout",
        VIEW_MENU_PARAGRAPHS, static_cast<int32_t>(sizeof(VIEW_MENU_PARAGRAPHS) / sizeof(VIEW_MENU_PARAGRAPHS[0]))},
    {"save_load", "Persistence", "Save and Load", "capitalvice_save.dat v2",
        SAVE_LOAD_PARAGRAPHS, static_cast<int32_t>(sizeof(SAVE_LOAD_PARAGRAPHS) / sizeof(SAVE_LOAD_PARAGRAPHS[0]))},
#if defined(CAPITALVICE_DEV_CONSOLE)
    {"dev_console", "Developer Tools", "Dev Console", "F12 debug commands",
        DEV_CONSOLE_PARAGRAPHS, static_cast<int32_t>(sizeof(DEV_CONSOLE_PARAGRAPHS) / sizeof(DEV_CONSOLE_PARAGRAPHS[0]))},
#endif
    {"coming_soon", "Coming Soon", "Planned Gameplay", "Future systems",
        COMING_SOON_PARAGRAPHS, static_cast<int32_t>(sizeof(COMING_SOON_PARAGRAPHS) / sizeof(COMING_SOON_PARAGRAPHS[0]))},
};

constexpr int32_t HELP_MANUAL_TOPIC_COUNT = static_cast<int32_t>(sizeof(HELP_MANUAL_TOPICS) / sizeof(HELP_MANUAL_TOPICS[0]));

} // namespace

int32_t getHelpManualTopicCount() {
    return HELP_MANUAL_TOPIC_COUNT;
}

const HelpManualTopic* getHelpManualTopic(int32_t topicIndex) {
    if (topicIndex < 0 || topicIndex >= HELP_MANUAL_TOPIC_COUNT) {
        return nullptr;
    }
    const HelpManualTopicEntry& entry = HELP_MANUAL_TOPICS[topicIndex];
    static HelpManualTopic topic{};
    topic.id = entry.id;
    topic.chapterTitle = entry.chapterTitle;
    topic.title = entry.title;
    topic.summary = entry.summary;
    topic.paragraphs = entry.paragraphs;
    topic.paragraphCount = entry.paragraphCount;
    return &topic;
}

void renderHelpManualWindow(HelpManualState& state) {
    if (!state.isOpen) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(720.0f, 480.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Capital Vice Manual", &state.isOpen, ImGuiWindowFlags_None)) {
        ImGui::End();
        return;
    }
    const float sidebarWidth = ImGui::GetContentRegionAvail().x * 0.35f;
    ImGui::BeginChild("ManualSidebar", ImVec2(sidebarWidth, 0.0f), true);
    int32_t topicIndex = 0;
    while (topicIndex < HELP_MANUAL_TOPIC_COUNT) {
        const char* chapterTitle = HELP_MANUAL_TOPICS[topicIndex].chapterTitle;
        ImGui::PushID(chapterTitle);
        if (ImGui::CollapsingHeader(chapterTitle, ImGuiTreeNodeFlags_DefaultOpen)) {
            while (topicIndex < HELP_MANUAL_TOPIC_COUNT && HELP_MANUAL_TOPICS[topicIndex].chapterTitle == chapterTitle) {
                const HelpManualTopicEntry& entry = HELP_MANUAL_TOPICS[topicIndex];
                ImGui::PushID(topicIndex);
                const bool isSelected = state.selectedTopicIndex == topicIndex;
                if (ImGui::Selectable(entry.title, isSelected)) {
                    state.selectedTopicIndex = topicIndex;
                }
                if (entry.summary[0] != '\0') {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                    ImGui::TextWrapped("%s", entry.summary);
                    ImGui::PopStyleColor();
                }
                ImGui::PopID();
                ++topicIndex;
            }
        } else {
            while (topicIndex < HELP_MANUAL_TOPIC_COUNT && HELP_MANUAL_TOPICS[topicIndex].chapterTitle == chapterTitle) {
                ++topicIndex;
            }
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("ManualContent", ImVec2(0.0f, 0.0f), true);
    if (state.selectedTopicIndex < 0 || state.selectedTopicIndex >= HELP_MANUAL_TOPIC_COUNT) {
        ImGui::TextDisabled("Select a topic on the left.");
    } else {
        const HelpManualTopicEntry& entry = HELP_MANUAL_TOPICS[state.selectedTopicIndex];
        ImGui::TextUnformatted(entry.chapterTitle);
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextUnformatted(entry.summary);
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::TextUnformatted(entry.title);
        ImGui::Spacing();
        for (int32_t paragraphIndex = 0; paragraphIndex < entry.paragraphCount; ++paragraphIndex) {
            ImGui::TextWrapped("%s", entry.paragraphs[paragraphIndex]);
            ImGui::Spacing();
        }
    }
    ImGui::EndChild();
    ImGui::End();
}

} // namespace Core
