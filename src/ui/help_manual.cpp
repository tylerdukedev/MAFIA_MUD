#include "ui/help_manual.h"
#include "imgui.h"
#include <cstring>

namespace Core {

namespace {

constexpr const char* P_OVERVIEW[] = {
    "Welcome to Capital Vice — a street-level life sim across NYC and New Jersey where you build cover, cash, and eventually an organization.",
    "You create a character, pick a borough, find housing, interview for jobs at blue businesses, and manage contacts who judge you by opinion, trust, and respect.",
    "Open Help > Manual from the menu bar anytime. Hold Ctrl to inspect UI elements for quick tips.",
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
    "Character Creation uses two columns: choices on the left, background profile on the right.",
    "Enter first, middle (optional), and last name. Extra spaces are stripped and names are title-cased.",
    "Changing heritage, nationality, generation, background, or borough rerolls the family tree preview with new names.",
    "Start New Game opens your official record: Immigration Intake (immigrant) or Certificate of Birth (all other generations).",
};

constexpr const char* P_CHAR_NAME[] = {
    "Use separate first, middle (optional), and last name fields. Only letters, spaces, hyphens, and apostrophes are kept.",
    "Both first and last name are required before you can continue.",
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
    "After you begin, the Character panel shows your name, cash on hand, income rates, starting city, and trait bars.",
    "Legit income is zero until you are employed; crime income still comes from owned cities and your background.",
    "Hover rows for tooltips; Ctrl+click opens deeper help. A broke warning appears when cash is very low.",
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
    "Current order: CityControlSystem (claim events), EconomySystem (income and starting borough influence), BoroughVitalitySystem (rollup), DebugSystem.",
    "The Simulation panel lists registered systems and last tick counts for debugging.",
};

constexpr const char* P_WORLD_DATA[] = {
    "The world is 512x512 tiles in a structure-of-arrays ChunkStore with region, terrain, elevation, and flags.",
};

constexpr const char* P_PROCGEN[] = {
    "WorldGenerator fills tiles from a seed using a baked borough mask, then streets and elevation passes.",
};

constexpr const char* P_MAP_VIEWPORT[] = {
    "Scroll to zoom, drag to pan, hover to highlight tiles, and click to inspect tiles or city landmarks.",
    "Zoom in to see city labels. Landmarks show a marker and name; LaGuardia displays as LGA with the full name on hover.",
    "Toggle Crime heat overlay in the Simulation panel to tint land tiles by live crime pressure.",
};

constexpr const char* P_TILE_INSPECTOR[] = {
    "Shows borough, terrain, chunk coords, and elevation for the hovered or selected map tile.",
};

constexpr const char* P_LANDMARKS_OVERVIEW[] = {
    "City landmarks are fixed strategic nodes placed on specific map tiles across all boroughs.",
    "Each landmark inherits its borough from the underlying tile region data. Add new entries in landmark_table.cpp.",
    "Click a landmark on the map to open the City panel. Labels appear when zoomed in.",
};

constexpr const char* P_CITY_PANEL[] = {
    "The City panel opens when you click a landmark. It shows the full city name, borough, tile coords, ownership, live heat/difficulty, and claim cost.",
    "LaGuardia Airport uses the map label LGA but the panel and hover tooltip show the full name.",
    "Establish operation requires headquarters first, then era-scale capital ($2,500 default) plus network and reputation gates.",
};

constexpr const char* P_OPERATIONS_PANEL[] = {
    "Your first step is headquarters: rented room, apartment (rental application), or family/friend stay (DPA).",
    "DPA means Dwelling Place Arrangement — crashing with family or a friend who vouches for you. No cash rent, but household opinion drifts down unless you pitch in.",
    "Apartment uses an interactive application. Morris Schwartz becomes your landlord contact only after approval.",
    "Family/friend stay requires a family or friend contact in-country. Your kin becomes the landlord on record while staying a separate contact.",
    "Street crime (solo, crew, organization) pays immediate cash when jobs are thin; crew and bigger scores need trusted criminals (friend, rival) and network access.",
    "Rackets and fronts in the catalog unlock later via wealth, network access, and reputation — they will carry upkeep as your empire grows.",
};

constexpr const char* P_STREET_CRIME[] = {
    "Power tier: Solo (default) → Crew (street gang) → Organization (incorporated enterprise).",
    "Recruit friend or rival via Contacts when opinion ≥ 12, trust ≥ 38, respect ≥ 32, loyalty score ≥ 35.",
    "After two recruits, formalize the crew in Operations. Incorporate when network, crime record, and heat allow.",
    "Solo crimes work at any tier; crew crimes need Crew+; organization crimes need Organization tier.",
    "Witnesses can appear after crimes — higher notoriety (heat, reputation, tier) raises witness chance and warrants.",
    "Financial-tier crimes (e.g. import skim) need organization power tier and clear probation/parole gates.",
};

constexpr const char* P_POLICE_HEAT[] = {
    "Personal heat tracks how much local law enforcement is watching you (0–100).",
    "Tiers: off the radar, on the beat's notes, under watch, active case file.",
    "Witnesses after crimes build evidence; warrants issue when evidence and heat cross thresholds.",
    "Arrest rolls increase with warrants and heat. Failed street crimes can trigger pickup.",
};

constexpr const char* P_CRIMINAL_JUSTICE[] = {
    "Legal tiers gate crimes: petty street, street, organization, financial (see each crime's tier).",
    "Arrest → bond hearing modal (pay bond or wait in jail) → court modal → acquittal, probation, or prison.",
    "Prison sentences are short in sim time (~5–15 seconds at 1x) but the world keeps moving.",
    "While incarcerated, rivals can seize a city you claimed. Probation limits you to petty/street tier; parole allows organization tier.",
    "Community NPCs can also be jailed briefly under the same justice tick (contacts panel shows active agents).",
};

constexpr const char* P_BUSINESS_PANEL[] = {
    "Blue nodes are workplaces (always visible). Labels appear when you zoom in, like city landmarks.",
    "You must be in the same borough to apply.",
    "Apply for job opens a paused interview — answer three questions. Pass the interview to be hired; wages accrue over time, not as a lump sum.",
    "You cannot apply again while already employed.",
};

constexpr const char* P_CONTACTS_PANEL[] = {
    "Contacts show opinion (-100 to +100), trust, and respect. Trust and respect follow opinion so rivals stay low-trust.",
    "At start you get generated family/friend contacts plus beat cop, union delegate, and rival. Apartment adds Morris Schwartz; DPA makes your kin the landlord.",
};

constexpr const char* P_TRAVEL_AND_SCHEDULE[] = {
    "You operate only in the borough where you currently are. Travel between boroughs will gate jobs, meetings, and crime (foundation in this build).",
    "Employed characters receive periodic work-day prompts: go on time, go late, or call out. Shifts can lock some actions while you are at work.",
    "Future builds will add route risk (heat, rivals, feds) and commute time affecting lateness and pay.",
};

constexpr const char* P_WINDOWS_MENU[] = {
    "Windows toggles Simulation, Character, Operations, Contacts, Boroughs, City, Business, Tile Inspector, and Map Viewport visibility.",
};

constexpr const char* P_LANDMARK_CONTROL[] = {
    "Cities are always-hot control nodes: higher baseline heat and the highest capture difficulty.",
    "Holding cities weighs more toward borough-wide influence than ordinary tiles (see LANDMARK_BOROUGH_INFLUENCE_WEIGHT).",
    "The City panel shows live crime heat and difficulty adjusted by player influence on the landmark tile.",
};

constexpr const char* P_ECONOMY_OVERVIEW[] = {
    "Cash is tracked in cents. Starting money is modest ($0–$25, weighted low) and only applied when you begin life in New York — not shown during creation.",
    "Legit income requires a job. Passive crime income can come from owned cities; street crimes pay lump sums from Operations.",
    "Income bundles into your wallet every 20 simulation ticks.",
};

constexpr const char* P_CHAR_STARTING_BOROUGH[] = {
    "Starting Borough picks which region rolls your entry city landmark.",
    "The official record lists your city and starting contacts. Cash is applied only when you begin play.",
};

constexpr const char* P_FAMILY_CULTURE[] = {
    "Duty, Express, and Elder auth come from your heritage and nationality. They apply to your profile at game start.",
    "Duty raises kin loyalty and how fast family opinion drops when you ignore them.",
    "Express affects in-group negotiation; family emotional confrontations hit harder.",
    "Elder auth nudges respectable job access and ethnic introductions.",
    "Parents may be in your borough, elsewhere in the city, or abroad. Elsewhere in the city is not the same as abroad.",
};

constexpr const char* P_OFFICIAL_RECORD[] = {
    "After creation you review one document before entering the map.",
    "Immigrants see an Immigration Intake Record. Everyone else sees a Certificate of Birth.",
    "The record lists your legal name, household, cultural notes, and every starting contact (generated NPCs).",
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

constexpr const char* P_CITY_HOT_NODES[] = {
    "Landmark tiles seed max economic weight, higher crime heat, and boosted business vitality within heat radius.",
    "City panel heat and difficulty read live tile pressures; borough lines show rollup context.",
};

constexpr const char* P_DOCKING[] = {
    "Each new game applies the default layout: map center, tools left, boroughs/contacts right, city/business along the bottom.",
    "Panels stay draggable and tabbed; layout saves to capitalvice_layout.ini between sessions (load game keeps your layout).",
    "Panel names and dock zones live in include/ui/game_dock_panels.h — change that table when adding windows.",
    "If panels look stuck, use View > Reset Panel Layout or delete capitalvice_layout.ini.",
};

constexpr const char* P_VIEW_MENU[] = {
    "Reset Panel Layout restores default docking when windows are lost or messy.",
};

constexpr const char* P_SAVE_LOAD[] = {
    "Saves to capitalvice_save.dat (v10). Stores character, world, wallet, housing, contacts, crew tier, police, jail/probation/parole, and crime cooldowns.",
    "Save with Ctrl+S or the File menu while in-game. Load from the main menu or File menu when a save exists.",
};

#if defined(CAPITALVICE_DEV_CONSOLE)
constexpr const char* P_DEV_CONSOLE[] = {
    "Dev builds only. F12 toggles a command console for profile inspection and tweaks.",
    "Commands: help, log clear, profile dump, draft show, wallet/cities/operations/crew/law/justice show (in-game), justice arrest|release, profile set ..., network/legitimacy/loyalty/culture/paths show.",
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

    {"char_creation", "Character / Identity", "Character Creation Screen", "Two-panel setup", P_CHAR_CREATION, 3},
    {"char_starting_borough", "Character / Identity", "Starting Borough", "Roll city and cash", P_CHAR_STARTING_BOROUGH, 2},
    {"char_name", "Character / Identity", "Name Fields", "First, middle, last", P_CHAR_NAME, 2},
    {"family_culture", "Character / Identity", "Family & Culture", "Duty, express, elder auth", P_FAMILY_CULTURE, 4},
    {"official_record", "Character / Identity", "Official Record", "Birth cert or immigration", P_OFFICIAL_RECORD, 3},
    {"char_generation", "Character / Identity", "Generation Choice", "Tradeoffs by generation", P_CHAR_GENERATION, 2},

    {"profile_overview", "Character / Profile", "Player Profile", "Foundational derived data", P_PROFILE_OVERVIEW, 2},
    {"trait_axes", "Character / Profile", "Trait Axes Overview", "One profile layer among many", P_TRAIT_AXES, 3},
    {"profile_builder", "Character / Profile", "Profile Builder", "Draft to numbers", P_PROFILE_BUILDER, 2},
    {"character_panel", "Character / Profile", "Character Panel", "In-game stats UI", P_CHARACTER_PANEL, 3},

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
    {"system_registry", "Simulation", "System Registry", "Tick pipeline", P_SYSTEM_REGISTRY, 3},
    {"borough_vitality_overview", "Simulation / Borough Vitality", "Borough Vitality Overview", "Tiles as signals, boroughs as scoreboard", P_BOROUGH_VITALITY_OVERVIEW, 2},
    {"tile_economic_weight", "Simulation / Borough Vitality", "Tile Economic Weight", "Static contribution to rollup", P_TILE_ECONOMIC_WEIGHT, 2},
    {"borough_health_formula", "Simulation / Borough Vitality", "Borough Health Formula", "What moves economic health", P_BOROUGH_HEALTH_FORMULA, 2},
    {"population_model", "Simulation / Borough Vitality", "Population Model", "Borough stock-flow", P_POPULATION_MODEL, 2},
    {"crime_law_opposition", "Simulation / Borough Vitality", "Crime, Law, and Opposition", "Competing pressures", P_CRIME_LAW_OPPOSITION, 2},
    {"city_hot_nodes", "Simulation / Borough Vitality", "City Hot Nodes", "Landmark vitality seeding", P_CITY_HOT_NODES, 2},
    {"economy_overview", "Simulation / Economy", "Economy Overview", "Cash and income", P_ECONOMY_OVERVIEW, 2},

    {"world_data", "World and Map", "World Data", "512x512 ChunkStore", P_WORLD_DATA, 1},
    {"procgen", "World and Map", "World Generation", "Seed and borough mask", P_PROCGEN, 1},
    {"map_viewport", "World and Map", "Map Viewport", "Pan zoom pick", P_MAP_VIEWPORT, 3},
    {"tile_inspector", "World and Map", "Tile Inspector", "Single tile details", P_TILE_INSPECTOR, 1},
    {"boroughs", "World and Map", "Boroughs Panel", "Live borough vitality bars", P_BOROUGHS, 2},
    {"landmarks_overview", "World and Map / Cities", "City Landmarks", "Strategic map nodes", P_LANDMARKS_OVERVIEW, 3},
    {"city_panel", "World and Map / Cities", "City Panel", "Landmark stats and claims", P_CITY_PANEL, 3},
    {"operations_panel", "World and Map / Operations", "Operations Panel", "HQ, street crime, expansion", P_OPERATIONS_PANEL, 6},
    {"street_crime_panel", "World and Map / Operations", "Street Crime", "Solo, crew, and organization jobs", P_STREET_CRIME, 5},
    {"police_heat", "Simulation / Law", "Police Heat", "Attention and investigation tier", P_POLICE_HEAT, 4},
    {"business_panel", "World and Map / Operations", "Business Panel", "Jobs at blue nodes", P_BUSINESS_PANEL, 3},
    {"travel_schedule", "World and Map / Operations", "Travel & Work Days", "Borough presence and shifts", P_TRAVEL_AND_SCHEDULE, 3},
    {"landmark_control", "World and Map / Cities", "City Control Model", "Hot high-difficulty nodes", P_LANDMARK_CONTROL, 3},

    {"docking", "Interface", "Docking Panels", "Layout persistence", P_DOCKING, 3},
    {"view_menu", "Interface", "View Menu", "Reset layout", P_VIEW_MENU, 1},
    {"windows_menu", "Interface", "Windows Menu", "Toggle panels", P_WINDOWS_MENU, 1},
    {"contacts_panel", "Simulation / Characters", "Contacts Panel", "AI opinions", P_CONTACTS_PANEL, 2},

    {"save_load", "Persistence", "Save and Load", "capitalvice_save.dat v10", P_SAVE_LOAD, 2},
    {"power_tier", "Simulation / Crime", "Power Tier", "Solo, crew, organization", P_STREET_CRIME, 3},

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
        const int32_t chapterStartIndex = topicIndex;
        const char* chapterTitle = HELP_MANUAL_TOPICS[topicIndex].chapterTitle;
        ImGui::PushID(chapterStartIndex);
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
