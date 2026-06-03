#include "ui/help_manual.h"
#include "imgui.h"
#include <cstring>

namespace Core {

namespace {

constexpr const char* P_OVERVIEW[] = {
    "Capital Vice is a data-driven mafia simulation across NYC boroughs and parts of New Jersey.",
    "This build is a foundational layer: character creation, world generation, map navigation, simulation timing, and save/load.",
    "Open Help > Manual from the top menu bar on any screen. Hold Ctrl for inspect mode to click UI elements for quick help.",
};

constexpr const char* P_MAIN_MENU[] = {
    "The main menu is the launch screen. Use New Game, Load Game, or Exit Game.",
    "Load Game is disabled when capitalvice_save.dat is missing from the working directory.",
};

constexpr const char* P_NEW_GAME[] = {
    "New Game opens Character Creation, then generates the world when you click Start New Game.",
    "Your character draft and derived profile are saved with the game.",
};

constexpr const char* P_CONTEXT_HELP[] = {
    "Hold Ctrl to enter inspect mode. A question-mark badge follows the cursor.",
    "Click any highlighted UI element to see a short explanation and jump to the matching Manual topic.",
    "Release Ctrl to return to normal play. Hover tooltips still work on the Character panel without Ctrl.",
};

constexpr const char* P_CHAR_CREATION[] = {
    "Character Creation uses a two-panel layout: choices on the left, live preview on the right.",
    "Every field updates the preview and rebuilds your foundational profile in the background.",
};

constexpr const char* P_CHAR_NAME[] = {
    "Your display name in descriptions and the Character panel. Up to 31 characters.",
};

constexpr const char* P_CHAR_GENERATION[] = {
    "Generation is a tradeoff, not a power ladder. Each tier favors different networks and paths.",
    "Read the preview role summary before committing.",
};

constexpr const char* P_PROFILE_OVERVIEW[] = {
    "The Player Profile is derived from your Character Draft at runtime.",
    "It stores foundational modifiers used by future gameplay systems. It is not the full simulation model.",
};

constexpr const char* P_TRAIT_AXES[] = {
    "Trait axes are one category of foundational profile data. They describe starting tendencies, not final outcomes.",
    "More profile layers and gameplay systems will be added later. Axes are not the only systems in the game.",
    "See each axis topic under Character / Trait Axes for individual breakdowns.",
};

constexpr const char* P_PROFILE_BUILDER[] = {
    "The profile builder runs on Character Creation, new game, and load.",
    "It copies your draft, applies generation base tables, then nudges from heritage, nationality, age, and background.",
};

constexpr const char* P_CHARACTER_PANEL[] = {
    "The Character panel appears in-game after you start. It shows identity, description, and foundational trait bars.",
    "Hover any row for a tooltip. Hold Ctrl and click a row to open detailed help with a Manual link.",
};

constexpr const char* P_AXIS_NETWORK[] = {
    "Network Access measures reach into ethnic enclaves, politics, law enforcement, business circles, and import routes.",
    "High values unlock contacts; low values mean you must build networks through play.",
};

constexpr const char* P_STAT_ETHNIC_NETWORK[] = {
    "Ties to your heritage community: introductions, favors, and word-of-mouth inside the enclave.",
};

constexpr const char* P_STAT_POLITICAL_MACHINE[] = {
    "Access to ward bosses, party operatives, and patronage jobs that grease permits and protection.",
};

constexpr const char* P_STAT_LAW_CHANNEL[] = {
    "Informal channels with beat cops, detectives, or clerks who can delay or soften attention.",
};

constexpr const char* P_STAT_BUSINESS_ASSOC[] = {
    "Legitimate business associations: trade groups, suppliers, and front-company partners.",
};

constexpr const char* P_STAT_IMPORT_PIPELINE[] = {
    "Smuggling and import routes: docks, freight, and cross-border supply lines.",
};

constexpr const char* P_AXIS_LEGITIMACY[] = {
    "Legitimacy is how clean you look to institutions and the public versus how suspicious you appear.",
};

constexpr const char* P_STAT_POLICE_DECAY[] = {
    "How quickly heat and police attention fade when you stay quiet.",
};

constexpr const char* P_STAT_SHELL_COMPANY[] = {
    "Ease of standing up shell companies and paper fronts for laundering.",
};

constexpr const char* P_STAT_PUBLIC_JOB[] = {
    "Access to respectable public-facing employment as cover.",
};

constexpr const char* P_STAT_MAINSTREAM_SUSPICION[] = {
    "Baseline suspicion from mainstream society and law enforcement profiling.",
};

constexpr const char* P_AXIS_LOYALTY[] = {
    "Loyalty Bias shapes who you trust, who trusts you, and how faction pressure lands on you.",
};

constexpr const char* P_STAT_FACTION_RESIST[] = {
    "Resistance to pressure from rival ethnic or criminal factions.",
};

constexpr const char* P_STAT_KIN_ALLIANCE[] = {
    "Preference to ally with family and kin networks over outsiders.",
};

constexpr const char* P_STAT_DETECTION_RISK[] = {
    "Risk that mainstream institutions detect organized ties.",
};

constexpr const char* P_STAT_INDIVIDUAL_LOYALTY[] = {
    "Willingness to break from group expectations for personal gain.",
};

constexpr const char* P_AXIS_CULTURE[] = {
    "Cultural Competency covers negotiation inside and outside your group, language, and translation edges.",
};

constexpr const char* P_STAT_IN_GROUP_NEG[] = {
    "Negotiation skill with people who share your heritage or neighborhood culture.",
};

constexpr const char* P_STAT_OUT_GROUP_NEG[] = {
    "Negotiation skill with outsiders, officials, and other ethnic groups.",
};

constexpr const char* P_STAT_CROSS_ETHNIC[] = {
    "Penalty when working across ethnic lines without shared language or trust.",
};

constexpr const char* P_STAT_LANGUAGE[] = {
    "Access to English and other languages needed for mainstream deals.",
};

constexpr const char* P_STAT_TRANSLATE[] = {
    "Bonus when using intermediaries or translators to bridge groups.",
};

constexpr const char* P_AXIS_OPPORTUNITY[] = {
    "Opportunity Paths hint which criminal and legitimate career arcs fit your starting profile.",
};

constexpr const char* P_STAT_STREET_CRIME[] = {
    "Aptitude for street-level crime: theft, enforcement, and quick cash rackets.",
};

constexpr const char* P_STAT_ORGANIZER[] = {
    "Aptitude for organizing crews, neighborhoods, and labor leverage.",
};

constexpr const char* P_STAT_INSTITUTIONAL[] = {
    "Aptitude for unions, political offices, and institutional corruption.",
};

constexpr const char* P_STAT_CORPORATE[] = {
    "Aptitude for white-collar infiltration, finance, and corporate fronts.",
};

constexpr const char* P_SIM_CLOCK[] = {
    "The simulation clock runs at 20 Hz by default. Rendering is every frame; ticks batch via an accumulator.",
    "Space toggles pause. S steps one tick while paused. Speed multiplier scales 0.25x to 4x.",
};

constexpr const char* P_SYSTEM_REGISTRY[] = {
    "Each tick, the System Registry calls every registered ISimSystem in order.",
    "DebugSystem is the current placeholder. Future economy, heat, and racket systems register here.",
};

constexpr const char* P_WORLD_DATA[] = {
    "The world is 512x512 tiles in a structure-of-arrays ChunkStore with region, terrain, elevation, and flags.",
};

constexpr const char* P_PROCGEN[] = {
    "WorldGenerator fills tiles from a seed using a baked borough mask, then streets and elevation passes.",
};

constexpr const char* P_MAP_VIEWPORT[] = {
    "Scroll to zoom, drag to pan, hover to highlight tiles, and click to inspect tiles or district landmarks.",
    "Zoom in to see district labels. Landmarks show a marker and name; LaGuardia displays as LGA with the full name on hover.",
};

constexpr const char* P_TILE_INSPECTOR[] = {
    "Shows borough, terrain, chunk coords, and elevation for the hovered or selected map tile.",
};

constexpr const char* P_LANDMARKS_OVERVIEW[] = {
    "District landmarks are fixed strategic nodes placed on specific map tiles across all boroughs.",
    "Each landmark inherits its borough from the underlying tile region data. Add new entries in landmark_table.cpp.",
    "Click a landmark on the map to open the District panel. Labels appear when zoomed in.",
};

constexpr const char* P_DISTRICT_PANEL[] = {
    "The District panel opens when you click a landmark. It shows the full district name, borough, tile coords, and foundational stats.",
    "LaGuardia Airport uses the map label LGA but the panel and hover tooltip show the full name.",
};

constexpr const char* P_LANDMARK_CONTROL[] = {
    "Districts are always-hot control nodes: higher baseline heat and the highest capture difficulty.",
    "Holding districts weighs more toward borough-wide influence than ordinary tiles (see LANDMARK_BOROUGH_INFLUENCE_WEIGHT).",
    "The District panel shows live crime heat and difficulty adjusted by player influence on the landmark tile.",
};

constexpr const char* P_BOROUGHS[] = {
    "Lists playable boroughs with live economic health, population, crime, and unemployment bars.",
    "Metrics refresh when BoroughVitalitySystem runs a full rollup every 40 ticks.",
};

constexpr const char* P_BOROUGH_VITALITY_OVERVIEW[] = {
    "Tiles store signals (weight, population, pressures). Boroughs expose the player-facing scoreboard.",
    "Simulation updates borough population with stock-flow math, then redistributes to tiles by economic weight on rollup.",
};

constexpr const char* P_TILE_ECONOMIC_WEIGHT[] = {
    "Economic weight is mostly static from terrain and borough, with Manhattan and landmarks boosting contribution.",
    "Water tiles have zero weight and do not receive population on redistribution.",
};

constexpr const char* P_BOROUGH_HEALTH_FORMULA[] = {
    "Economic health blends weighted business vitality, crime penalty, law stability, player boost, and opposition drag.",
    "Health is clamped 0-100 and shown in Simulation and Boroughs panels after each rollup pass.",
};

constexpr const char* P_POPULATION_MODEL[] = {
    "Births, deaths, and migration run at borough level using economic health and crime rate, not per-tile demography.",
    "When targets diverge from tile sums, population is redistributed proportionally to economic weight.",
};

constexpr const char* P_CRIME_LAW_OPPOSITION[] = {
    "Crime, law, business vitality, player influence, and opposition are uint8 tile pressures rolled up by weight.",
    "Opponent stub periodically raises law pressure and opposition while eroding player influence in a random borough.",
};

constexpr const char* P_DISTRICT_HOT_NODES[] = {
    "Landmark tiles seed max economic weight, higher crime heat, and boosted business vitality within heat radius.",
    "District panel heat and difficulty read live tile pressures; borough lines show rollup context.",
};

constexpr const char* P_DOCKING[] = {
    "Panels dock and tab via ImGui. Layout saves to capitalvice_layout.ini between sessions.",
};

constexpr const char* P_VIEW_MENU[] = {
    "Reset Panel Layout restores default docking when windows are lost or messy.",
};

constexpr const char* P_SAVE_LOAD[] = {
    "Binary save capitalvice_save.dat (v3). Stores draft, sim clock, camera, geography, and tile vitality arrays.",
    "Save with Ctrl+S or File menu. Load from File menu or main menu. Older save versions are rejected.",
};

#if defined(CAPITALVICE_DEV_CONSOLE)
constexpr const char* P_DEV_CONSOLE[] = {
    "Dev builds only. F12 toggles a command console for profile inspection and tweaks.",
};
#endif

constexpr const char* P_COMING_SOON[] = {
    "Planned: rackets, heat, crew management, territory, and events tied to profile layers beyond trait axes.",
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
    {"overview", "Getting Started", "Overview", "What this build includes", P_OVERVIEW, 3},
    {"main_menu", "Getting Started", "Main Menu", "Launch actions", P_MAIN_MENU, 2},
    {"new_game", "Getting Started", "New Game Flow", "Creation to simulation", P_NEW_GAME, 2},
    {"context_help", "Getting Started", "Inspect Mode (Ctrl)", "Click-to-help cursor", P_CONTEXT_HELP, 3},

    {"char_creation", "Character / Identity", "Character Creation Screen", "Two-panel setup", P_CHAR_CREATION, 2},
    {"char_name", "Character / Identity", "Name Field", "Display name", P_CHAR_NAME, 1},
    {"char_generation", "Character / Identity", "Generation Choice", "Tradeoffs by generation", P_CHAR_GENERATION, 2},

    {"profile_overview", "Character / Profile", "Player Profile", "Foundational derived data", P_PROFILE_OVERVIEW, 2},
    {"trait_axes", "Character / Profile", "Trait Axes Overview", "One profile layer among many", P_TRAIT_AXES, 3},
    {"profile_builder", "Character / Profile", "Profile Builder", "Draft to numbers", P_PROFILE_BUILDER, 2},
    {"character_panel", "Character / Profile", "Character Panel", "In-game stats UI", P_CHARACTER_PANEL, 2},

    {"axis_network", "Character / Trait Axes", "Network Access Axis", "Contacts and pipelines", P_AXIS_NETWORK, 2},
    {"stat_ethnic_network", "Character / Trait Axes", "Ethnic Network", "Enclave ties", P_STAT_ETHNIC_NETWORK, 1},
    {"stat_political_machine", "Character / Trait Axes", "Political Machine", "Patronage access", P_STAT_POLITICAL_MACHINE, 1},
    {"stat_law_channel", "Character / Trait Axes", "Law Enforcement Channel", "Soft influence on cops", P_STAT_LAW_CHANNEL, 1},
    {"stat_business_assoc", "Character / Trait Axes", "Business Association", "Trade group ties", P_STAT_BUSINESS_ASSOC, 1},
    {"stat_import_pipeline", "Character / Trait Axes", "Import Pipeline", "Smuggling routes", P_STAT_IMPORT_PIPELINE, 1},

    {"axis_legitimacy", "Character / Trait Axes", "Legitimacy Axis", "Clean cover vs suspicion", P_AXIS_LEGITIMACY, 1},
    {"stat_police_decay", "Character / Trait Axes", "Police Attention Decay", "Heat fade rate", P_STAT_POLICE_DECAY, 1},
    {"stat_shell_company", "Character / Trait Axes", "Shell Company Ease", "Front company setup", P_STAT_SHELL_COMPANY, 1},
    {"stat_public_job", "Character / Trait Axes", "Public Job Access", "Respectable cover jobs", P_STAT_PUBLIC_JOB, 1},
    {"stat_mainstream_suspicion", "Character / Trait Axes", "Mainstream Suspicion", "Baseline scrutiny", P_STAT_MAINSTREAM_SUSPICION, 1},

    {"axis_loyalty", "Character / Trait Axes", "Loyalty Bias Axis", "Trust and faction pressure", P_AXIS_LOYALTY, 1},
    {"stat_faction_resist", "Character / Trait Axes", "Faction Resistance", "Pushback on rivals", P_STAT_FACTION_RESIST, 1},
    {"stat_kin_alliance", "Character / Trait Axes", "Kin Alliance", "Family-first loyalty", P_STAT_KIN_ALLIANCE, 1},
    {"stat_detection_risk", "Character / Trait Axes", "Detection Risk", "Organized ties exposure", P_STAT_DETECTION_RISK, 1},
    {"stat_individual_loyalty", "Character / Trait Axes", "Individualistic Loyalty", "Personal over group", P_STAT_INDIVIDUAL_LOYALTY, 1},

    {"axis_culture", "Character / Trait Axes", "Cultural Competency Axis", "In-group vs out-group skill", P_AXIS_CULTURE, 1},
    {"stat_in_group_neg", "Character / Trait Axes", "In-Group Negotiation", "Heritage community deals", P_STAT_IN_GROUP_NEG, 1},
    {"stat_out_group_neg", "Character / Trait Axes", "Out-Group Negotiation", "Outsider deals", P_STAT_OUT_GROUP_NEG, 1},
    {"stat_cross_ethnic", "Character / Trait Axes", "Cross-Ethnic Penalty", "Cross-line friction", P_STAT_CROSS_ETHNIC, 1},
    {"stat_language", "Character / Trait Axes", "Language Access", "Mainstream language", P_STAT_LANGUAGE, 1},
    {"stat_translate", "Character / Trait Axes", "Translate Bonus", "Interpreter edge", P_STAT_TRANSLATE, 1},

    {"axis_opportunity", "Character / Trait Axes", "Opportunity Paths Axis", "Career arc hints", P_AXIS_OPPORTUNITY, 1},
    {"stat_street_crime", "Character / Trait Axes", "Street Crime Path", "Street-level work", P_STAT_STREET_CRIME, 1},
    {"stat_organizer", "Character / Trait Axes", "Organizer Path", "Crew and neighborhood", P_STAT_ORGANIZER, 1},
    {"stat_institutional", "Character / Trait Axes", "Institutional Path", "Unions and offices", P_STAT_INSTITUTIONAL, 1},
    {"stat_corporate", "Character / Trait Axes", "Corporate Path", "White-collar infiltration", P_STAT_CORPORATE, 1},

    {"sim_clock", "Simulation", "Simulation Clock", "20 Hz tick loop", P_SIM_CLOCK, 2},
    {"system_registry", "Simulation", "System Registry", "Tick pipeline", P_SYSTEM_REGISTRY, 2},
    {"borough_vitality_overview", "Simulation / Borough Vitality", "Borough Vitality Overview", "Tiles as signals, boroughs as scoreboard", P_BOROUGH_VITALITY_OVERVIEW, 2},
    {"tile_economic_weight", "Simulation / Borough Vitality", "Tile Economic Weight", "Static contribution to rollup", P_TILE_ECONOMIC_WEIGHT, 2},
    {"borough_health_formula", "Simulation / Borough Vitality", "Borough Health Formula", "What moves economic health", P_BOROUGH_HEALTH_FORMULA, 2},
    {"population_model", "Simulation / Borough Vitality", "Population Model", "Borough stock-flow", P_POPULATION_MODEL, 2},
    {"crime_law_opposition", "Simulation / Borough Vitality", "Crime, Law, and Opposition", "Competing pressures", P_CRIME_LAW_OPPOSITION, 2},
    {"district_hot_nodes", "Simulation / Borough Vitality", "District Hot Nodes", "Landmark vitality seeding", P_DISTRICT_HOT_NODES, 2},

    {"world_data", "World and Map", "World Data", "512x512 ChunkStore", P_WORLD_DATA, 1},
    {"procgen", "World and Map", "World Generation", "Seed and borough mask", P_PROCGEN, 1},
    {"map_viewport", "World and Map", "Map Viewport", "Pan zoom pick", P_MAP_VIEWPORT, 2},
    {"tile_inspector", "World and Map", "Tile Inspector", "Single tile details", P_TILE_INSPECTOR, 1},
    {"landmarks_overview", "World and Map / Districts", "District Landmarks", "Strategic map nodes", P_LANDMARKS_OVERVIEW, 3},
    {"district_panel", "World and Map / Districts", "District Panel", "Landmark stats UI", P_DISTRICT_PANEL, 2},
    {"landmark_control", "World and Map / Districts", "District Control Model", "Hot high-difficulty nodes", P_LANDMARK_CONTROL, 3},
    {"boroughs", "World and Map", "Boroughs Panel", "Live borough vitality bars", P_BOROUGHS, 2},

    {"docking", "Interface", "Docking Panels", "Layout persistence", P_DOCKING, 1},
    {"view_menu", "Interface", "View Menu", "Reset layout", P_VIEW_MENU, 1},

    {"save_load", "Persistence", "Save and Load", "capitalvice_save.dat v3", P_SAVE_LOAD, 2},

#if defined(CAPITALVICE_DEV_CONSOLE)
    {"dev_console", "Developer Tools", "Dev Console", "F12 commands", P_DEV_CONSOLE, 1},
#endif
    {"coming_soon", "Coming Soon", "Planned Gameplay", "Future systems", P_COMING_SOON, 1},
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

int32_t findHelpManualTopicIndexById(const char* topicId) {
    if (topicId == nullptr || topicId[0] == '\0') {
        return -1;
    }
    for (int32_t topicIndex = 0; topicIndex < HELP_MANUAL_TOPIC_COUNT; ++topicIndex) {
        if (std::strcmp(HELP_MANUAL_TOPICS[topicIndex].id, topicId) == 0) {
            return topicIndex;
        }
    }
    return -1;
}

void openHelpManualTopic(HelpManualState& state, const char* topicId) {
    const int32_t topicIndex = findHelpManualTopicIndexById(topicId);
    if (topicIndex < 0) {
        return;
    }
    state.isOpen = true;
    state.selectedTopicIndex = topicIndex;
}

void renderHelpManualWindow(HelpManualState& state) {
    if (!state.isOpen) {
        return;
    }
    ImGui::SetNextWindowSize(ImVec2(780.0f, 520.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Capital Vice Manual", &state.isOpen, ImGuiWindowFlags_None)) {
        ImGui::End();
        return;
    }
    ImGui::TextDisabled("Hold Ctrl and click UI elements in-game for quick help links.");
    ImGui::Separator();
    const float sidebarWidth = ImGui::GetContentRegionAvail().x * 0.38f;
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
