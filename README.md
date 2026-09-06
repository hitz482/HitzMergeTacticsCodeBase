# HITZ: Merge Tactics

A server-authoritative auto-battler / merge-tactics framework for Unreal Engine 5.8, written in C++ with a Blueprint-first extension surface. It's a complete, playable reference implementation — board placement, merges, trait synergies, a shop economy, PvE and PvP rounds, and combat that resolves entirely on the server — split into a reusable core module and a sample game layer that shows how to wire it up.

![Hex board with units placed](https://hitz482.github.io/HitzMergeTactics/images/screenshot-board.png)

## Architecture

### Module layout

Four modules, three of them buildable independently of the sample game:

- **`HMT_Core`** (Runtime) — the reusable framework. Board/grid, match state machine, combat resolution, stats, synergy, economy, rulers, modifiers. Contains no game-specific content: no GameMode, no example abilities, no test content.
- **`HMT_CoreEditor`** (Editor) — asset editors, detail customizations, and the save-time content validator. Depends only on `HMT_Core`.
- **`HMT_GASBridge`** (Runtime) — an optional GAS-backed implementation of the core stat interface. Depends on `HMT_Core` and `GameplayAbilities`; nothing else depends on it.
- **`HitzMergeTactics`** (Runtime) — the sample layer: GameMode/PlayerController/GameState, a bot decision strategy, combat playback (turning the replicated event log into what the player sees), and example abilities/modifiers/traits.

The boundary is deliberately at "reusable framework" vs. "one example of using it," not at some other seam. The alternative — one module with everything — would mean anyone dropping this into an existing project drags in the sample GameMode, sample abilities, and test content whether they want them or not. As shipped, `Source/HitzMergeTactics/` and its content are the part meant to be replaced; `HMT_Core`, `HMT_CoreEditor`, and (if used) `HMT_GASBridge` are the part meant to stay.

### Server-authoritative combat

Combat never runs on a client. The match state machine has a dedicated `CombatLock` phase whose entire job is to freeze both sides' board state — units, star levels, active stat modifiers, tile modifiers, active traits — into an immutable `FHMT_BoardSnapshot` per player before anything resolves. `UHMT_CombatResolver::ResolveCombat` then runs once, server-side only, against that locked pair of snapshots and produces an ordered `FHMT_CombatEventLog`: attacks, damage, casts, deaths, movement, all timestamped.

That log is what actually replicates — via `FFastArraySerializer` delta serialization on `UHMT_CombatPlaybackComponent` — not the simulation. Clients (the combat's own participants and any spectators) receive a fixed sequence of events they didn't compute and can't influence, and `HMT_CombatPlaybackDriver` (sample layer) turns that sequence into local-only cosmetic skeletal-mesh proxies: no replicated state depends on what the client does with the log. The alternative — replicating raw board state and letting each client resolve its own combat, or running prediction client-side — is exactly the class of exploit this closes off: a client can't influence damage rolls, crit chance, or targeting, because it never computes them.

### Extension interfaces

Four `UINTERFACE(BlueprintType)` interfaces are the entire extension surface for gameplay-specific behavior:

- **`IHMT_StatProvider`** — get/set/modify a unit's stats by `FGameplayTag`.
- **`IHMT_TargetingStrategy`** — pick a target from a candidate list during combat.
- **`IHMT_TraitEffect`** — react to synergy activation/deactivation and combat events.
- **`IHMT_AbilityExecutor`** — decide whether and how a unit casts its ability.

All four methods are `BlueprintNativeEvent`, and `HMT_Core`'s combat/stat/targeting code calls through the interface exclusively — it never downcasts to a concrete class. The alternative would be an abstract base actor class that units must inherit from, which breaks the moment a buyer already has their own actor/pawn hierarchy they want to reuse. An interface can be implemented by any actor, in C++ or pure Blueprint, without touching its existing parent class — that's the actual reason it's an interface and not a base class.

### The GAS bridge

`HMT_GASBridge` is what that interface design buys you: `UHMT_ASCStatComponent` implements `IHMT_StatProvider` by delegating to a `UAbilitySystemComponent`, mapping `FGameplayTag` stat identifiers onto GAS attributes through a `UHMT_AttributeTagMapAsset`. `HMT_Core` never takes a dependency on `GameplayAbilities` — buyers who don't use GAS never pull that module in; buyers who do get a drop-in stat provider instead of writing the bridge themselves. If `IHMT_StatProvider` had been a base class instead of an interface, this module either couldn't exist as a separate, optional thing, or would require multiple inheritance.

### Editor tooling

- **Board layout painter** — `FHMT_BoardLayoutAssetCustomization` renders a clickable grid directly in the details panel for `UHMT_BoardLayoutAsset`, toggling blocked cells, plus an in-level preview button.
- **Unit Definition editor** — `FHMT_UnitDefinitionEditorToolkit` adds a live 3D preview viewport (with anim-slot playback) alongside the normal details panel when editing a `UHMT_UnitDefinition`, so a designer can see the mesh/animation/material result without leaving the asset editor.
- **Save-time content validator** — `UHMT_ContentValidator` hooks into UE's `UEditorValidatorBase` framework and validates seven data-asset types (unit, trait, board layout, PvE round, ruler, game modifier, tile modifier) on save, catching misconfiguration before it reaches a running match.

### Grid abstraction

`UHMT_BoardLayoutAsset` carries a single `EHMT_GridTopology` field (`Hex` or `Square`). Every coordinate — `FHMT_GridCoord{Q, R}` — is topology-agnostic axial coordinates; `UHMT_BoardComponent` holds a `TScriptInterface<IHMT_GridLayout>` and resolves it to either `UHMT_HexGridLayout` or `UHMT_SquareGridLayout` based on that one field. Everything above the board component — pathing, targeting, combat — only ever calls `GetNeighbors`/`GetDistance`/`GridToWorld`/`WorldToGrid` through the interface. Switching a board's topology is a dropdown on a data asset, not a code change or a duplicated implementation.

## Start here

In this order:

1. **[`Source/HMT_Core/Public/Combat/HMT_CombatTypes.h`](Source/HMT_Core/Public/Combat/HMT_CombatTypes.h)** — the shared vocabulary: `FHMT_BoardSnapshot`, `FHMT_CombatEvent`, `FHMT_CombatEventLog`. Everything else in combat is built around these three types.
2. **[`Source/HMT_Core/Public/Combat/HMT_CombatResolver.h`](Source/HMT_Core/Public/Combat/HMT_CombatResolver.h)** — where a locked pair of snapshots turns into an event log, server-side only.
3. **[`Source/HMT_Core/Public/Units/IHMT_AbilityExecutor.h`](Source/HMT_Core/Public/Units/IHMT_AbilityExecutor.h)** (and its three siblings in `Public/Units`, `Public/Stats`, `Public/Synergy`) — the entire extension surface, in ~30 lines each.
4. **[`Source/HitzMergeTactics/HMT_CombatPlaybackDriver.h`](Source/HitzMergeTactics/HMT_CombatPlaybackDriver.h)** — the sample layer's payoff: how a replicated event log becomes what the player actually sees. Also the single file that best represents the comment style used throughout — read it to gauge the rest of the codebase.

## Build instructions

Requires **Unreal Engine 5.8** and **Visual Studio 2022** (Desktop development with C++ workload).

1. Clone the repo.
2. Right-click `HitzMergeTactics.uproject` → **Generate Visual Studio project files** (or just double-click it — no `Binaries/`/`Intermediate/` is committed, so UE will offer to build missing modules on first open; accept).
3. First build compiles all four modules against the engine; expect it to take a while.
4. The editor opens directly into `Demo.umap` — it's both the editor startup map and the packaged game's default map.
5. Press Play. The shipped `DA_MatchRules` asset defaults `MinPlayers` to 2; either launch with 2 players (Editor Preferences → Play → Number of Players), or enable `bFillWithBotsWhenMinPlayersUnmet` on that asset first to get a full round from a single PIE session.

## Documentation

Full setup guide, data-asset reference, GameMode wiring, and troubleshooting: **https://hitz482.github.io/HitzMergeTactics/**

## License

Source-available — see [LICENSE](LICENSE) for the full terms.
