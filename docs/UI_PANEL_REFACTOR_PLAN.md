# UI Panel Refactor Plan — Decoupled, Reorderable Sections

## Purpose

We want the in-game panels to support rearranging each block of displayed data with ImGui, while keeping the UI cleanly separated from gameplay data and simulation logic.

This plan is intentionally focused on architecture first:
- keep gameplay data in gameplay stores
- keep panel layout state in UI state
- make panel sections composable and reorderable
- minimize regression risk in a large codebase

## Core Principle

**The UI should not own gameplay state.**

The UI may read gameplay stores and render them, but all arrangement, visibility, and presentation preferences must live in UI-local state that can be reset independently of the simulation.

That means:
- gameplay stores remain authoritative for game rules
- UI stores remain authoritative for panel ordering and presentation
- render functions become consumers of both, but only mutate UI state

## Goals

1. Allow sections inside `Operations`, `Business`, and `Contacts` panels to be reordered.
2. Keep section order persistent during a session, but not embedded in gameplay stores.
3. Avoid coupling reorder state to save-game data unless there is a strong user-facing need.
4. Preserve existing gameplay behavior while refactoring incrementally.
5. Keep render code maintainable and testable.

## Non-Goals

- No change to game balance or simulation rules.
- No redesign of the underlying gameplay stores.
- No broad UI framework rewrite.
- No mandatory save-file migration for panel order.

## Architectural Direction

### 1) Separate UI state from gameplay state

Create a dedicated UI state model for panel layout concerns.

Examples of UI-local state:
- which panels are visible
- order of sections within a panel
- selected tab or sub-view within a panel
- transient input controls such as travel targets

Examples of gameplay state:
- money
- heat
- crimes
- job eligibility
- custody status
- property ownership

### 2) Introduce composable section renderers

Each panel should be decomposed into small section renderers that:
- accept gameplay data as read-only input
- accept UI state as mutable input only if needed for presentation controls
- return nothing or return a tiny UI action result

A section renderer should be responsible for one display block, not the whole panel.

### 3) Render sections by order list

Each panel gets a list of section IDs.
At render time:
- iterate the order list
- switch on the section ID
- render the matching block
- show reorder controls near the section header

This is the simplest way to support rearrangement without making the UI dependent on gameplay data structures.

## Proposed State Model

### Panel layout state

Add a UI-only state structure, separate from gameplay stores.

Recommended fields:
- `operationsSectionOrder[]`
- `businessSectionOrder[]`
- `contactsSectionOrder[]`
- optional per-panel selected section IDs
- optional per-panel collapsed section flags later, if needed

### Storage location

Best place:
- `GamePanelVisibility` can be expanded if it is already the UI state bundle
- or a separate `GamePanelLayoutState` can be introduced if we want a cleaner split

Recommended approach:
- create a distinct `GamePanelLayoutState` for ordering data
- keep `GamePanelVisibility` limited to show/hide flags and simple UI preferences

That makes the design clearer:
- visibility state = whether a panel is open
- layout state = how sections are arranged

## Panel Decomposition Plan

### Operations panel

Candidate sections:
1. Housing / headquarters status
2. Power tier / organization status
3. Street crime section
4. Information feed summary
5. Narrative archive summary
6. Legal counsel summary
7. Travel controls
8. Property browser section

### Business panel

Candidate sections:
1. Calendar and shift summary
2. Current employment summary
3. Selected business details
4. Job application / eligibility actions
5. Employment / action status notes

### Contacts panel

Candidate sections:
1. Contact header / work restriction note
2. Social actions
3. Pressure actions
4. Crew actions
5. Contact detail footer / status notes
6. Tab / action container wrapper, if needed

The exact section boundaries should be chosen to preserve readability and avoid mixing unrelated responsibilities.

## ImGui Interaction Model

### Reordering controls

Each section header should have small up/down controls.

Optional later improvement:
- drag-and-drop reordering between section headers

Recommended first version:
- up/down buttons only
- simpler, less error-prone, easier to test

### Rendering order

The order array should define display order, not storage order.

For each section ID:
- render the section content
- place the reorder controls in the header
- keep all existing game logic intact

## Incremental Refactor Strategy

### Step 1 — Add dedicated UI layout state

Create a UI state object for section order.

Requirements:
- default stable ordering
- easy to reset on new game or app restart
- no dependency on save-game data

### Step 2 — Extract section renderers

Break each panel into section-specific helper functions.

Rules:
- helpers should be small and focused
- helpers read game stores but do not own them
- helpers should not change unrelated panel state

### Step 3 — Add order-aware render loop

Use order arrays to choose which section renders first, second, etc.

Requirements:
- skip invalid IDs safely
- preserve behavior if order state is corrupted or uninitialized
- fall back to default ordering when needed

### Step 4 — Add reorder controls

Add UI controls per section.

Behavior:
- move section up/down within its panel
- update UI state only
- do not touch gameplay stores

### Step 5 — Validate and clean up

After implementation:
- run lint checks
- verify existing gameplay still works
- confirm no save/load changes are necessary
- ensure no panel logic started relying on UI order state

## Risk Mitigation

### What could go wrong

1. Section renderers accidentally start mutating gameplay state in presentation code.
2. Order arrays become coupled to save files or simulation state.
3. Panel code becomes more fragmented but still hard to maintain.
4. Reordering logic introduces regressions in controls or ImGui focus behavior.

### How to reduce risk

- keep UI state separate from gameplay stores
- make section renderers simple and side-effect limited
- preserve old behavior while moving code into smaller functions
- use conservative defaults if layout state is invalid
- avoid deep refactors in the same pass as behavior changes

## Recommended Implementation Boundaries

### Put in UI layer
- section order arrays
- panel visibility flags
- selected target coordinates for travel UI
- temporary UI-only labels and toggles

### Keep in gameplay layer
- wallet, heat, travel rules, properties, crimes, jobs, evidence, custody, and all sim data

### Keep out of save data unless justified
- panel section order
- transient UI layout choices

## Acceptance Criteria

The refactor is successful when:
- each target panel section can be moved up or down
- existing gameplay features continue to work
- no gameplay stores were repurposed for UI layout
- UI state can be reset independently
- the code is easier to extend with future sections

## Suggested Next Milestone

Implement the first pass for `Operations` only.

Why `Operations` first:
- it has the widest variety of section types
- it proves the architecture for mixed informational/action blocks
- it is the best test of keeping UI state decoupled from gameplay state

After that:
- apply the same pattern to `Business`
- then to `Contacts`

## Summary

The correct direction is not to couple reorder state to gameplay data. Instead, we should introduce a clean UI layout model, decompose the panels into section renderers, and render those sections in the order stored in UI-local state.

This gives us:
- reorderable sections
- minimal gameplay risk
- clearer code
- a structure that can scale as the UI grows
