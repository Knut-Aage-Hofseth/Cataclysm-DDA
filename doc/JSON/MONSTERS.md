<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
*Contents*

- [MONSTERS](#monsters)
  - [Monster properties](#monster-properties)
  - ["name"](#name)
  - ["description"](#description)
  - ["categories"](#categories)
  - ["species"](#species)
  - ["volume"](#volume)
  - ["weight"](#weight)
  - ["scents_tracked"](#scents_tracked)
  - ["scents_ignored"](#scents_ignored)
  - ["symbol", "color"](#symbol-color)
  - ["material"](#material)
  - ["phase"](#phase)
  - ["default_faction"](#default_faction)
  - ["bodytype"](#bodytype)
  - ["attack_cost"](#attack_cost)
  - ["diff"](#diff)
  - ["aggression"](#aggression)
  - ["morale"](#morale)
  - ["aggro_character"](#aggro_character)
  - ["speed"](#speed)
  - ["mountable_weight_ratio"](#mountable_weight_ratio)
  - ["melee_skill"](#melee_skill)
  - ["dodge"](#dodge)
  - ["melee_damage"](#melee_damage)
  - ["melee_dice", "melee_dice_sides"](#melee_dice-melee_dice_sides)
  - ["hitsize_min", "hitsize_max"](#hitsize_min-hitsize_max)
  - ["grab_strength"](#grab_strength)
  - ["armor"](#armor)
  - ["weakpoints"](#weakpoints)
  - ["weakpoint_sets"](#weakpoint_sets)
  - ["families"](#families)
  - ["vision_day", "vision_night"](#vision_day-vision_night)
  - ["luminance"](#luminance)
  - ["hp"](#hp)
  - ["bleed_rate"](#bleed_rate)
  - ["death_drops"](#death_drops)
  - ["death_function"](#death_function)
  - ["emit_fields"](#emit_fields)
  - ["regenerates"](#regenerates)
  - ["regenerates_in_dark"](#regenerates_in_dark)
  - ["regen_morale"](#regen_morale)
  - ["flags"](#flags)
  - ["fear_triggers", "anger_triggers", "placate_triggers"](#fear_triggers-anger_triggers-placate_triggers)
  - ["chat_topics"](#chat_topics)
  - ["revert_to_itype"](#revert_to_itype)
  - ["starting_ammo"](#starting_ammo)
  - ["upgrades"](#upgrades)
  - ["reproduction"](#reproduction)
  - ["zombify_into"](#zombify_into)
  - ["revive_forms"](#revive_forms)
  - ["baby_flags"](#baby_flags)
  - ["shearing"](#shearing)
  - ["speed_description"](#speed_description)
  - ["petfood"](#petfood)
  - ["special_when_hit"](#special_when_hit)
  - ["attack_effs"](#attack_effs)
  - ["path_settings"](#path_settings)
  - ["move_skills"](#move_skills)
  - ["special_attacks"](#special_attacks)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# MONSTERS

Monsters include not just zombies, but fish, dogs, moose, Mi-gos, manhacks, and even stationary installations like turrets. They are defined in JSON objects with "type" set to "MONSTER":

```jsonc
{
  "type": "MONSTER",
  "id": "mon_foo",
  ...
}
```

The `"id"` member is the unique identifier for this monster type. It can be any string, but by convention has the prefix `mon_`. This id is referenced in monster groups or in mapgen to spawn specific monsters.

For quantity strings (i.e. volume, weight) use the largest unit you can keep full precision with.


## Monster properties

These properties are required for all monsters:

Property          | Description
---               | ---
`name`            | (string or object) Monster name, and optional plural name and translation context. Three words maximum.
`description`     | (string) In-game description of the monster, in one or two sentences
`hp`              | (integer) Hit points. Also see [monster HP scaling in GAME_BALANCE.md](GAME_BALANCE.md#monster-hp-scaling)
`volume`          | (string) Volume of the creature's body, as an integer with metric units, ex. `"35 L"` or `"1500 ml"`. Used to calculate monster size, size influences melee hit chances on different-sized targets.
`weight`          | (string) Monster weight, as an integer with metric units, ex. `"12 kg"` or `"7500 g"`
`symbol`          | (string) UTF-8 single-character string representing the monster in-game
`color`           | (string) Symbol color for the monster
`default_faction` | (string) What faction the monster is allied with
`bodytype`        | (string) Monster's body type, ex. bear, bird, dog, human, insect, lizard etc.
`speed`           | (integer) Monster speed, relative to human speed `100`, with greater numbers being faster. Also see [monster speed scaling in GAME_BALANCE.md](/doc/design-balance-lore/GAME_BALANCE.md#monster-speed-scaling)

Monsters may also have any of these optional properties:

Property                 | Description
---                      | ---
`copy-from`              | (string) Inherit monster attributes from another. See [JSON_INHERITANCE.md](JSON_INHERITANCE.md)
`categories`             | (array of strings) Monster categories (NULL, CLASSIC, or WILDLIFE). Important for mods who black or whitelist monsters.
`ascii_picture`          | (string) Id of the `ascii_art` used for this monster
`species`                | (array of strings) Species IDs, ex. HUMAN, ROBOT, ZOMBIE, BIRD, MUTANT, etc. Defined in [`species.json`](../data/json/species.json)
`scents_tracked`         | (array of strings) Monster tracks these scents. Defined [`scent_types.json`](../data/json/scent_types.json)
`scents_ignored`         | (array of strings) Monster ignores these scents
`material`               | (array of strings) Materials the monster is made of. Defined in [`materials.json`](../data/json/materials.json)
`phase`                  | (string) Monster's body matter state, ex. SOLID, LIQUID, GAS, PLASMA, NULL
`attack_cost`            | (integer) Number of moves per regular attack. If not defined defaults to `100`.
`diff`                   | (integer) Displayed monster difficulty, e.g affects how the monster name is colored, e.g red for very nasty critters. Also see [monster difficulty scaling in GAME_BALANCE.md](/doc/design-balance-lore/GAME_BALANCE.md#monster-difficulty-scaling)
`aggression`             | (integer) Starting aggression, the monster will become hostile when it reaches 10
`morale`                 | (integer) Starting morale, monster will flee when (current aggression + current morale) < 0
`aggro_character`        | (bool) If true the monster will always attack characters when angry.
`mountable_weight_ratio` | (float) For mounts, max ratio of mount to rider weight, ex. `0.2` for `<=20%`
`melee_skill`            | (integer) Monster skill in melee combat, from `0-10`, with `4` being an average mob. Also see [monster melee skill scaling in GAME_BALANCE.md](/doc/design-balance-lore/GAME_BALANCE.md#monster-melee-skill-scaling)
`dodge`                  | (integer) Monster's skill at dodging attacks
`melee_damage`           | (array of objects) List of damage instances added to die roll on monster melee attack. Also see [monster maximum damage scaling in GAME_BALANCE.md](/doc/design-balance-lore/GAME_BALANCE.md#monster-maximum-damage-scaling)
`melee_dice`             | (integer) Number of dice rolled on monster melee attack to determine bash damage
`melee_dice_sides`       | (integer) Number of sides on each die rolled by `melee_dice`
`grab_strength`          | (integer) Intensity of grab effect, from `1` to `n`. A default zombie has a grab strength of `20`
`melee_training_cap`     | (integer) The maximum melee skill levels learnable by fighting this monster. If not defined defaults to `melee_skill + 2`.
`armor`                  | (object) Monster's protection from different types of damage
`weakpoints`             | (array of objects) Weakpoints in the monster's protection
`weakpoint_sets`         | (array of strings) Weakpoint sets to apply to the monster. Defined in [`monster_weakpoints`](../data/json/monster_weakpoints)
`status_chance_multiplier`| (float) Multiplier to chance to apply zapped when electric damage is dealt (no other effects are implemented at this time)
`families`               | (array of objects or strings) Weakpoint families that the monster belongs to
`vision_day`             | (integer) Vision range in full daylight, with `50` being the typical maximum
`vision_night`           | (integer) Vision range in total darkness, ex. coyote `5`, bear `10`, sewer rat `30`, flaming eye `40`
`tracking_distance`      | (integer) Amount of tiles the monster will keep between itself and its current tracked enemy or followed leader. Defaults to `3`.
`trap_avoids`            | (array of strings) trap_id of traps that are not triggered by this monster. Default behaviour is to trigger all traps.
`luminance`              | (float) Amount of light passively emitted by the monster, must be >0 to have any effect
`death_drops`            | (string or item group) Item group to spawn when the monster dies
`death_function`         | (array of strings) How the monster behaves on death. See JSON_FLAGS
`emit_fields`            | (array of objects) What field the monster emits, and how frequently
`regenerates`            | (integer) Number of hit points the monster regenerates per turn
`regenerates_in_dark`    | (boolean) True if monster regenerates quickly in the dark
`regeneration_modifiers` | (effect id, integer) When monster has this effect, modify regenerates by integer value (i.e. -5 reduces a regen value of 40hp/turn to 35hp/turn). Cannot reduce regeneration below 0.
`regen_morale`           | (bool) True if monster will stop fleeing at max HP to regenerate anger and morale
`special_attacks`        | (array of objects) Special attacks the monster has
`flags`                  | (array of strings) Any number of attributes like SEES, HEARS, SMELLS, STUMBLES, REVIVES
`fear_triggers`          | (array of strings) Triggers that lower monster morale (see JSON_FLAGS.md)
`anger_triggers`         | (array of strings) Triggers that raise monster aggression (same flags as fear)
`placate_triggers`       | (array of strings) Triggers that lower monster aggression (same flags as fear)
`chat_topics`            | (array of strings) Conversation topics if dialog is opened with the monster
`revert_to_itype`        | (string) Item monster can be converted to when friendly (ex. to deconstruct turrets)
`broken_itype`           | (string) Item that will spawn when the monster dies with `corpse_type` set to `BROKEN`. Can be left empty to create a `broken_` item based on the monster's `id`. 
`mech_weapon`            | (string) If this monster is a rideable mech with built-in weapons, this is the weapons id
`mech_str_bonus`         | (integer) If this monster is a rideable mech with enhanced strength, this is the strength it gives to the player when ridden
`mech_battery`           | (string) If this monster is a rideable mech, this is battery's id. Does not support objects or arrays (i.e. ONE battery id only)
`starting_ammo`          | (object) Ammo that newly spawned monsters start with
`mount_items`            | (array of objects) Mount-specific items this monster spawns with. Accepts entries for `tied`, `tack`, `armor`, and `storage`.
`upgrades`               | (boolean or object) False if monster does not upgrade, or an object do define an upgrade
`reproduction`           | (object) The monster's reproductive cycle and timing
`baby_flags`             | (array of strings) Seasons during which this monster is capable of reproduction
`special_when_hit`       | (array) Special defense triggered when the monster is attacked
`attack_effs`            | (array of objects) Effects applied to the attacked creature when the monster successfully attacks
`path_settings`          | (object) How monster may find a path, open doors, avoid traps, or bash obstacles
`biosignature`           | (object) Droppings or feces left by the animal or monster
`harvest`                | (string) ID of a "harvest" type describing what can be harvested from the corpse
`dissect`                | (string) (Optional) ID of a "harvest" type describing what is returned when a corpse of this monster is dissected
`decay`                  | (string) (Optional) ID of a "harvest" type describing what is left when a corpse of this monster rots away
`zombify_into`           | (string) mtype_id this monster zombifies into after it's death
`fungalize_into`         | (string) mtype_id this monster turns into when fungalized by spores
`shearing`               | (array of objects) Items produced when the monster is sheared
`speed_description`      | (string) ID of a `speed_description` type describing the monster speed string
`petfood`                | (object) Data regarding feeding this monster to turn it into a pet
`absorb_ml_per_hp`       | (int) For monsters with the `ABSORB_ITEMS` special attack. Determines the amount in milliliters that must be absorbed to gain 1 HP. Default 250.
`absorb_move_cost_per_ml`| (float) For monsters with the `ABSORB_ITEMS` special attack. Determines the move cost for absorbing items based on the volume in milliliters of the absorbed items. Default 0.025f.
`absorb_move_cost_min`   | (int) For monsters with the `ABSORB_ITEMS` special attack. Sets a minimum movement cost for absorbing items regardless of the volume of the consumed item. Default 1.
`absorb_move_cost_max`   | (int) For monsters with the `ABSORB_ITEMS` special attack. Sets a maximum movement cost for absorbing items regardless of the volume of the consumed item. -1 for no limit. Default -1.
`absorb_material`        | (array of string) For monsters with the `ABSORB_ITEMS` special attack. Specifies the types of materials that the monster will seek to absorb. Items with multiple materials will be matched as long as it is made of at least one of the materials in this list. If not specified the monster will absorb all materials.
`no_absorb_material`        | (array of string) For monsters with the `ABSORB_ITEMS` special attack. Specifies the types of materials that the monster is unable to absorb. This takes precedence over absorb_material; even if the monster is whitelisted for this material, it cannot do so if any of its materials are found here. If not specified, there are no limits placed on what was whitelisted.
`split_move_cost`        | (int) For monsters with the `SPLIT` special attack. Determines the move cost when splitting into a copy of itself.
`revive_forms`           | (array of objects) allows to define conditional monster 
`move_skills`           | (object with optional members) allows to define how well the monster moves on difficult terrain. Skill can range from 0-10.

Properties in the above tables are explained in more detail in the sections below.


## "name"
(string or object, required)

```jsonc
"name": "cow"
```

```jsonc
"name": { "ctxt": "fish", "str": "pike", "str_pl": "pikes" }
```

or, if the singular and plural forms are the same:

```jsonc
"name": { "ctxt": "fish", "str_sp": "bass" }
```

Name displayed in-game, and optionally the plural name and a translation context (ctxt).

If the plural name is not specified, it defaults to singular name + "s". "str_pl" may also be needed if the unit test cannot determine if the correct plural form can be formed by simply appending "s".

Ctxt is used to help translators in case of homonyms (two different things with the same name). For example, pike the fish and pike the weapon.

## "description"
(string, required)

In-game description for the monster.

## "categories"
(array of strings, optional)

Monster categories. Can be NULL, CLASSIC (only mobs found in classic zombie movies) or WILDLIFE (natural animals). If they are not CLASSIC or WILDLIFE, they will not spawn in classic mode.

## "species"
(array of strings, optional)

A list of species ids. Properties (currently only triggers) from species are added to the properties of each monster that belong to the species.

In mainline game it can be HUMAN, ROBOT, ZOMBIE, MAMMAL, BIRD, FISH, REPTILE, WORM, MOLLUSK, AMPHIBIAN, INSECT, SPIDER, FUNGUS, PLANT, NETHER, MUTANT, BLOB, HORROR, ABERRATION, HALLUCINATION and UNKNOWN.

## "volume"
(string, required)

```jsonc
"volume": "40 L"
```
The numeric part of the string must be an integer. Accepts L, and ml as units. Note that l and mL are not currently accepted.

## "weight"
(string, required)

```jsonc
"weight": "3 kg"
```
The numeric part of the string must be an integer. Use the largest unit you can keep full precision with. For example: 3 kg, not 3000 g. Accepts g and kg as units.

## "scents_tracked"
(array of strings, optional)

List of scenttype_id tracked by this monster. scent_types are defined in scent_types.json

## "scents_ignored"
(array of strings, optional)

List of scenttype_id ignored by this monster. scent_types are defined in scent_types.json

## "symbol", "color"
(string, required)

Symbol and color representing monster in-game. The symbol must be a UTF-8 string, that is exactly one console cell width (may be several Unicode characters). See [COLOR.md](/doc/user-guides/COLOR.md) for details.

## "material"
(array of strings, optional)

The materials the monster is primarily composed of. Must contain valid material ids. An empty array (which is the default) is also allowed, the monster is made of no specific material.

## "phase"
(string, optional)

It describes monster's body state of matter. However, it doesn't seem to have any gameplay purpose, right now.
It can be SOLID, LIQUID, GAS, PLASMA or NULL.

## "default_faction"
(string, required)

The id of the faction the monster belongs to, this affects what other monsters it will fight. See Monster factions.

## "bodytype"
(string, required)

The id of the monster's bodytype, which is a general description of the layout of the monster's body.

Value should be one of:

Value           | Description
---             | ---
`angel`         | a winged human
`bear`          | a four legged animal that can stand on its hind legs
`bird`          | a two legged animal with two wings
`blob`          | a blob of material
`crab`          | a multilegged animal with two large arms
`dog`           | a four legged animal with a short neck elevating the head above the line of the body
`elephant`      | a very large quadruped animal with a large head and torso with equal sized limbs
`fish`          | an aquatic animal with a streamlined body and fins
`flying insect` | a six legged animal with a head and two body segments and wings
`frog`          | a four legged animal with a neck and with very large rear legs and small forelegs
`gator`         | a four legged animal with a very long body and short legs
`horse`         | a four legged animal with a long neck elevating the head above the line of the body
`human`         | a bipedal animal with two arms
`insect`        | a six legged animal with a head and two body segments
`kangaroo`      | a pentapedal animal that utilizes a large tail for stability with very large rear legs and smaller forearms
`lizard`        | a smaller form of 'gator'
`migo`          | whatever form migos have
`pig`           | a four legged animal with the head in the same line as the body
`spider`        | an eight legged animal with a small head on a large abdomen
`snake`         | an animal with a long body and no limbs

## "attack_cost"
(integer, optional)

Number of moves per regular attack. Higher values means the monster attacks less often (e.g. a monster with a speed of 100 and an attack_cost of 1000 attacks about once every 10 seconds).

## "diff"
(integer, optional)

Monster baseline difficulty.  Impacts the shade used to label the monster, and if it is above 30 a kill will be recorded in the memorial log.  Monster difficulty is calculated based on expected melee damage, dodge, armor, hit points, speed, morale, aggression, and vision ranges, and the defined ``diff`` value is added on top of that.  The calculation does not handle special attacks, and baseline difficulty can be used to offset that gap.  Suggested values:

Value | Description
---   | ---
`2`   | a limited defensive ability such as a skitterbot's taser, or a weak special like a shrieker zombie's special ability to alert nearby monsters, or a minor bonus to attack like poison or venom.
`5`   | a limited ranged attack weaker than spitter zombie's spit, or a powerful defensive ability like a shocker zombie's zapback or an acid zombie's acid spray.
`10`  | a powerful ranged attack, like a spitters zombie's spit or an turret's 9mm SMG.
`15`  | a powerful ranged attack with additional hazards, like a corrosive zombie's spit
`20`  | a very powerful ranged attack, like a laser turret or military turret's 5.56mm rifle, or a powerful special ability, like a zombie necromancer's ability to raise other zombies.
`30`  | a ranged attack that is deadly even for armored characters, like an anti-material turret's .50 BMG rifle.

Most monsters should have ``diff`` of 0 - even dangerous monsters like a zombie hulk or razorclaw alpha.  This field should only be used for exceptional, ranged, special attacks.

## "aggression"
(integer, optional)

Baseline aggression, modified by anger/placation triggers dynamically and tending towards this value otherwise.  Anger above 10 triggers hostility (gated by monster factions and character aggro), anger below 0 means ignoring or fleeing when not at max HP.

## "morale"
(integer, optional)

Baseline morale, modified by fear triggers dynamically and tending towards this value otherwise.  At negative morale the monster will flee unless its anger + morale is above 0 and it has more than a third of its max HP.

## "aggro_character"
(bool, optional, default true)

If the monster will differentiate between monsters and characters when deciding on targets - if false the monster will ignore characters regardless of current anger/morale until a character trips and anger trigger. Resets randomly when the monster is at its base anger level.

## "speed"
(integer, required)

Monster speed. 100 is the normal speed for a human being - higher values are faster and lower values are slower.

## "mountable_weight_ratio"
(float, optional)

Used as the acceptable rider vs. mount weight percentage ratio. Defaults to "0.2", which means the mount is capable of carrying riders weighing `<= 20%` of the mount's weight.

## "melee_skill"
(integer, optional)

Monster melee skill, ranges from 0 - 10, with 4 being an average mob. See [GAME_BALANCE.md](/doc/design-balance-lore/GAME_BALANCE.md) for more examples

## "dodge"
(integer, optional)

Monster dodge skill. See [GAME_BALANCE.md](/doc/design-balance-lore/GAME_BALANCE.md) for an explanation of dodge mechanics.

## "melee_damage"
(array of objects, optional)

List of damage instances added to die roll on monster melee attack.

Field               | Description
---                 | ---
`damage_type`       | one of "pure", "biological", "bash", "cut", "acid", "stab", "heat", "cold", "electric"
`amount`            | amount of damage
`armor_penetration` | how much of the armor the damage instance ignores
`armor_multiplier`  | multiplier on `armor_penetration`
`damage_multiplier` | multiplier on `amount`

Example:

```jsonc
"melee_damage": [
  {
    "damage_type": "electric",
    "amount": 4.0,
    "armor_penetration": 1,
    "armor_multiplier": 1.2,
    "damage_multiplier": 1.4
  }
]
```

## "melee_dice", "melee_dice_sides"
(integer, optional)

Number of dices and their sides that are rolled on monster melee attack. This defines the amount of bash damage.

## "hitsize_min", "hitsize_max"
(integer, optional )

Lower and upper bound of limb sizes the monster's melee attack can target - see `body_parts.json` for the hit sizes.

## "grab_strength"
(integer, optional)

Base intensity of the grab effect applied by this monster as used by a `grab`-type special attack unless overwritten by the `grab_data` of the attack itself, default 1, range 1-100. See [Grab attacks](MONSTER_SPECIAL_ATTACKS.md#grab_attacks) for more information.

## "armor"
(object, optional)

Monster protection from various types of damage. Any `damage_type` id can be used here, see `damage_types.json`.

```jsonc
"armor": { "bash": 7, "cut": 7, "acid": 4, "bullet": 6, "electric": 2 }
```

## "weakpoints"
(array of objects, optional)

Weakpoints in the monster's protection.

Field              | Description
---                | ---
`id`               | id of the weakpoint. Defaults to `name`, if not specified.
`name`             | name of the weakpoint. Used in hit messages.
`coverage`         | base percentage chance of hitting the weakpoint. (e.g. A coverage of 5 means a 5% base chance of hitting the weakpoint)
`is_good`          | marks mutation, that is beneficial for you to hit (like headshot); false means it is a bad weakpoint for you to hit (like thick piece of armor); default true;
`is_head`          | determines whether this weakpoint is part of the monster's head and can trigger a headshot; Only functions when `is_good` is also true, default false;
`coverage_mult`    | object mapping weapon types to constant coverage multipliers.
`difficulty`       | object mapping weapon types to difficulty values. Difficulty acts as soft "gate" on the attacker's skill. If the the attacker has skill equal to the difficulty, coverage is reduced to 50%. For bad weakpoint, attacker's skill will reduce the chance of hitting it.
`armor_mult`       | object mapping damage types to multipliers on the monster's base protection, when hitting the weakpoint.
`armor_penalty`    | object mapping damage types to flat penalties on the monster's protection, applied after the multiplier.
`damage_mult`      | object mapping damage types to multipliers on the post-armor damage, when hitting the weakpoint.
`crit_mult`        | object mapping damage types to multipliers on the post-armor damage, when critically hitting the weakpoint. Defaults to `damage_mult`, if not specified.
`condition`        | condition, that need to be met for weakpoint to be used. `u_` is attacker (if presented, use `has_alpha` condition for safety), `npc_` is victim. See EFFECT_ON_CONDITION.md for more information
`effects`          | list of effects objects that may be applied to the monster by hitting the weakpoint.

The `effects` field is a list of objects with the following subfields:

Field              | Description
---                | ---
`effect`           | The effect type.
`effect_on_conditions` | Array of EoCs that would be run. `u_` is attacker (if presented, use `has_alpha` condition for safety), `npc_` is victim. See EFFECT_ON_CONDITION.md for more information
`chance`           | The probability of causing the effect.
`duration`         | The duration of the effect. Either a (min, max) pair or a single value.
`permanent`        | Whether the effect is permanent.
`intensity`        | The intensity of the effect. Either a (min, max) pair or a single value.
`damage_required`  | The range of damage, as a percentage of max health, required to trigger the effect.
`message`          | The message to print, if the player triggers the effect. Should take a single template parameter, referencing the monster's name.

The `coverage_mult` and `difficulty` objects support the following subfields:

Field              | Description
---                | ---
`all`              | The default value, if nothing more specific is provided.
`bash`             | The value used for melee bashing weapons.
`cut`              | The value used for melee cutting weapons.
`stab`             | The value used for melee stabbing weapons.
`ranged`           | The value used for ranged weapons, including projectiles and throwing weapons.
`melee`            | The default value for melee weapons (`bash`, `cut`, and `stab`). Takes precedence over `point` and `broad`.
`point`            | The default value for pointed weapons (`stab` and `ranged`).
`broad`            | The default value for broad weapons (`bash` and `cut`).

The `armor_mult`, `armor_penalty`, `damage_mult`, and `crit_mult` objects support *all damage types*, as well as the following fields:

Field              | Description
---                | ---
`all`              | The default value for all fields, if nothing more specific is provided.
`physical`         | The default value for physical damage types (`bash`, `cut`, `stab`, and `bullet`)
`non_physical`     | The default value for non-physical damage types (`biological`, `acid`, `heat`, `cold`, and `electric`)

Default weakpoints are weakpoint objects with an `id` equal to the empty string.
When an attacker misses the other weakpoints, they will hit the defender's default weakpoint.
A monster should have at most 1 default weakpoint.

## "weakpoint_sets"
(array of strings, optional)

Each string refers to the id of a separate `"weakpoint_set"` type JSON object (See [Weakpoint Sets](JSON_INFO.md#weakpoint-sets) for details).

Each subsequent weakpoint set overwrites weakpoints with the same id from the previous set. This allows hierarchical sets that can be applied from general -> specific, so that general weakpoint sets can be reused for many different monsters, and more specific sets can override some general weakpoints for specific monsters. For example:
```jsonc
"weakpoint_sets": [ "humanoid", "zombie_headshot", "riot_gear" ]
```
In the example above, the `"humanoid"` weakpoint set is applied as a base, then the `"zombie_headshot"` set overwrites any previously defined weakpoints with the same id (ex: "wp_head_stun"). Then the `"riot_gear"` set overwrites any matching weakpoints from the previous sets with armour-specific weakpoints. Finally, if the monster type has an inline `"weakpoints"` definition, those weakpoints overwrite any matching weakpoints from all sets.

Weakpoints only match if they share the same id, so it's important to define the weakpoint's id field if you plan to overwrite previous weakpoints.

## "families"
(array of objects or strings, optional)

Weakpoint families that the monster belongs to.

Field              | Description
---                | ---
`id`               | The ID of the family. Defaults to `proficiency`, if not provided.
`proficiency`      | The proficiency ID corresponding to the family.
`bonus`            | The bonus to weak point skill, if the attacker has the proficiency.
`penalty`          | The penalty to weak point skill, if the attacker lacks the proficiency.

`"families": [ "prof_intro_biology" ]` is equivalent to `"families": [ { "proficiency": "prof_intro_biology" } ]`

## "vision_day", "vision_night"
(integer, optional)

Vision range in full daylight and in total darkness.  Defaults to 40 tiles for day and 1 for night.

## "luminance"
(integer, optional)

Amount of light passively output by monster. Ranges from 0 to 10.

## "hp"
(integer, required)

Monster hit points.

## "bleed_rate"
(integer, optional)

Percent multiplier on all bleed effects' duration applied to the monster. Values below the default of 100 mean a resistance to bleed, values above 100 make the monster bleed longer and more intensive. 0 translates to bleed immunity.

## "death_drops"
(string or item group, optional)

An item group that is used to spawn items when the monster dies. This can be an inlined item group, see [ITEM_SPAWN.md](ITEM_SPAWN.md). The default subtype is "distribution".

## "death_function"
(object, optional)

How the monster behaves on death.
```jsonc
{
    "corpse_type": "NORMAL", // can be: BROKEN, NO_CORPSE, NORMAL (default)
    "message": "The %s dies!", // substitute %s for the monster's name.
    "effect": { "id": "death_boomer", "hit_self": true }, // the spell that gets called when the monster dies.  follows the syntax of fake_spell.
    "eoc": "debug_eoc_message",  // eoc that would be run when monster dies. Alpha talker is monster, beta talker is player (always).
}
```

## "emit_fields"
(array of objects of emit_id and time_duration, optional)
"emit_fields": [ { "emit_id": "emit_gum_web", "delay": "30 m" } ],

What field the monster emits and how often it does so. Time duration can use strings: "1 h", "60 m", "3600 s" etc...

## "regenerates"
(integer, optional)

Number of hitpoints regenerated per turn.

## "regenerates_in_dark"
(boolean, optional)

Monster regenerates very quickly in poorly lit tiles.

## "regen_morale"
(boolean, optional)

Will stop fleeing if at max hp, and regen anger and morale.

## "flags"
(array of strings, optional)

Monster flags. See [JSON_FLAGS.md](JSON_FLAGS.md) for a full list. These are IDs that point to a `"monster_flag"` object, which usually can be found in `data/json/monsters/monster_flags.json`.

## "fear_triggers", "anger_triggers", "placate_triggers"
(array of strings, optional)

What makes the monster afraid / angry / what calms it. See [JSON_FLAGS.md](JSON_FLAGS.md) for a full list

## "chat_topics"
(string, optional)

Lists possible chat topics that will be used as dialogue display when talking to a monster, done by `e`xamining it and `c`hatting with it. The creature in question must be friendly to the player in order to talk to it, or have the `CONVERSATION` flag. Alternatively an EOC spell/special attack between a player and monster can start a conversation using the `open_dialogue` effect, see [NPCs.md](NPCs.md) for more details.  Monsters can be assigned variables, but cannot trade with the exchange interface. Listing multiple chat topics will cause the game to crash. This must be defined as an array.

```jsonc
"chat_topics": [ "TALK_FREE_MERCHANTS_MERCHANT" ]
```

## "revert_to_itype"
(string, optional)

If not empty and a valid item id, the monster can be converted into this item by the player, when it's friendly. This is usually used for turrets and similar to revert the turret monster back into the turret item, which can be picked up and placed elsewhere.

## "starting_ammo"
(object, optional)

An object containing ammo that newly spawned monsters start with. This is useful for a monster that has a special attack that consumes ammo. Example:

```jsonc
"starting_ammo": { "9mm": 100, "40mm_frag": 100 }
```

## "upgrades"
(boolean or object, optional)

Controls how this monster is upgraded over time. It can either be the single value `false` (which is the default and disables upgrading) or an object describing the upgrades.

Example:

```jsonc
"upgrades": {
  "into_group": "GROUP_ZOMBIE_UPGRADE",
  "half_life": 28
}
```

The upgrades object may have the following members:

Field               | Description
---                 | ---
`half_life`         | (int) Days in which half of the monsters upgrade according to an approximated exponential progression. It is multiplied with the evolution scaling factor which defaults to 1.
`into_group`        | (string, optional) The upgraded monster's type is taken from the specified group.
`into`              | (string, optional) The upgraded monster's type.
`age_grow`          | (int, optional) Number of days needed for monster to change into another monster. Does not scale with the evolution factor.
`multiple_spawns`   | (bool, optional) If using `into_group`, the selected entry spawns a number of monsters based on the entry's `pack_size`.
`spawn_range`       | (int, optional) Mandatory when `multiple_spawns` is true. Determines how far away from the original monster the upgraded monsters can spawn.
`despawn_when_null` | (bool, optional) For `into_group`, when `mon_null` is selected as the group entry upgrade, the monster will despawn leaving no trace when this is true. Otherwise the monster "dies" naturally. Defaults to false.

## "reproduction"
(dictionary, optional)

The monster's reproduction cycle, if any. Supports:

Field          | Description
---            | ---
`baby_count`   | (int) Number of new creatures or eggs to spawn on reproduction.
`baby_timer`   | (int) Number of days between reproduction events.
`baby_type`    | (object) Specifies the type of reproduction. Types can be `{ "baby_type": { "baby_egg": "item_id" }`, `{ "baby_type": { "baby_egg_group": "item_group_id" }`, `{ "baby_type": { "baby_monster": "monster_id" }`, `{ "baby_type": { "baby_monster_group": "monster_group_id" }`. Only one type can be used.

## "zombify_into"
(monster string id, optional)

When defined the monster's unpulped corpse will rise, zombifying into the defined (different) monster. For mutated animals (including giant arthropods) the `mon_meat_cocoon` line of monsters should be defined, depending on the monster's weight:
No cocoon below 10 kg; 10 - 35 kg monsters zombify into the tiny cocoon; 36 - 100 kg monsters turn into the small cocoon; 101 - 300 kg monsters turn into the medium cocoon; 301+ kg monsters turn into a large cocoon.

## "revive_forms"

Advanced form of `zombify_into`, allows to specify reviving into different monsters depending on condition;

```jsonc
    "revive_forms": [
      {
        // condition as dialogue condition, but with neither alpha nor beta talkers
        // instead context variables `loc` (location of corpse) and `corpse_damage` are exposed
        // can be omitted, in this case condition would be assumed to always be TRUE
        "condition": { "map_terrain_with_flag": "SWIMMABLE", "loc": { "context_val": "loc" } },
        // either `monster` or `monster_group` should be used
        "monster": "pseudo_debug_mon",
        "monster_group": "GROUP_ANIMALPOUND_CATS"
      }
    ],
```

## "baby_flags"
(Array, optional)
Designate seasons during which this monster is capable of reproduction. ie: `[ "SPRING", "SUMMER" ]`

## "shearing"
(array of objects, optional)

A set of items that are given to the player when they shear this monster. These entries can be duplicates and are one of these 4 types:
```jsonc
"shearing": [
    {
        "result": "wool",
        "amount": 100        // exact amount
    },
    {
        "result": "rags",
        "amount": [10, 100]  // random number in range ( inclusive )
    },
    {
        "result": "leather",
        "ratio_mass": 0.25   // amount from percentage of mass ( kilograms )
    },
    {
        "result": "wool",
        "ratio_volume": 0.60 // amount from percentage of volume ( liters )
    }
]
```

This means that when this monster is sheared, it will give: 100 units of wool, 10 to 100 pieces of rag, 25% of its body mass as leather and 60% of its volume as wool.

## "speed_description"
(string, optional)

By default monsters will use the `"DEFAULT"` speed description.
```jsonc
"speed_description": "SPEED_DESCRIPTION_ID"
```

## "petfood"
(object, optional)

Decides whether this monster can be tamed. `%s` is the monster name.
```jsonc
"petfood": {
    "food": [ "CATFOOD", "YULECATFOOD" ], // food categories this monster accepts
    "feed": "The gigantic %s decides not to maul you today.", // (optional) message when feeding the monster the food
    "pet": "The %s is enjoying hunting the red laser dot." // (optional) message when playing with pet
}
```

## "special_when_hit"
(array, optional)

A special defense attack, triggered when the monster is attacked. It should contain an array with the id of the defense (see Monster defense attacks in [MONSTER_SPECIAL_ATTACKS.md](MONSTER_SPECIAL_ATTACKS.md)) and the chance for that defense to be actually triggered. Example:

```jsonc
"special_when_hit": [ "ZAPBACK", 100 ]
```

## "attack_effs"
(array of objects, optional)

A set of effects that may get applied to the attacked creature when the monster successfully attacks. Example:

```jsonc
"attack_effs": [
  {
    "id": "paralyzepoison",
    "duration": 33,
    "chance": 50
  }
]
```

Each element of the array should be an object containing the following members:

Field           | Description
---             | ---
`id`            | (string, required) The id of the effect that is to be applied.
`duration`      | (integer or a pair of integers, optional) How long (in turns) the effect should last. When defined with a pair of values the duration will be randomized between those.
`intensity`     | ( integer or a pair of integers, optional) What intensity the effect should be applied at, when defined as a pair the intensity will be randomized between them. Can't overwrite effects that derive their intensity from their duration via `int_dur_factor`.
`affect_hit_bp` | (boolean, optional) Whether the effect should be applied to the hit body part instead of the one set below.
`bp`            | (string, optional) The body part that where the effect is applied. The default is to apply the effect to the whole body. Note that some effects may require a specific body part (e.g. "hot") and others may require the whole body (e.g. "meth").
`permanent`     | (boolean, optional) Whether the effect is permanent, in which case the "duration" will be ignored. The default is non-permanent.
`chance`        | (integer, optional) The chance of the effect getting applied.
`message`       | (string, optional) Message to print when the effect is applied to the player. Supports dynamic lines with the syntax `%s = <the monster's name>`.

## "path_settings"
(object, optional)

Field                | Description
---                  | ---
`max_dist`           | (int, default 0) Maximum direct distance of path
`max_length`         | (int, default -1) Maximum total length of path
`bash_strength`      | (int, default -1) Monster strength when bashing through an obstacle
`allow_open_doors`   | (bool, default false) Monster knows how to open doors
`allow_unlock_doors` | (bool, default false) Monster knows how to unlock doors
`avoid_traps`        | (bool, default false) Monster avoids stepping into traps
`allow_climb_stairs` | (bool, default true) Monster may climb stairs
`avoid_rough_terrain` | (bool, default false) Monster may avoid rough terrain like rubble
`avoid_sharp`        | (bool, default false) Monster may avoid sharp things like barbed wire
`avoid_dangerous_fields` | (bool, default false) Monster may avoid dangerous fields like fire or acid

## "move_skills"
(object, optional)

Field                | Description
---                  | ---
`swim`               | (int, 0-10,optional) swimming monsters ignore SWIMMABLE terrain-cost. Instead it applies a flat movecost penalty inversly related to the skill.
`dig`                | (int, 0-10, optional) swimming monsters ignore DIGGABLE terrain-cost. Instead it applies a flat movecost penalty inversly related to the skill.
`climb`              | (int, 0-10, optional) climbing monsters can climb CLIMBABLE ter/furn and can use DIFFICULT_Z ter/furn (i.e. ladders). The ter/furn cost gets multiplied by the skill modifier


## "special_attacks"

See [MONSTER_SPECIAL_ATTACKS.md](MONSTER_SPECIAL_ATTACKS.md)
