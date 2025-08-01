#include "mapgen.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>

#include "all_enum_values.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character_id.h"
#include "city.h"
#include "clzones.h"
#include "common_types.h"
#include "computer.h"
#include "condition.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "cube_direction.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "dialogue.h"
#include "drawing_primitives.h"
#include "enum_conversions.h"
#include "enums.h"
#include "field_type.h"
#include "flat_set.h"
#include "game.h"
#include "generic_factory.h"
#include "global_vars.h"
#include "input.h"
#include "input_enums.h"
#include "item.h"
#include "item_category.h"
#include "item_factory.h"
#include "item_group.h"
#include "itype.h"
#include "jmapgen_flags.h"
#include "level_cache.h"
#include "line.h"
#include "localized_comparator.h"
#include "magic_ter_furn_transform.h"
#include "map.h"
#include "map_extras.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "mapbuffer.h"
#include "mapdata.h"
#include "mapgen_functions.h"
#include "mapgendata.h"
#include "mapgenformat.h"
#include "memory_fast.h"
#include "messages.h"
#include "mission.h"
#include "mongroup.h"
#include "npc.h"
#include "omdata.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "pocket_type.h"
#include "point.h"
#include "regional_settings.h"
#include "ret_val.h"
#include "rng.h"
#include "string_formatter.h"
#include "submap.h"
#include "talker.h"
#include "text_snippets.h"
#include "tileray.h"
#include "to_string_id.h"
#include "translation.h"
#include "translations.h"
#include "trap.h"
#include "uilist.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vehicle_group.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weighted_dbl_or_var_list.h"
#include "weighted_list.h"

static const field_type_str_id field_fd_blood( "fd_blood" );
static const field_type_str_id field_fd_fire( "fd_fire" );

static const furn_str_id furn_f_ash( "f_ash" );
static const furn_str_id furn_f_bed( "f_bed" );
static const furn_str_id furn_f_console( "f_console" );
static const furn_str_id furn_f_counter( "f_counter" );
static const furn_str_id furn_f_dresser( "f_dresser" );
static const furn_str_id furn_f_flower_fungal( "f_flower_fungal" );
static const furn_str_id furn_f_fridge( "f_fridge" );
static const furn_str_id furn_f_fungal_clump( "f_fungal_clump" );
static const furn_str_id furn_f_rack( "f_rack" );
static const furn_str_id furn_f_rubble( "f_rubble" );
static const furn_str_id furn_f_sign( "f_sign" );
static const furn_str_id furn_f_table( "f_table" );
static const furn_str_id furn_f_toilet( "f_toilet" );
static const furn_str_id furn_f_vending_c( "f_vending_c" );
static const furn_str_id furn_f_vending_c_networked( "f_vending_c_networked" );
static const furn_str_id furn_f_vending_c_off( "f_vending_c_off" );
static const furn_str_id furn_f_vending_reinforced( "f_vending_reinforced" );
static const furn_str_id furn_f_vending_reinforced_networked( "f_vending_reinforced_networked" );
static const furn_str_id furn_f_vending_reinforced_off( "f_vending_reinforced_off" );

static const item_group_id Item_spawn_data_ammo_rare( "ammo_rare" );
static const item_group_id Item_spawn_data_bed( "bed" );
static const item_group_id Item_spawn_data_bionics( "bionics" );
static const item_group_id Item_spawn_data_bionics_common( "bionics_common" );
static const item_group_id Item_spawn_data_chem_lab( "chem_lab" );
static const item_group_id Item_spawn_data_cleaning( "cleaning" );
static const item_group_id Item_spawn_data_cloning_vat( "cloning_vat" );
static const item_group_id Item_spawn_data_dissection( "dissection" );
static const item_group_id Item_spawn_data_dresser( "dresser" );
static const item_group_id Item_spawn_data_goo( "goo" );
static const item_group_id Item_spawn_data_guns_rare( "guns_rare" );
static const item_group_id Item_spawn_data_lab_dorm( "lab_dorm" );
static const item_group_id Item_spawn_data_mut_lab( "mut_lab" );
static const item_group_id Item_spawn_data_teleport( "teleport" );

static const itype_id itype_UPS_off( "UPS_off" );
static const itype_id itype_ash( "ash" );
static const itype_id itype_avgas( "avgas" );
static const itype_id itype_diesel( "diesel" );
static const itype_id itype_gasoline( "gasoline" );
static const itype_id itype_glass_shard( "glass_shard" );
static const itype_id itype_heavy_battery_cell( "heavy_battery_cell" );
static const itype_id itype_id_science( "id_science" );
static const itype_id itype_jp8( "jp8" );
static const itype_id itype_laser_rifle( "laser_rifle" );
static const itype_id itype_plasma( "plasma" );
static const itype_id itype_plasma_gun( "plasma_gun" );
static const itype_id itype_plut_cell( "plut_cell" );
static const itype_id itype_recipe_atomic_battery( "recipe_atomic_battery" );
static const itype_id itype_recipe_caseless( "recipe_caseless" );
static const itype_id itype_rm13_armor( "rm13_armor" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_v29( "v29" );
static const itype_id itype_water( "water" );

static const mongroup_id GROUP_BREATHER( "GROUP_BREATHER" );
static const mongroup_id GROUP_BREATHER_HUB( "GROUP_BREATHER_HUB" );
static const mongroup_id GROUP_FUNGI_FUNGALOID( "GROUP_FUNGI_FUNGALOID" );
static const mongroup_id GROUP_LAB( "GROUP_LAB" );
static const mongroup_id GROUP_LAB_CYBORG( "GROUP_LAB_CYBORG" );
static const mongroup_id GROUP_LAB_SECURITY( "GROUP_LAB_SECURITY" );
static const mongroup_id GROUP_NETHER( "GROUP_NETHER" );
static const mongroup_id GROUP_ROBOT_SECUBOT( "GROUP_ROBOT_SECUBOT" );
static const mongroup_id GROUP_TURRET( "GROUP_TURRET" );

static const oter_str_id oter_ants_es( "ants_es" );
static const oter_str_id oter_ants_esw( "ants_esw" );
static const oter_str_id oter_ants_ew( "ants_ew" );
static const oter_str_id oter_ants_lab( "ants_lab" );
static const oter_str_id oter_ants_lab_stairs( "ants_lab_stairs" );
static const oter_str_id oter_ants_ne( "ants_ne" );
static const oter_str_id oter_ants_nes( "ants_nes" );
static const oter_str_id oter_ants_nesw( "ants_nesw" );
static const oter_str_id oter_ants_new( "ants_new" );
static const oter_str_id oter_ants_ns( "ants_ns" );
static const oter_str_id oter_ants_nsw( "ants_nsw" );
static const oter_str_id oter_ants_sw( "ants_sw" );
static const oter_str_id oter_ants_wn( "ants_wn" );
static const oter_str_id oter_central_lab( "central_lab" );
static const oter_str_id oter_central_lab_core( "central_lab_core" );
static const oter_str_id oter_central_lab_finale( "central_lab_finale" );
static const oter_str_id oter_central_lab_stairs( "central_lab_stairs" );
static const oter_str_id oter_ice_lab( "ice_lab" );
static const oter_str_id oter_ice_lab_core( "ice_lab_core" );
static const oter_str_id oter_ice_lab_finale( "ice_lab_finale" );
static const oter_str_id oter_ice_lab_stairs( "ice_lab_stairs" );
static const oter_str_id oter_lab( "lab" );
static const oter_str_id oter_lab_core( "lab_core" );
static const oter_str_id oter_lab_finale( "lab_finale" );
static const oter_str_id oter_lab_stairs( "lab_stairs" );
static const oter_str_id oter_tower_lab( "tower_lab" );
static const oter_str_id oter_tower_lab_finale( "tower_lab_finale" );
static const oter_str_id oter_tower_lab_stairs( "tower_lab_stairs" );

static const oter_type_str_id oter_type_road( "road" );
static const oter_type_str_id oter_type_sewer( "sewer" );

static const ter_str_id ter_t_bars( "t_bars" );
static const ter_str_id ter_t_card_science( "t_card_science" );
static const ter_str_id ter_t_concrete_wall( "t_concrete_wall" );
static const ter_str_id ter_t_cvdbody( "t_cvdbody" );
static const ter_str_id ter_t_cvdmachine( "t_cvdmachine" );
static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_door_glass_frosted_c( "t_door_glass_frosted_c" );
static const ter_str_id ter_t_door_metal_c( "t_door_metal_c" );
static const ter_str_id ter_t_door_metal_locked( "t_door_metal_locked" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_floor_burnt( "t_floor_burnt" );
static const ter_str_id ter_t_fungus_floor_in( "t_fungus_floor_in" );
static const ter_str_id ter_t_fungus_wall( "t_fungus_wall" );
static const ter_str_id ter_t_grass( "t_grass" );
static const ter_str_id ter_t_marloss( "t_marloss" );
static const ter_str_id ter_t_radio_tower( "t_radio_tower" );
static const ter_str_id ter_t_reinforced_door_glass_c( "t_reinforced_door_glass_c" );
static const ter_str_id ter_t_reinforced_glass( "t_reinforced_glass" );
static const ter_str_id ter_t_reinforced_glass_lab( "t_reinforced_glass_lab" );
static const ter_str_id ter_t_rock_floor( "t_rock_floor" );
static const ter_str_id ter_t_sewage( "t_sewage" );
static const ter_str_id ter_t_stairs_down( "t_stairs_down" );
static const ter_str_id ter_t_stairs_up( "t_stairs_up" );
static const ter_str_id ter_t_strconc_floor( "t_strconc_floor" );
static const ter_str_id ter_t_thconc_floor( "t_thconc_floor" );
static const ter_str_id ter_t_thconc_floor_olight( "t_thconc_floor_olight" );
static const ter_str_id ter_t_vat( "t_vat" );
static const ter_str_id ter_t_wall_burnt( "t_wall_burnt" );
static const ter_str_id ter_t_water_sh( "t_water_sh" );

static const trait_id trait_NPC_STATIC_NPC( "NPC_STATIC_NPC" );

static const trap_str_id tr_dissector( "tr_dissector" );
static const trap_str_id tr_drain( "tr_drain" );
static const trap_str_id tr_glow( "tr_glow" );
static const trap_str_id tr_goo( "tr_goo" );
static const trap_str_id tr_hum( "tr_hum" );
static const trap_str_id tr_portal( "tr_portal" );
static const trap_str_id tr_shadow( "tr_shadow" );
static const trap_str_id tr_snake( "tr_snake" );
static const trap_str_id tr_telepad( "tr_telepad" );

static const vproto_id vehicle_prototype_shopping_cart( "shopping_cart" );

#define dbg(x) DebugLog((x),D_MAP_GEN) << __FILE__ << ":" << __LINE__ << ": "

static constexpr int MON_RADIUS = 3;

static void science_room( map *m, const point_bub_ms &p1, const point_bub_ms &p2, int z,
                          int rotate );

enum class blood_trail_direction : int {
    first = 1,
    NORTH = 1,
    SOUTH = 2,
    EAST = 3,
    WEST = 4,
    last = 4
};

static tripoint_bub_ms get_point_from_direction( int direction,
        const tripoint_bub_ms &current_tile )
{
    switch( static_cast<blood_trail_direction>( direction ) ) {
        case blood_trail_direction::NORTH:
            return tripoint_bub_ms( current_tile.x(), current_tile.y() - 1, current_tile.z() );
        case blood_trail_direction::SOUTH:
            return tripoint_bub_ms( current_tile.x(), current_tile.y() + 1, current_tile.z() );
        case blood_trail_direction::WEST:
            return tripoint_bub_ms( current_tile.x() - 1, current_tile.y(), current_tile.z() );
        case blood_trail_direction::EAST:
            return tripoint_bub_ms( current_tile.x() + 1, current_tile.y(), current_tile.z() );
    }
    // This shouldn't happen unless the function is used incorrectly
    debugmsg( "Attempted to get point from invalid direction.  %d", direction );
    return current_tile;
}

static bool tile_can_have_blood( map &md, const tripoint_bub_ms &current_tile,
                                 bool wall_streak, int days_since_cataclysm )
{
    // Wall streaks stick to walls, like blood was splattered against the surface
    // Floor streaks avoid obstacles to look more like a person left the streak behind (Not walking through walls, closed doors, windows, or furniture)
    if( wall_streak ) {
        return md.has_flag_ter( ter_furn_flag::TFLAG_WALL, current_tile ) &&
               !md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile );
    } else {
        if( !md.has_flag_ter( ter_furn_flag::TFLAG_INDOORS, current_tile ) &&
            x_in_y( days_since_cataclysm, 30 ) ) {
            // Placement of blood outdoors scales down over the course of 30 days until no further blood is placed.
            return false;
        }

        return !md.has_flag_ter( ter_furn_flag::TFLAG_WALL, current_tile ) &&
               !md.has_flag_ter( ter_furn_flag::TFLAG_WINDOW, current_tile ) &&
               !md.has_flag_ter( ter_furn_flag::TFLAG_DOOR, current_tile ) &&
               !md.has_furn( current_tile );
    }

}

static void place_blood_on_adjacent( map &md, const tripoint_bub_ms &current_tile, int chance,
                                     int days_since_catacylsm )
{
    for( int i = static_cast<int>( blood_trail_direction::first );
         i <= static_cast<int>( blood_trail_direction::last ); i++ ) {
        tripoint_bub_ms adjacent_tile = get_point_from_direction( i, current_tile );

        if( !tile_can_have_blood( md, adjacent_tile, false, days_since_catacylsm ) ) {
            continue;
        }
        if( rng( 1, 100 ) < chance ) {
            md.add_field( adjacent_tile, field_fd_blood );
        }
    }
}

static void place_blood_streaks( map &md, const tripoint_bub_ms &current_tile,
                                 int days_since_cataclysm )
{
    int streak_length = rng( 3, 12 );
    int streak_direction = rng( static_cast<int>( blood_trail_direction::first ),
                                static_cast<int>( blood_trail_direction::last ) );

    bool wall_streak = md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_WALL, current_tile );

    if( !tile_can_have_blood( md, current_tile, wall_streak, days_since_cataclysm ) ) {
        // Quick check the tile is valid.
        return;
    }

    md.add_field( current_tile, field_fd_blood );
    tripoint_bub_ms last_tile = current_tile;

    for( int i = 0; i < streak_length; i++ ) {
        tripoint_bub_ms destination_tile = get_point_from_direction( streak_direction, last_tile );

        if( !tile_can_have_blood( md, destination_tile, wall_streak, days_since_cataclysm ) ) {
            // We hit a non-valid tile. Try to find a new direction otherwise just terminate the streak.
            bool terminate_streak = true;

            for( int ii = static_cast<int>( blood_trail_direction::first );
                 ii <= static_cast<int>( blood_trail_direction::last );
                 ii++ ) {
                if( ii == streak_direction ) {
                    // We don't want to check the direction we came from. No turning around!
                    continue;
                }
                tripoint_bub_ms adjacent_tile = get_point_from_direction( ii, last_tile );

                if( tile_can_have_blood( md, adjacent_tile, wall_streak, days_since_cataclysm ) ) {
                    streak_direction = ii;
                    destination_tile = adjacent_tile;
                    terminate_streak = false;
                    break;
                }
            }

            if( terminate_streak ) {
                break;
            }
        }

        if( rng( 1, 100 ) < 5 + i * 3 ) {
            // Sometimes a streak should skip a tile, the chance increases with each step
            last_tile = destination_tile;
            continue;
        }

        if( rng( 1, 100 ) < 10 + i * 5 ) {
            // Sometimes a streak should end early, the chance increases with each step.
            // This is just a hack to further weight the distribution in favor of short streaks over long ones.
            break;
        }

        md.add_field( destination_tile, field_fd_blood );
        last_tile = destination_tile;


        if( ( rng( 1, 100 ) < 30 + i * 3 ) && !wall_streak ) {
            // Floor streaks can meander and the probability of meandering increases with each step
            // Long straight streaks aren't visually interesting. So sometimes a streak will curve by meandering to the side.
            int new_direction = 0;
            if( streak_direction == static_cast<int>( blood_trail_direction::NORTH ) ||
                streak_direction == static_cast<int>( blood_trail_direction::SOUTH ) ) {
                new_direction = one_in( 2 ) ? static_cast<int>( blood_trail_direction::EAST ) : static_cast<int>(
                                    blood_trail_direction::WEST );
            } else {
                new_direction = one_in( 2 ) ? static_cast<int>( blood_trail_direction::NORTH ) : static_cast<int>(
                                    blood_trail_direction::SOUTH );
            }

            tripoint_bub_ms adjacent_tile = get_point_from_direction( new_direction, last_tile );
            if( tile_can_have_blood( md, adjacent_tile, wall_streak, days_since_cataclysm ) ) {
                md.add_field( adjacent_tile, field_fd_blood );
                last_tile = adjacent_tile;
            }
        }
    }
}

static void place_bool_pools( map &md, const tripoint_bub_ms &current_tile,
                              int days_since_cataclysm )
{
    if( !tile_can_have_blood( md, current_tile, false, days_since_cataclysm ) ) {
        // Quick check the first tile is valid for placement
        return;
    }

    md.add_field( current_tile, field_fd_blood );
    place_blood_on_adjacent( md, current_tile, 60, days_since_cataclysm );

    for( int i = static_cast<int>( blood_trail_direction::first );
         i <= static_cast<int>( blood_trail_direction::last ); i++ ) {
        tripoint_bub_ms adjacent_tile = get_point_from_direction( i, current_tile );
        if( !tile_can_have_blood( md, adjacent_tile, false, days_since_cataclysm ) ) {
            continue;
        }
        place_blood_on_adjacent( md, adjacent_tile, 30, days_since_cataclysm );
    }
}

struct generator_vars {
    int scaling_days_start;
    int scaling_days_end;
    int num_attempts;
    int percent_chance;
    int min_intensity;
    int max_intensity;
};

static void GENERATOR_bash_damage( map &md,
                                   std::list<tripoint_bub_ms> &all_points_in_map,
                                   int days_since_cataclysm )
{
    // Later, this will be loaded from json.
    generator_vars bash_vars{};
    bash_vars.scaling_days_start = 0; // irrelevant for this one
    bash_vars.scaling_days_end = days_since_cataclysm; // irrelevant for this one
    bash_vars.num_attempts = 250; // Roughly half as many attempts as old version, may need tweaking
    bash_vars.percent_chance = 10;
    bash_vars.min_intensity = 6; // For this generator: Bash damage
    bash_vars.max_intensity = 60; // For this generator: Bash damage

    for( int i = 0; i < bash_vars.num_attempts; i++ ) {
        if( !x_in_y( bash_vars.percent_chance, 100 ) ) {
            continue; // failed roll
        }
        const tripoint_bub_ms current_tile = random_entry( all_points_in_map );
        if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ) {
            continue;
        }
        md.bash( current_tile, rng( bash_vars.min_intensity, bash_vars.max_intensity ) );
    }
}

static void GENERATOR_move_items( map &md,
                                  std::list<tripoint_bub_ms> &all_points_in_map,
                                  int days_since_cataclysm )
{
    // Later, this will be loaded from json.
    generator_vars mover_vars{};
    mover_vars.scaling_days_start = 0; // irrelevant for this one, currently.
    mover_vars.scaling_days_end = days_since_cataclysm; // irrelevant for this one

    // NOTE: Each tile of items is fully iterated over, to eliminate the effects of stack ordering.
    // Otherwise we would be biased towards the front of the stack
    mover_vars.num_attempts = 250; // Roughly half as many attempts as old version, may need tweaking

    mover_vars.percent_chance = 10;
    mover_vars.min_intensity = 0; // For this generator: Min distance moved. Note: NOT IMPLEMENTED
    mover_vars.max_intensity = 3; // For this generator: Max distance moved

    for( int i = 0; i < mover_vars.num_attempts; i++ ) {
        const tripoint_bub_ms current_tile = random_entry( all_points_in_map );
        if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ) {
            continue;
        }
        auto item_iterator = md.i_at( current_tile.xy() ).begin();
        while( item_iterator != md.i_at( current_tile.xy() ).end() ) {
            // Some items must not be moved out of SEALED CONTAINER
            if( md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_SEALED, current_tile ) &&
                md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_CONTAINER, current_tile ) ) {
                // Seed should stay in their planter for map::grow_plant to grow them
                if( md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_PLANT, current_tile ) &&
                    item_iterator->is_seed() ) {
                    item_iterator++;
                    continue;
                }
            }

            if( x_in_y( mover_vars.percent_chance, 100 ) ) {
                // pick a new spot...
                tripoint_bub_ms destination_tile(
                    current_tile.x() + rng( -mover_vars.max_intensity, mover_vars.max_intensity ),
                    current_tile.y() + rng( -mover_vars.max_intensity, mover_vars.max_intensity ),
                    current_tile.z() );
                // oops, don't place out of bounds. just skip moving
                const bool outbounds_X = destination_tile.x() < 0 || destination_tile.x() >= SEEX * 2;
                const bool outbounds_Y = destination_tile.y() < 0 || destination_tile.y() >= SEEY * 2;
                const bool cannot_place = md.has_flag( ter_furn_flag::TFLAG_DESTROY_ITEM, destination_tile ) ||
                                          md.has_flag( ter_furn_flag::TFLAG_NOITEM, destination_tile ) || !md.has_floor( destination_tile );
                if( outbounds_X || outbounds_Y || cannot_place ) {
                    item_iterator++;
                    continue;
                } else {
                    item copy( *item_iterator );
                    // add a copy of our item to the destination...
                    md.add_item( destination_tile, copy );
                    // and erase the one at our source.
                    item_iterator = md.i_at( current_tile.xy() ).erase( item_iterator );
                }
            } else {
                item_iterator++;
            }

        }
    }

}

static void GENERATOR_add_fire( map &md,
                                std::list<tripoint_bub_ms> &all_points_in_map,
                                int days_since_cataclysm )
{
    // Later, this will be loaded from json.
    generator_vars fire_vars{};
    fire_vars.scaling_days_start = 0;
    fire_vars.scaling_days_end = 14;

    // Placeholder. Number selected so that the # of fires is close to the old implementation.
    fire_vars.num_attempts = 2;

    // FIXME? I'm concerned by the fact that the initial chance scales higher the later the last scaling day is.
    // This is not great, but it ramps down linearly without relying on magic numbers.
    fire_vars.percent_chance = std::max( fire_vars.scaling_days_end - days_since_cataclysm, 0 );

    fire_vars.min_intensity = 1; // For this generator: field intensity
    fire_vars.max_intensity = 3; // For this generator: field intensity

    for( int i = 0; i < fire_vars.num_attempts; i++ ) {
        if( !x_in_y( fire_vars.percent_chance, 100 ) ) {
            continue; // failed roll
        }
        const tripoint_bub_ms current_tile = random_entry( all_points_in_map );
        if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ) {
            continue;
        }

        if( x_in_y( fire_vars.percent_chance, 100 ) ) {
            // FIXME: Magic number 3. Replace with some value loaded into generator_vars?
            if( md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FLAMMABLE, current_tile ) ||
                md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FLAMMABLE_ASH, current_tile ) ||
                md.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FLAMMABLE_HARD, current_tile ) ||
                days_since_cataclysm < 3 ) {
                // Only place fire on flammable surfaces unless the cataclysm started very recently
                // Note that most floors are FLAMMABLE_HARD, this is fine. This check is primarily geared
                // at preventing fire in the middle of roads or parking lots.
                md.add_field( current_tile, field_fd_fire,
                              rng( fire_vars.min_intensity, fire_vars.max_intensity ) );
            }
        }

    }

}

static void GENERATOR_pre_burn( map &md,
                                std::list<tripoint_bub_ms> &all_points_in_map,
                                int days_since_cataclysm )
{
    // Later, this will be loaded from json.
    generator_vars burnt_vars{};
    // Fires are still raging around this time, but some start appearing
    // Never appears before this date
    burnt_vars.scaling_days_start = 3;

    burnt_vars.scaling_days_end = 14; // Continues appearing at maximum appearance rate after this day
    burnt_vars.num_attempts = 1; // Currently only applied to the whole map, so one pass.

    burnt_vars.min_intensity = 6; // For this generator: % chance at start day
    burnt_vars.max_intensity = 28; // For this generator: % chance at end day

    // between start and end day we linearly interpolate.
    double lerp_scalar = static_cast<double>(
                             static_cast<double>( days_since_cataclysm - burnt_vars.scaling_days_start ) /
                             static_cast<double>( burnt_vars.scaling_days_end - burnt_vars.scaling_days_start ) );
    burnt_vars.percent_chance = lerp( burnt_vars.min_intensity, burnt_vars.max_intensity, lerp_scalar );
    // static values outside that range. Note we do not use std::clamp because the chance is *0* until the start day is reached
    if( days_since_cataclysm < burnt_vars.scaling_days_start ) {
        burnt_vars.percent_chance = 0;
    } else if( days_since_cataclysm >= burnt_vars.scaling_days_end ) {
        burnt_vars.percent_chance = burnt_vars.max_intensity;
    }

    for( int i = 0; i < burnt_vars.num_attempts; i++ ) {
        if( !x_in_y( burnt_vars.percent_chance, 100 ) ) {
            continue; // failed roll
        }
        for( tripoint_bub_ms current_tile : all_points_in_map ) {
            if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ||
                md.has_flag_ter( ter_furn_flag::TFLAG_GOES_DOWN, current_tile ) ||
                md.has_flag_ter( ter_furn_flag::TFLAG_GOES_UP, current_tile ) ) {
                // skip natural underground walls, or any stairs. (Even man-made or wooden stairs)
                continue;
            }
            if( md.has_flag_ter( ter_furn_flag::TFLAG_WALL, current_tile ) ) {
                // burnt wall
                md.ter_set( current_tile.xy(), ter_t_wall_burnt );
            } else if( md.has_flag_ter( ter_furn_flag::TFLAG_INDOORS, current_tile ) ||
                       md.has_flag_ter( ter_furn_flag::TFLAG_DOOR, current_tile ) ) {
                // if we're indoors but we're not a wall, then we must be a floor.
                // doorways also get burned to the ground.
                md.ter_set( current_tile.xy(), ter_t_floor_burnt );
            } else if( !md.has_flag_ter( ter_furn_flag::TFLAG_INDOORS, current_tile ) ) {
                // if we're outside on ground level, burn it to dirt.
                if( current_tile.z() == 0 ) {
                    md.ter_set( current_tile.xy(), ter_t_dirt );
                }
            }

            // destroy any furniture that is in the tile. it's been burned, after all.
            md.furn_set( current_tile.xy(), furn_str_id::NULL_ID() );

            // destroy all items in the tile.
            md.i_clear( current_tile.xy() );
        }
    }
}

static void GENERATOR_riot_damage( map &md, const tripoint_abs_omt &p, bool is_a_road )
{
    std::list<tripoint_bub_ms> all_points_in_map;


    int days_since_cataclysm = to_days<int>( calendar::turn - calendar::start_of_cataclysm );

    // Placeholder / FIXME
    // This assumes that we're only dealing with regular 24x24 OMTs. That is likely not the case.
    for( int i = 0; i < SEEX * 2; i++ ) {
        for( int n = 0; n < SEEY * 2; n++ ) {
            tripoint_bub_ms current_tile( i, n, p.z() );
            all_points_in_map.push_back( current_tile );
        }
    }

    // Run sub generators associated with this generator. Currently hardcoded.
    GENERATOR_bash_damage( md, all_points_in_map, days_since_cataclysm );
    GENERATOR_move_items( md, all_points_in_map, days_since_cataclysm );
    GENERATOR_add_fire( md, all_points_in_map, days_since_cataclysm );
    // HACK: Don't burn roads to the ground! This should be resolved when the system is moved to json
    if( !is_a_road ) {
        GENERATOR_pre_burn( md, all_points_in_map, days_since_cataclysm );
    }

    // NOTE: Below currently only runs for bloodstains.
    for( size_t i = 0; i < all_points_in_map.size(); i++ ) {
        // Pick a tile at random!
        tripoint_bub_ms current_tile = random_entry( all_points_in_map );

        // Do nothing at random!;
        if( x_in_y( 10, 100 ) ) {
            continue;
        }
        // Skip naturally occuring underground wall tiles
        if( md.has_flag_ter( ter_furn_flag::TFLAG_NATURAL_UNDERGROUND, current_tile ) ) {
            continue;
        }
        // Set some fields at random!
        if( x_in_y( 15, 1000 ) ) {
            int behavior_roll = rng( 1, 100 );

            if( behavior_roll <= 20 ) {
                if( tile_can_have_blood( md, current_tile, md.has_flag_ter( ter_furn_flag::TFLAG_WALL,
                                         current_tile ), days_since_cataclysm ) ) {
                    md.add_field( current_tile, field_fd_blood );
                }
            } else if( behavior_roll <= 60 ) {
                place_blood_streaks( md, current_tile, days_since_cataclysm );
            } else {
                place_bool_pools( md, current_tile, days_since_cataclysm );
            }
        }
    }
}

// Assumptions:
// - The map supplied is empty, i.e. no grid entries are in use
// - The map supports Z levels.
void map::generate( const tripoint_abs_omt &p, const time_point &when, bool save_results,
                    bool run_post_process )
{
    dbg( D_INFO ) << "map::generate( g[" << g.get() << "], p[" << p << "], "
                  "when[" << to_string( when ) << "] )";

    const tripoint_abs_sm p_sm_base = project_to<coords::sm>( p );
    std::vector<bool> generated;
    generated.resize( my_MAPSIZE * my_MAPSIZE * OVERMAP_LAYERS );

    // Prepare the canvas...
    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            for( int gridz = -OVERMAP_DEPTH; gridz <= OVERMAP_HEIGHT; gridz++ ) {
                const tripoint_rel_sm pos( gridx, gridy, gridz );
                const size_t grid_pos = get_nonant( pos );
                // For some reason 'emplace' doesn't work. emplacing data later overwrote data...
                generated[grid_pos] = MAPBUFFER.submap_exists( p_sm_base.xy() + pos );

                if( !generated.at( grid_pos ) || !save_results ) {
                    setsubmap( grid_pos, new submap() );

                    // Generate uniform submaps immediately and cheaply.
                    // This causes them to be available for "proper" overlays even if on a lower Z level.
                    const std::optional<ter_str_id> ter = overmap_buffer.ter( { p.xy(), gridz } )->get_uniform_terrain();
                    if( ter ) {
                        getsubmap( grid_pos )->set_all_ter( *ter, true );
                        getsubmap( grid_pos )->last_touched = calendar::turn;
                    }
                } else {
                    setsubmap( grid_pos, MAPBUFFER.lookup_submap( p_sm_base.xy() + pos ) );
                }
            }
        }
    }

    std::vector<submap *> saved_overlay;
    saved_overlay.reserve( 4 );
    for( size_t index = 0; index <= 3; index++ ) {
        saved_overlay.emplace_back( nullptr );
    }

    // We're generating all Z levels in one go to be able to account for dependencies
    // between levels. We iterate from the top down based on the assumption it is
    // more common to add overlays on other Z levels upwards than downwards, so
    // going downwards we can immediately apply overlays onto the already generated
    // map, while overlays further down will have to be reapplied when the basic
    // map exists.

    for( int gridz = OVERMAP_HEIGHT; gridz >= -OVERMAP_DEPTH; gridz-- ) {
        const tripoint_abs_sm p_sm = { p_sm_base.xy(), gridz };
        set_abs_sub( p_sm );

        const oter_id &terrain_type = overmap_buffer.ter( tripoint_abs_omt( p.xy(), gridz ) );

        for( int gridx = 0; gridx <= 1; gridx++ ) {
            for( int gridy = 0; gridy <= 1; gridy++ ) {
                const tripoint_rel_sm pos( gridx, gridy, gridz );
                const size_t grid_pos = get_nonant( pos );

                if( ( !generated.at( grid_pos ) || !save_results ) &&
                    !getsubmap( grid_pos )->is_uniform() &&
                    !terrain_type->has_uniform_terrain() ) {
                    saved_overlay[gridx + gridy * 2] = getsubmap( grid_pos );
                    setsubmap( grid_pos, new submap() );
                }
            }
        }

        // This attempts to scale density of zombies inversely with distance from the nearest city.
        // In other words, make city centers dense and perimeters sparse.
        float density = 0.0f;
        for( int i = -MON_RADIUS; i <= MON_RADIUS; i++ ) {
            for( int j = -MON_RADIUS; j <= MON_RADIUS; j++ ) {
                density += overmap_buffer.ter( { p.x() + i, p.y() + j, gridz } )->get_mondensity();
            }
        }
        density = density / 100;

        // Not sure if we actually have to check all submaps.
        const bool any_missing = !generated.at( get_nonant( tripoint_rel_sm{ point_rel_sm::zero, p_sm.z() } ) )
                                 ||
                                 !generated.at( get_nonant( tripoint_rel_sm{ point_rel_sm::east, p_sm.z() } ) ) ||
                                 !generated.at( get_nonant( tripoint_rel_sm{ point_rel_sm::south_east, p_sm.z() } ) ) ||
                                 !generated.at( get_nonant( tripoint_rel_sm{ point_rel_sm::south, p_sm.z() } ) );

        mapgendata dat( { p.xy(), gridz}, *this, density, when, nullptr );
        if( ( any_missing || !save_results ) &&
            !terrain_type->has_uniform_terrain() ) {
            draw_map( dat );
        }

        // Merge the overlays generated earlier into the current Z level now we have the base map on it.
        for( int gridx = 0; gridx <= 1; gridx++ ) {
            for( int gridy = 0; gridy <= 1; gridy++ ) {
                const tripoint_rel_sm pos( gridx, gridy, gridz );
                const size_t index = gridx + gridy * 2;
                if( saved_overlay.at( index ) != nullptr ) {
                    const size_t grid_pos = get_nonant( pos );
                    getsubmap( grid_pos )->merge_submaps( saved_overlay.at( index ), true );
                    delete saved_overlay.at( index );
                    saved_overlay[index] = nullptr;
                }
            }
        }

        if( any_missing || !save_results ) {

            // At some point, we should add region information so we can grab the appropriate extras
            map_extras &this_ex = region_settings_map["default"].region_extras[terrain_type->get_extras()];
            map_extras ex = this_ex.filtered_by( dat );
            if( this_ex.chance > 0 && ex.values.empty() && !this_ex.values.empty() ) {
                DebugLog( D_WARNING, D_MAP_GEN ) << "Overmap terrain " << terrain_type->get_type_id().str() <<
                                                 " (extra type \"" << terrain_type->get_extras() <<
                                                 "\") zlevel = " << p.z() <<
                                                 " is out of range of all assigned map extras.  Skipping map extra generation.";
            } else if( ex.chance > 0 && one_in( ex.chance ) ) {
                map_extra_id *extra = ex.values.pick();
                if( extra == nullptr ) {
                    debugmsg( "failed to pick extra for type %s (ter = %s)", terrain_type->get_extras(),
                              terrain_type->get_type_id().str() );
                } else {
                    MapExtras::apply_function( *ex.values.pick(), *this, tripoint_abs_sm( abs_sub ) );
                }
            }

            const overmap_static_spawns &spawns = terrain_type->get_static_spawns();

            float spawn_density = 1.0f;
            if( MonsterGroupManager::is_animal( spawns.group ) ) {
                spawn_density = get_option< float >( "SPAWN_ANIMAL_DENSITY" );
            } else {
                spawn_density = get_option< float >( "SPAWN_DENSITY" );
            }

            // Apply a multiplier to the number of monsters for really high densities.
            float odds_after_density = spawns.chance * spawn_density;
            const float max_odds = 100 - ( 100 - spawns.chance ) / 2.0f;
            float density_multiplier = 1.0f;
            if( odds_after_density > max_odds ) {
                density_multiplier = 1.0f * odds_after_density / max_odds;
                odds_after_density = max_odds;
            }
            const int spawn_count = roll_remainder( density_multiplier );

            if( spawns.group && x_in_y( odds_after_density, 100 ) ) {
                int pop = spawn_count * rng( spawns.population.min, spawns.population.max );
                for( ; pop > 0; pop-- ) {
                    std::vector<MonsterGroupResult> spawn_details =
                        MonsterGroupManager::GetResultFromGroup( spawns.group, &pop );
                    for( const MonsterGroupResult &mgr : spawn_details ) {
                        if( !mgr.id ) {
                            continue;
                        }
                        if( const std::optional<tripoint_bub_ms> pt =
                        random_point_on_level( *this, gridz, [this]( const tripoint_bub_ms & n ) {
                        return passable_through( n );
                        } ) ) {
                            const tripoint_bub_ms pnt = pt.value();
                            add_spawn( mgr, pnt );
                        }
                    }
                }
            }
        }

        // Apply post-process generators
        if( run_post_process ) {
            if( any_missing || !save_results ) {
                const tripoint_abs_omt omt_point = { p.x(), p.y(), gridz };
                oter_id omt = overmap_buffer.ter( omt_point );
                if( omt->has_flag( oter_flags::pp_generate_riot_damage ) && !omt->has_flag( oter_flags::road ) ) {
                    GENERATOR_riot_damage( *this, omt_point, false );
                } else if( omt->has_flag( oter_flags::road ) && overmap_buffer.is_in_city( omt_point ) ) {
                    // HACK: Hardcode running only certain sub-generators on roads
                    GENERATOR_riot_damage( *this, omt_point, true );
                }
            }
        }
    }

    if( save_results ) {
        for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
            for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
                for( int gridz = -OVERMAP_DEPTH; gridz <= OVERMAP_HEIGHT; gridz++ ) {
                    const tripoint_rel_sm pos( gridx, gridy, gridz );
                    const size_t grid_pos = get_nonant( pos );
                    if( !generated.at( grid_pos ) ) {
                        if( gridx <= 1 && gridy <= 1 ) {
                            saven( { gridx, gridy, gridz } );
                        } else {
                            delete getsubmap( grid_pos );
                        }
                    }
                }
            }
        }
    }

    set_abs_sub( p_sm_base );
}

void map::delete_unmerged_submaps()
{
    tripoint_abs_sm sm_base = get_abs_sub();

    for( size_t index = 0; index < grid.size(); index++ ) {
        tripoint offset;
        const int ix = static_cast<int>( index );

        // This is the inverse of get_nonant.
        if( zlevels ) {
            offset = { ( ix / OVERMAP_LAYERS ) % my_MAPSIZE, ix / OVERMAP_LAYERS / my_MAPSIZE, ix % OVERMAP_LAYERS - OVERMAP_DEPTH };
        } else {
            offset = { ix % my_MAPSIZE, ix / my_MAPSIZE, sm_base.z()};
        }

        if( grid[index] != nullptr && MAPBUFFER.lookup_submap( sm_base.xy() + offset ) != grid[index] ) {
            delete grid[index];
            grid[index] = nullptr;
        }
    }
}

void mapgen_function_builtin::generate( mapgendata &mgd )
{
    ( *fptr )( mgd );
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
///// mapgen_function class.
///// all sorts of ways to apply our hellish reality to a grid-o-squares

class mapgen_basic_container
{
    private:
        std::vector<std::shared_ptr<mapgen_function>> mapgens_;
        //mapgens that need to be recalculated with a function when spawned
        std::vector<std::shared_ptr<mapgen_function>> mapgens_to_recalc_;
        weighted_int_list<std::shared_ptr<mapgen_function>> weights_;

    public:
        int add( const std::shared_ptr<mapgen_function> &ptr ) {
            cata_assert( ptr );
            if( std::find( mapgens_.begin(), mapgens_.end(), ptr ) != mapgens_.end() ) {
                debugmsg( "Adding duplicate mapgen to container!" );
            }
            mapgens_.push_back( ptr );
            return mapgens_.size() - 1;
        }
        /**
         * Pick a mapgen function randomly and call its generate function.
         * This basically runs the mapgen functions with the given @ref mapgendata
         * as argument.
         * @return Whether the mapgen function has been run. It may not get run if
         * the list of mapgen functions is effectively empty.
         * @p hardcoded_weight Weight for an additional entry. If that entry is chosen,
         * false is returned. If unsure, just use 0 for it.
         */
        bool generate( mapgendata &dat, const int hardcoded_weight ) {
            for( const std::shared_ptr<mapgen_function> &ptr : mapgens_to_recalc_ ) {
                dialogue d( get_talker_for( get_avatar() ), std::make_unique<talker>() );
                int const weight = ptr->weight.evaluate( d );
                if( weight >= 1 ) {
                    weights_.add_or_replace( ptr, weight );
                } else {
                    weights_.remove( ptr );
                }
            }

            if( hardcoded_weight > 0 &&
                rng( 1, weights_.get_weight() + hardcoded_weight ) > weights_.get_weight() ) {
                return false;
            }
            const std::shared_ptr<mapgen_function> *const ptr = weights_.pick();
            if( !ptr ) {
                return false;
            }
            cata_assert( *ptr );
            ( *ptr )->generate( dat );
            return true;
        }
        /**
         * Calls @ref mapgen_function::setup and sets up the internal weighted list using
         * the **current** value of @ref mapgen_function::weight. This value may have
         * changed since it was first added, so this is needed to recalculate the weighted list.
         */
        void setup() {
            for( const std::shared_ptr<mapgen_function> &ptr : mapgens_ ) {
                cata_assert( ptr->weight );
                if( ptr->weight.is_constant() ) {
                    int const weight = ptr->weight.constant();
                    if( weight < 1 ) {
                        continue; // rejected!
                    }
                    weights_.add( ptr, weight );
                } else {
                    mapgens_to_recalc_.push_back( ptr );
                }

                ptr->setup();
            }
            // Not needed anymore, pointers are now stored in weights_ (or not used at all)
            mapgens_.clear();
        }
        void finalize_parameters() {
            for( auto &mapgen_function_ptr : weights_ ) {
                mapgen_function_ptr.obj->finalize_parameters();
            }
        }
        void check_consistency() const {
            for( const auto &mapgen_function_ptr : weights_ ) {
                mapgen_function_ptr.obj->check();
            }
        }
        void check_consistency_with( const oter_t &ter ) const {
            for( const auto &mapgen_function_ptr : weights_ ) {
                mapgen_function_ptr.obj->check_consistent_with( ter );
            }
        }

        mapgen_parameters get_mapgen_params( mapgen_parameter_scope scope,
                                             const std::string &context ) const {
            mapgen_parameters result;
            for( const weighted_object<int, std::shared_ptr<mapgen_function>> &p : weights_ ) {
                result.check_and_merge( p.obj->get_mapgen_params( scope ), context );
            }
            return result;
        }
};

class mapgen_factory
{
    private:
        std::map<std::string, mapgen_basic_container> mapgens_;

        /// Collect all the possible and expected keys that may get used with @ref pick.
        static std::set<std::string> get_usages() {
            std::set<std::string> result;
            for( const oter_t &elem : overmap_terrains::get_all() ) {
                result.insert( elem.get_mapgen_id() );
                result.insert( elem.id.str() );
            }
            // Why do I have to repeat the MapExtras here? Wouldn't "MapExtras::factory" be enough?
            for( const map_extra &elem : MapExtras::mapExtraFactory().get_all() ) {
                if( elem.generator_method == map_extra_method::mapgen ) {
                    result.insert( elem.generator_id );
                }
            }
            // Used in C++ code only, see calls to `oter_mapgen.generate()` below
            result.insert( "lab_1side" );
            result.insert( "lab_4side" );
            result.insert( "lab_finale_1level" );
            return result;
        }

    public:
        void reset() {
            mapgens_.clear();
        }
        /// @see mapgen_basic_container::setup
        void setup() {
            for( std::pair<const std::string, mapgen_basic_container> &omw : mapgens_ ) {
                omw.second.setup();
                inp_mngr.pump_events();
            }
            // Dummy entry, overmap terrain null should never appear and is
            // therefore never generated.
            mapgens_.erase( "null" );
        }
        void finalize_parameters() {
            for( std::pair<const std::string, mapgen_basic_container> &omw : mapgens_ ) {
                omw.second.finalize_parameters();
            }
        }
        void check_consistency() const {
            // Cache all strings that may get looked up here so we don't have to go through
            // all the sources for them upon each loop.
            const std::set<std::string> usages = get_usages();
            for( const std::pair<const std::string, mapgen_basic_container> &omw : mapgens_ ) {
                omw.second.check_consistency();
                if( usages.count( omw.first ) == 0 ) {
                    debugmsg( "Mapgen %s is not used by anything!", omw.first );
                }
            }
        }
        /**
         * Checks whether we have an entry for the given key.
         * Note that the entry itself may not contain any valid mapgen instance
         * (could all have been removed via @ref erase).
         */
        bool has( const std::string &key ) const {
            return mapgens_.count( key ) != 0;
        }
        const mapgen_basic_container *find( const std::string &key ) const {
            auto it = mapgens_.find( key );
            if( it == mapgens_.end() ) {
                return nullptr;
            } else {
                return &it->second;
            }
        }
        /// @see mapgen_basic_container::add
        int add( const std::string &key, const std::shared_ptr<mapgen_function> &ptr ) {
            return mapgens_[key].add( ptr );
        }
        /// @see mapgen_basic_container::generate
        bool generate( mapgendata &dat, const std::string &key, const int hardcoded_weight = 0 ) {
            const auto iter = mapgens_.find( key );
            if( iter == mapgens_.end() ) {
                return false;
            }
            return iter->second.generate( dat, hardcoded_weight );
        }

        mapgen_parameters get_map_special_params( const std::string &key ) const {
            const auto iter = mapgens_.find( key );
            if( iter == mapgens_.end() ) {
                return mapgen_parameters();
            }
            return iter->second.get_mapgen_params( mapgen_parameter_scope::overmap_special,
                                                   // NOLINTNEXTLINE(cata-translate-string-literal)
                                                   string_format( "map special %s", key ) );
        }
};

static mapgen_factory oter_mapgen;

std::map<nested_mapgen_id, nested_mapgen> nested_mapgens;
std::map<update_mapgen_id, update_mapgen> update_mapgens;
static std::unordered_map<std::string, tripoint_abs_ms> queued_points;

template<>
bool string_id<nested_mapgen>::is_valid() const
{
    return str() == "null" || nested_mapgens.find( *this ) != nested_mapgens.end();
}

template<>
const nested_mapgen &string_id<nested_mapgen>::obj() const
{
    auto it = nested_mapgens.find( *this );
    if( it == nested_mapgens.end() ) {
        debugmsg( "Using invalid nested_mapgen_id %s", str() );
        static const nested_mapgen null_mapgen;
        return null_mapgen;
    }
    return it->second;
}

template<>
bool string_id<update_mapgen>::is_valid() const
{
    return str() == "null" || update_mapgens.find( *this ) != update_mapgens.end();
}

template<>
const update_mapgen &string_id<update_mapgen>::obj() const
{
    auto it = update_mapgens.find( *this );
    if( it == update_mapgens.end() ) {
        debugmsg( "Using invalid nested_mapgen_id %s", str() );
        static const update_mapgen null_mapgen;
        return null_mapgen;
    }
    return it->second;
}

/*
 * setup mapgen_basic_container::weights_ which mapgen uses to diceroll. Also setup mapgen_function_json
 */
void calculate_mapgen_weights()   // TODO: rename as it runs jsonfunction setup too
{
    oter_mapgen.setup();
    // Not really calculate weights, but let's keep it here for now
    for( auto &pr : nested_mapgens ) {
        for( const weighted_object<int, std::shared_ptr<mapgen_function_json_nested>> &ptr :
             pr.second.funcs() ) {
            ptr.obj->setup();
            inp_mngr.pump_events();
        }
    }
    for( auto &pr : update_mapgens ) {
        for( const auto &ptr : pr.second.funcs() ) {
            ptr->setup();
            inp_mngr.pump_events();
        }
    }
    // Having set up all the mapgens we can now perform a second
    // pass of finalizing their parameters
    oter_mapgen.finalize_parameters();
    for( auto &pr : nested_mapgens ) {
        for( const weighted_object<int, std::shared_ptr<mapgen_function_json_nested>> &ptr :
             pr.second.funcs() ) {
            ptr.obj->finalize_parameters();
            inp_mngr.pump_events();
        }
    }
    for( auto &pr : update_mapgens ) {
        for( const auto &ptr : pr.second.funcs() ) {
            ptr->finalize_parameters();
            inp_mngr.pump_events();
        }
    }
}

void check_mapgen_definitions()
{
    oter_mapgen.check_consistency();
    for( const auto &oter_definition : nested_mapgens ) {
        for( const auto &mapgen_function_ptr : oter_definition.second.funcs() ) {
            mapgen_function_ptr.obj->check();
        }
    }
    for( const auto &oter_definition : update_mapgens ) {
        for( const auto &mapgen_function_ptr : oter_definition.second.funcs() ) {
            mapgen_function_ptr->check();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
///// json mapgen functions
///// 1 - init():

/**
 * Tiny little namespace to hold error messages
 */
namespace mapgen_defer
{
static std::string member;
static std::string message;
static bool defer;
static JsonObject jsi;
} // namespace mapgen_defer

static void set_mapgen_defer( const JsonObject &jsi, const std::string &member,
                              const std::string &message )
{
    mapgen_defer::defer = true;
    mapgen_defer::jsi = jsi;
    mapgen_defer::member = member;
    mapgen_defer::message = message;
}

/*
 * load a single mapgen json structure; this can be inside an overmap_terrain, or on it's own.
 */
std::shared_ptr<mapgen_function>
load_mapgen_function( const JsonObject &jio, const std::string &id_base,
                      const point_rel_omt &offset,
                      const point_rel_omt &total )
{
    dbl_or_var weight = get_dbl_or_var( jio, "weight", false,  1000 );
    if( weight.min.is_constant() && ( weight.min.constant() < 0 ||
                                      weight.min.constant() >= INT_MAX ) ) {
        jio.throw_error_at( "weight", "min value out of bounds (0 - max int)" );
    }
    if( weight.pair && weight.max.is_constant() && ( weight.max.constant() < 0 ||
            weight.max.constant() >= INT_MAX ) ) {
        jio.throw_error_at( "weight", "max value out of bounds (0 - max int)" );
    }
    if( jio.has_string( "builtin" ) ) {
        if( const building_gen_pointer ptr = get_mapgen_cfunction( jio.get_string( "builtin" ) ) ) {
            return std::make_shared<mapgen_function_builtin>( ptr, std::move( weight ) );
        } else {
            jio.throw_error_at( "builtin", "function does not exist" );
        }
    } else if( !jio.has_object( "object" ) ) {
        jio.throw_error( R"(mapgen must define key "object" or "builtin")" );
    }
    JsonObject jo = jio.get_object( "object" );
    return std::make_shared<mapgen_function_json>( std::move( jo ), std::move( weight ),
            "mapgen " + id_base, offset,
            total );
}

void load_and_add_mapgen_function( const JsonObject &jio, const std::string &id_base,
                                   const point_rel_omt &offset, const point_rel_omt &total )
{
    std::shared_ptr<mapgen_function> f = load_mapgen_function( jio, id_base, offset, total );
    if( f ) {
        oter_mapgen.add( id_base, f );
    }
}

static void load_nested_mapgen( JsonObject &&jio, const nested_mapgen_id &id_base )
{
    if( jio.has_object( "object" ) ) {
        int weight = jio.get_int( "weight", 1000 );
        JsonObject jo = jio.get_object( "object" );
        nested_mapgens[id_base].add(
            std::make_shared<mapgen_function_json_nested>(
                std::move( jo ), "nested mapgen " + id_base.str() ),
            weight );
    } else {
        debugmsg( "Nested mapgen: Invalid mapgen function (missing \"object\" object)", id_base.c_str() );
    }
}

static void load_update_mapgen( JsonObject &&jio, const update_mapgen_id &id_base )
{
    if( jio.has_object( "object" ) ) {
        JsonObject jo = jio.get_object( "object" );
        update_mapgens[id_base].add(
            std::make_unique<update_mapgen_function_json>(
                std::move( jo ), "update mapgen " + id_base.str() ) );
    } else {
        debugmsg( "Update mapgen: Invalid mapgen function (missing \"object\" object)",
                  id_base.c_str() );
    }
}

/*
 * feed bits `o json from standalone file to load_mapgen_function. (standalone json "type": "mapgen")
 */
void load_mapgen( const JsonObject &jo )
{
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    static constexpr point_rel_omt point_one( 1, 1 );

    if( jo.has_array( "om_terrain" ) ) {
        JsonArray ja = jo.get_array( "om_terrain" );
        if( ja.test_array() ) {
            point_rel_omt offset;
            point_rel_omt total( ja.get_array( 0 ).size(), ja.size() );
            for( JsonArray row_items : ja ) {
                for( const std::string mapgenid : row_items ) {
                    load_and_add_mapgen_function( jo, mapgenid, offset, total );
                    offset.x()++;
                }
                offset.y()++;
                offset.x() = 0;
            }
        } else {
            std::vector<std::string> mapgenid_list;
            for( const std::string line : ja ) {
                mapgenid_list.push_back( line );
            }
            if( !mapgenid_list.empty() ) {
                const std::string mapgenid = mapgenid_list[0];
                const auto mgfunc = load_mapgen_function( jo, mapgenid, point_rel_omt::zero, point_one );
                if( mgfunc ) {
                    for( auto &i : mapgenid_list ) {
                        oter_mapgen.add( i, mgfunc );
                    }
                }
            }
        }
    } else if( jo.has_string( "om_terrain" ) ) {
        load_and_add_mapgen_function( jo, jo.get_string( "om_terrain" ), point_rel_omt::zero, point_one );
    } else if( jo.has_string( "nested_mapgen_id" ) ) {
        std::string id_base = jo.get_string( "nested_mapgen_id" );
        JsonObject jo_copy = jo;
        jo_copy.copy_visited_members( jo );
        jo.allow_omitted_members();
        load_nested_mapgen( std::move( jo_copy ), nested_mapgen_id( id_base ) );
    } else if( jo.has_string( "update_mapgen_id" ) ) {
        std::string id_base = jo.get_string( "update_mapgen_id" );
        JsonObject jo_copy = jo;
        jo_copy.copy_visited_members( jo );
        jo.allow_omitted_members();
        load_update_mapgen( std::move( jo_copy ), update_mapgen_id( id_base ) );
    } else {
        debugmsg( "mapgen entry requires \"om_terrain\" or \"nested_mapgen_id\"(string, array of strings, or array of array of strings)\n%s\n",
                  jo.str() );
    }
}

void reset_mapgens()
{
    oter_mapgen.reset();
    nested_mapgens.clear();
    update_mapgens.clear();
}

/////////////////////////////////////////////////////////////////////////////////
///// 2 - right after init() finishes parsing all game json and terrain info/etc is set..
/////   ...parse more json! (mapgen_function_json)

size_t mapgen_function_json_base::calc_index( const point_rel_ms &p ) const
{
    if( p.x() >= mapgensize.x() ) {
        debugmsg( "invalid value %zu for x in calc_index", p.x() );
    }
    if( p.y() >= mapgensize.y() ) {
        debugmsg( "invalid value %zu for y in calc_index", p.y() );
    }
    return p.y() * mapgensize.y() + p.x();
}

static bool common_check_bounds( const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                                 const point_rel_ms &mapgensize, const JsonObject &jso )
{
    half_open_rectangle<point_rel_ms> bounds( point_rel_ms::zero, mapgensize );
    if( !bounds.contains( point_rel_ms( x.val, y.val ) ) ) {
        return false;
    }

    if( x.valmax < x.val ) {
        jso.throw_error( "x maximum is less than x minimum" );
    }

    if( y.valmax < y.val ) {
        jso.throw_error( "y maximum is less than y minimum" );
    }

    if( z.valmax != z.val ) {
        jso.throw_error( "z maximum has to be identical to z minimum" );
    }

    if( x.valmax > mapgensize.x() - 1 ) {
        jso.throw_error_at( "x", "coordinate range cannot cross grid boundaries" );
    }

    if( y.valmax > mapgensize.y() - 1 ) {
        jso.throw_error_at( "y", "coordinate range cannot cross grid boundaries" );
    }

    return true;
}

void mapgen_function_json_base::merge_non_nest_parameters_into(
    mapgen_parameters &params, const std::string &outer_context ) const
{
    // NOLINTNEXTLINE(cata-translate-string-literal)
    const std::string context = string_format( "%s within %s", context_, outer_context );
    params.check_and_merge( parameters, context, mapgen_parameter_scope::nest );
}

bool mapgen_function_json_base::check_inbounds( const jmapgen_int &x, const jmapgen_int &y,
        const jmapgen_int &z,
        const JsonObject &jso ) const
{
    return common_check_bounds( x, y, z, mapgensize, jso );
}

mapgen_function_json_base::mapgen_function_json_base(
    JsonObject &&jsobj, const std::string &context )
    : jsobj( std::move( jsobj ) )
    , context_( context )
    , is_ready( false )
    , mapgensize( SEEX * 2, SEEY * 2 )
    , total_size( mapgensize )
    , objects( m_offset, mapgensize, total_size )
{
}

mapgen_function_json_base::~mapgen_function_json_base()
{
    jsobj.allow_omitted_members();
}

mapgen_function_json::mapgen_function_json( JsonObject &&jsobj,
        dbl_or_var w, const std::string &context, const point_rel_omt &grid_offset,
        const point_rel_omt &grid_total )
    : mapgen_function( std::move( w ) )
    , mapgen_function_json_base( std::move( jsobj ), context )
    , rotation( 0 )
    , fallback_predecessor_mapgen_( oter_str_id::NULL_ID() )
{
    m_offset.x() = grid_offset.x() * mapgensize.x();
    m_offset.y() = grid_offset.y() * mapgensize.y();
    m_offset.z() = 0;
    total_size.x() = grid_total.x() * mapgensize.x();
    total_size.y() = grid_total.y() * mapgensize.y();
    objects = jmapgen_objects( m_offset, mapgensize, total_size );
}

mapgen_function_json_nested::mapgen_function_json_nested(
    JsonObject &&jsobj, const std::string &context )
    : mapgen_function_json_base( std::move( jsobj ), context )
    , rotation( 0 )
{
}

jmapgen_int::jmapgen_int( const JsonObject &jo, std::string_view tag )
{
    if( jo.has_array( tag ) ) {
        JsonArray sparray = jo.get_array( tag );
        if( sparray.empty() || sparray.size() > 2 ) {
            jo.throw_error_at( tag, "invalid data: must be an array of 1 or 2 values" );
        }
        int tmpval = sparray.get_int( 0 );
        if( tmpval >= std::numeric_limits<int16_t>::max() ) {
            jo.throw_error_at( tag, string_format( "Value %d too large", tmpval ) );
        }
        val = tmpval;
        if( sparray.size() == 2 ) {
            int tmpvalmax = sparray.get_int( 1 );
            if( tmpvalmax >= std::numeric_limits<int16_t>::max() ) {
                jo.throw_error_at( tag, string_format( "Value %d too large", tmpvalmax ) );
            }
            valmax = tmpvalmax;
        } else {
            valmax = val;
        }
    } else {
        val = valmax = jo.get_int( tag );
    }
}

jmapgen_int::jmapgen_int( const JsonObject &jo, std::string_view tag, const int &def_val,
                          const int &def_valmax )
    : val( def_val )
    , valmax( def_valmax )
{
    if( jo.has_array( tag ) ) {
        JsonArray sparray = jo.get_array( tag );
        if( sparray.size() > 2 ) {
            jo.throw_error_at( tag, "invalid data: must be an array of 1 or 2 values" );
        }
        if( !sparray.empty() ) {
            int tmpval = sparray.get_int( 0 );
            if( tmpval >= std::numeric_limits<int16_t>::max() ) {
                jo.throw_error_at( tag, string_format( "Value %d too large", tmpval ) );
            }
            val = tmpval;
        }
        if( sparray.size() >= 2 ) {
            int tmpvalmax = sparray.get_int( 1 );
            if( tmpvalmax >= std::numeric_limits<int16_t>::max() ) {
                jo.throw_error_at( tag, string_format( "Value %d too large", tmpvalmax ) );
            }
            valmax = tmpvalmax;
        }
    } else if( jo.has_member( tag ) ) {
        int tmpval = jo.get_int( tag );
        if( tmpval >= std::numeric_limits<int16_t>::max() ) {
            jo.throw_error_at( tag, string_format( "Value %d too large", tmpval ) );
        }
        val = valmax = tmpval;
    }
}

int jmapgen_int::get() const
{
    return val == valmax ? val : rng( val, valmax );
}

/*
 * Turn json gobbldigook into machine friendly gobbldigook, for applying
 * basic map 'set' functions, optionally based on one_in(chance) or repeat value
 */
void mapgen_function_json_base::setup_setmap( const JsonArray &parray )
{
    std::string tmpval;
    std::map<std::string, jmapgen_setmap_op> setmap_opmap;
    setmap_opmap[ "terrain" ] = JMAPGEN_SETMAP_TER;
    setmap_opmap[ "furniture" ] = JMAPGEN_SETMAP_FURN;
    setmap_opmap[ "trap" ] = JMAPGEN_SETMAP_TRAP;
    setmap_opmap[ "trap_remove" ] = JMAPGEN_SETMAP_TRAP_REMOVE;
    setmap_opmap[ "creature_remove" ] = JMAPGEN_SETMAP_CREATURE_REMOVE;
    setmap_opmap[ "item_remove" ] = JMAPGEN_SETMAP_ITEM_REMOVE;
    setmap_opmap[ "field_remove" ] = JMAPGEN_SETMAP_FIELD_REMOVE;
    setmap_opmap[ "radiation" ] = JMAPGEN_SETMAP_RADIATION;
    setmap_opmap[ "bash" ] = JMAPGEN_SETMAP_BASH;
    setmap_opmap[ "burn" ] = JMAPGEN_SETMAP_BURN;
    setmap_opmap[ "variable" ] = JMAPGEN_SETMAP_VARIABLE;
    std::map<std::string, jmapgen_setmap_op>::iterator sm_it;
    jmapgen_setmap_op tmpop;
    int setmap_optype = 0;

    for( const JsonObject pjo : parray ) {
        if( pjo.read( "point", tmpval ) ) {
            setmap_optype = JMAPGEN_SETMAP_OPTYPE_POINT;
        } else if( pjo.read( "set", tmpval ) ) {
            setmap_optype = JMAPGEN_SETMAP_OPTYPE_POINT;
            debugmsg( "Warning, set: [ { \"set\": … } is deprecated, use set: [ { \"point\": … " );
        } else if( pjo.read( "line", tmpval ) ) {
            setmap_optype = JMAPGEN_SETMAP_OPTYPE_LINE;
        } else if( pjo.read( "square", tmpval ) ) {
            setmap_optype = JMAPGEN_SETMAP_OPTYPE_SQUARE;
        } else {
            pjo.throw_error( R"(invalid data: must contain "point", "set", "line" or "square" member)" );
        }

        sm_it = setmap_opmap.find( tmpval );
        if( sm_it == setmap_opmap.end() ) {
            pjo.throw_error( string_format( "invalid subfunction %s", tmpval.c_str() ) );
        }

        tmpop = sm_it->second;
        jmapgen_int tmp_x2( 0, 0 );
        jmapgen_int tmp_y2( 0, 0 );
        jmapgen_int tmp_i( 0, 0 );
        std::string string_val;
        int tmp_chance = 1;

        const jmapgen_int tmp_x( pjo, "x" );
        const jmapgen_int tmp_y( pjo, "y" );
        const jmapgen_int tmp_z( pjo, "z", 0, 0 );
        if( !check_inbounds( tmp_x, tmp_y, tmp_z, pjo ) ) {
            pjo.allow_omitted_members();
            continue;
        }
        if( setmap_optype != JMAPGEN_SETMAP_OPTYPE_POINT ) {
            tmp_x2 = jmapgen_int( pjo, "x2" );
            tmp_y2 = jmapgen_int( pjo, "y2" );
            if( !check_inbounds( tmp_x2, tmp_y2, tmp_z, pjo ) ) {
                pjo.allow_omitted_members();
                continue;
            }
        }
        if( tmpop == JMAPGEN_SETMAP_RADIATION ) {
            tmp_i = jmapgen_int( pjo, "amount" );
        } else if( tmpop == JMAPGEN_SETMAP_BASH || tmpop == JMAPGEN_SETMAP_BURN ||
                   tmpop == JMAPGEN_SETMAP_ITEM_REMOVE || tmpop == JMAPGEN_SETMAP_FIELD_REMOVE ||
                   tmpop == JMAPGEN_SETMAP_CREATURE_REMOVE ) {
            //suppress warning
        } else if( tmpop == JMAPGEN_SETMAP_VARIABLE ) {
            string_val = pjo.get_string( "id" );
        } else {
            std::string tmpid = pjo.get_string( "id" );
            switch( tmpop ) {
                case JMAPGEN_SETMAP_TER: {
                    const ter_str_id tid( tmpid );

                    if( !tid.is_valid() ) {
                        set_mapgen_defer( pjo, "id", "no such terrain" );
                        return;
                    }
                    tmp_i.val = tid.id().to_i();
                }
                break;
                case JMAPGEN_SETMAP_FURN: {
                    const furn_str_id fid( tmpid );

                    if( !fid.is_valid() ) {
                        set_mapgen_defer( pjo, "id", "no such furniture" );
                        return;
                    }
                    tmp_i.val = fid.id().to_i();
                }
                break;
                case JMAPGEN_SETMAP_TRAP_REMOVE:
                case JMAPGEN_SETMAP_TRAP: {
                    const trap_str_id sid( tmpid );
                    if( !sid.is_valid() ) {
                        set_mapgen_defer( pjo, "id", "no such trap" );
                        return;
                    }
                    tmp_i.val = sid.id().to_i();
                }
                break;

                default:
                    //Suppress warnings
                    break;
            }
            // TODO: ... support for random furniture? or not.
            tmp_i.valmax = tmp_i.val;
        }
        // TODO: sanity check?
        const jmapgen_int tmp_repeat = jmapgen_int( pjo, "repeat", 1, 1 );
        pjo.read( "chance", tmp_chance );
        jmapgen_setmap tmp( tmp_x, tmp_y, tmp_z, tmp_x2, tmp_y2,
                            static_cast<jmapgen_setmap_op>( tmpop + setmap_optype ), tmp_i,
                            tmp_chance, tmp_repeat, string_val );

        setmap_points.push_back( tmp );
        tmpval.clear();
    }

}

void mapgen_function_json_base::finalize_parameters_common()
{
    objects.merge_parameters_into( parameters, context_ );
}

mapgen_arguments mapgen_function_json_base::get_args(
    const mapgendata &md, mapgen_parameter_scope scope ) const
{
    return parameters.get_args( md, scope );
}

jmapgen_place::jmapgen_place( const JsonObject &jsi )
    : x( jsi, "x" )
    , y( jsi, "y" )
    , z( jsi, "z", 0, 0 )
    , repeat( jsi, "repeat", 1, 1 )
{
}

void jmapgen_place::offset( const tripoint_rel_ms &offset )
{
    x.val -= offset.x();
    x.valmax -= offset.x();
    y.val -= offset.y();
    y.valmax -= offset.y();
    z.val -= offset.z();
    z.valmax -= offset.z();
}

map_key::map_key( const std::string &s ) : str( s )
{
    if( utf8_width( str ) != 1 ) {
        debugmsg( "map key '%s' must be 1 column", str );
    }
}

map_key::map_key( const JsonMember &member ) : str( member.name() )
{
    if( utf8_width( str ) != 1 ) {
        member.throw_error( "format map key must be 1 column" );
    }
}

template<typename T>
static bool is_null_helper( const string_id<T> &id )
{
    return id.is_null();
}

template<typename T>
static bool is_null_helper( const int_id<T> &id )
{
    return id.id().is_null();
}

static bool is_null_helper( std::string_view )
{
    return false;
}

template<typename T>
struct make_null_helper;

template<>
struct make_null_helper<std::string> {
    std::string operator()() const {
        return {};
    }
};

template<typename T>
struct make_null_helper<string_id<T>> {
    string_id<T> operator()() const {
        return string_id<T>::NULL_ID();
    }
};

template<typename T>
struct make_null_helper<int_id<T>> {
    int_id<T> operator()() const {
        return string_id<T>::NULL_ID().id();
    }
};

template<typename T>
static string_id<T> to_string_id_helper( const string_id<T> &id )
{
    return id;
}

template<typename T>
static string_id<T> to_string_id_helper( const int_id<T> &id )
{
    return id.id();
}

static std::string to_string_id_helper( const std::string &s )
{
    return s;
}

template<typename T>
static bool is_valid_helper( const string_id<T> &id )
{
    return id.is_valid();
}

template<typename T>
static bool is_valid_helper( const int_id<T> & )
{
    return true;
}

static bool is_valid_helper( std::string_view )
{
    return true;
}

// Mapgen often uses various id values.  Usually these are specified verbatim
// as strings, but they can also be parameterized.  This class encapsulates
// such a value.  It records how the value was specified so that it can be
// calculated later based on the parameters chosen for a particular instance of
// the mapgen.
template<typename Id>
class mapgen_value
{
    public:
        using StringId = to_string_id_t<Id>;
        struct void_;

        using Id_unless_string =
            std::conditional_t<std::is_same_v<Id, std::string>, void_, Id>;

        struct value_source {
            virtual ~value_source() = default;
            virtual Id get( const mapgendata & ) const = 0;
            virtual void check( const std::string &/*oter_name*/, const mapgen_parameters &
                              ) const {};
            virtual void check_consistent_with(
                const value_source &, const std::string &context ) const = 0;
            virtual std::vector<StringId> all_possible_results(
                const mapgen_parameters & ) const = 0;
            virtual const std::string *get_name_if_parameter() const {
                return nullptr;
            }
        };

        struct null_source : value_source {
            Id get( const mapgendata & ) const override {
                return make_null_helper<Id> {}();
            }

            void check_consistent_with(
                const value_source &o, const std::string &context ) const override {
                if( const null_source *other = dynamic_cast<const null_source *>( &o ) ) {
                    // OK
                } else {
                    debugmsg( "inconsistent default types for %s", context );
                }
            }

            std::vector<StringId> all_possible_results( const mapgen_parameters & ) const override {
                return { make_null_helper<StringId>{}() };
            }
        };

        struct id_source : value_source {
            Id id;

            explicit id_source( const std::string &s ) :
                id( s ) {
            }

            explicit id_source( const Id_unless_string &s ) :
                id( s ) {
            }

            Id get( const mapgendata & ) const override {
                return id;
            }

            void check( const std::string &context, const mapgen_parameters & ) const override {
                if( !is_valid_helper( id ) ) {
                    debugmsg( "mapgen '%s' uses invalid entry '%s'",
                              context, cata_variant( id ).get_string() );
                }
            }

            void check_consistent_with(
                const value_source &o, const std::string &context ) const override {
                if( const id_source *other = dynamic_cast<const id_source *>( &o ) ) {
                    if( id != other->id ) {
                        debugmsg( "inconsistent default values for %s (%s vs %s)",
                                  context, cata_variant( id ).get_string(),
                                  cata_variant( other->id ).get_string() );
                    }
                } else {
                    debugmsg( "inconsistent default types for %s", context );
                }
            }

            std::vector<StringId> all_possible_results( const mapgen_parameters & ) const override {
                return { to_string_id_helper( id ) };
            }
        };

        struct param_source : value_source {
            std::string param_name;
            std::optional<StringId> fallback;

            explicit param_source( const JsonObject &jo )
                : param_name( jo.get_string( "param" ) ) {
                jo.read( "fallback", fallback, false );
            }

            Id get( const mapgendata &dat ) const override {
                if( fallback ) {
                    return Id( dat.get_arg_or<StringId>( param_name, *fallback ) );
                } else {
                    return Id( dat.get_arg<StringId>( param_name ) );
                }
            }

            void check( const std::string &context, const mapgen_parameters &parameters
                      ) const override {
                auto param_it = parameters.map.find( param_name );
                if( param_it == parameters.map.end() ) {
                    debugmsg( "mapgen '%s' uses undefined parameter '%s'", context, param_name );
                } else {
                    const mapgen_parameter &param = param_it->second;
                    constexpr cata_variant_type req_type = cata_variant_type_for<StringId>();
                    cata_variant_type param_type = param.type();
                    if( param_type != req_type && req_type != cata_variant_type::string ) {
                        debugmsg( "mapgen '%s' uses parameter '%s' of type '%s' in a context "
                                  "expecting type '%s'", context, param_name,
                                  io::enum_to_string( param_type ),
                                  io::enum_to_string( req_type ) );
                    }
                    if( param.scope() == mapgen_parameter_scope::overmap_special && !fallback ) {
                        debugmsg( "mapgen '%s' uses parameter '%s' of map_special scope without a "
                                  "fallback.  Such parameters must provide a fallback to allow "
                                  "for changes to overmap_special definitions", context,
                                  param_name );
                    }
                }
            }

            void check_consistent_with(
                const value_source &o, const std::string &context ) const override {
                if( const param_source *other = dynamic_cast<const param_source *>( &o ) ) {
                    if( param_name != other->param_name ) {
                        debugmsg( "inconsistent default values for %s (%s vs %s)",
                                  context, param_name, other->param_name );
                    }
                } else {
                    debugmsg( "inconsistent default types for %s", context );
                }
            }

            std::vector<StringId> all_possible_results(
                const mapgen_parameters &params ) const override {
                auto param_it = params.map.find( param_name );
                if( param_it == params.map.end() ) {
                    return {};
                } else {
                    const mapgen_parameter &param = param_it->second;
                    std::vector<StringId> result;
                    for( const std::string &s : param.all_possible_values( params ) ) {
                        result.emplace_back( s );
                    }
                    return result;
                }
            }

            const std::string *get_name_if_parameter() const override {
                return &param_name;
            }
        };

        struct distribution_source : value_source {
            weighted_int_list<StringId> list;

            explicit distribution_source( const JsonObject &jo ) {
                load_weighted_list( jo.get_member( "distribution" ), list, 1 );
            }

            Id get( const mapgendata & ) const override {
                return *list.pick();
            }

            void check( const std::string &context, const mapgen_parameters & ) const override {
                for( const weighted_object<int, StringId> &wo : list ) {
                    if( !is_valid_helper( wo.obj ) ) {
                        debugmsg( "mapgen '%s' uses invalid entry '%s' in weighted list",
                                  context, cata_variant( wo.obj ).get_string() );
                    }
                }
            }

            void check_consistent_with(
                const value_source &o, const std::string &context ) const override {
                if( const distribution_source *other =
                        dynamic_cast<const distribution_source *>( &o ) ) {
                    if( list != other->list ) {
                        const std::string my_list = list.to_debug_string();
                        const std::string other_list = other->list.to_debug_string();
                        debugmsg( "inconsistent default value distributions for %s (%s vs %s)",
                                  context, my_list, other_list );
                    }
                } else {
                    debugmsg( "inconsistent default types for %s", context );
                }
            }

            std::vector<StringId> all_possible_results( const mapgen_parameters & ) const override {
                std::vector<StringId> result;
                for( const weighted_object<int, StringId> &wo : list ) {
                    result.push_back( wo.obj );
                }
                return result;
            }
        };

        struct switch_source : value_source {
            // This has to be a pointer because mapgen_value is an incomplete
            // type.  We could resolve this by pulling out all these
            // value_source classes and defining them at namespace scope after
            // mapgen_value, but that would make the code much more verbose.
            std::unique_ptr<mapgen_value<std::string>> on;
            std::unordered_map<std::string, StringId> cases;

            explicit switch_source( const JsonObject &jo )
                : on( std::make_unique<mapgen_value<std::string>>( jo.get_object( "switch" ) ) ) {
                jo.read( "cases", cases, true );
            }

            Id get( const mapgendata &dat ) const override {
                std::string based_on = on->get( dat );
                auto it = cases.find( based_on );
                if( it == cases.end() ) {
                    debugmsg( "switch does not handle case %s", based_on );
                    return make_null_helper<Id> {}();
                }
                return Id( it->second );
            }

            void check( const std::string &context, const mapgen_parameters &params
                      ) const override {
                on->check( context, params );
                for( const std::pair<const std::string, StringId> &p : cases ) {
                    if( !is_valid_helper( p.second ) ) {
                        debugmsg( "mapgen '%s' uses invalid entry '%s' in switch",
                                  context, cata_variant( p.second ).get_string() );
                    }
                }
                std::vector<std::string> possible_values = on->all_possible_results( params );
                for( const std::string &value : possible_values ) {
                    if( !cases.count( value ) ) {
                        debugmsg( "mapgen '%s' has switch which does not account for potential "
                                  "case '%s' of the switched-on value", context, value );
                    }
                }
            }

            void check_consistent_with(
                const value_source &o, const std::string &context ) const override {
                if( const switch_source *other = dynamic_cast<const switch_source *>( &o ) ) {
                    on->check_consistent_with( *other->on, context );
                    if( cases != other->cases ) {
                        auto dump_set = []( const std::unordered_map<std::string, StringId> &s ) {
                            bool first = true;
                            std::string result = "{ ";
                            for( const std::pair<const std::string, StringId> &p : s ) {
                                if( first ) {
                                    first = false;
                                } else {
                                    result += ", ";
                                }
                                result += p.first;
                                result += ": ";
                                result += cata_variant( p.second ).get_string();
                            }
                            return result;
                        };

                        const std::string my_list = dump_set( cases );
                        const std::string other_list = dump_set( other->cases );
                        debugmsg( "inconsistent switch cases for %s (%s vs %s)",
                                  context, my_list, other_list );
                    }
                } else {
                    debugmsg( "inconsistent default types for %s", context );
                }
            }

            std::vector<StringId> all_possible_results( const mapgen_parameters & ) const override {
                std::vector<StringId> result;
                result.reserve( cases.size() );
                for( const std::pair<const std::string, StringId> &p : cases ) {
                    result.push_back( p.second );
                }
                return result;
            }
        };

        mapgen_value()
            : is_null_( true )
            , source_( make_shared_fast<null_source>() )
        {}

        explicit mapgen_value( const std::string &s ) {
            init_string( s );
        }

        explicit mapgen_value( const Id_unless_string &id ) {
            init_string( id );
        }

        explicit mapgen_value( const JsonValue &jv ) {
            if( jv.test_string() ) {
                init_string( jv.get_string() );
            } else {
                init_object( jv.get_object() );
            }
        }

        explicit mapgen_value( const JsonObject &jo ) {
            init_object( jo );
        }

        template<typename S>
        void init_string( const S &s ) {
            source_ = make_shared_fast<id_source>( s );
            is_null_ = is_null_helper( s );
        }

        void init_object( const JsonObject &jo ) {
            if( jo.has_member( "param" ) ) {
                source_ = make_shared_fast<param_source>( jo );
            } else if( jo.has_member( "distribution" ) ) {
                source_ = make_shared_fast<distribution_source>( jo );
            } else if( jo.has_member( "switch" ) ) {
                source_ = make_shared_fast<switch_source>( jo );
            } else {
                jo.throw_error(
                    R"(Expected member "param", "distribution", or "switch" in mapgen object)" );
            }
        }

        bool is_null() const {
            return is_null_;
        }

        void check( const std::string &context, const mapgen_parameters &params ) const {
            source_->check( context, params );
        }
        void check_consistent_with( const mapgen_value &other, const std::string &context ) const {
            source_->check_consistent_with( *other.source_, context );
        }

        Id get( const mapgendata &dat ) const {
            return source_->get( dat );
        }
        std::vector<StringId> all_possible_results( const mapgen_parameters &params ) const {
            return source_->all_possible_results( params );
        }

        const std::string *get_name_if_parameter() const {
            return source_->get_name_if_parameter();
        }

        void deserialize( const JsonValue &jsin ) {
            if( jsin.test_object() ) {
                *this = mapgen_value( jsin.get_object() );
            } else {
                *this = mapgen_value( jsin.get_string() );
            }
        }
    private:
        bool is_null_ = false;
        shared_ptr_fast<const value_source> source_;
};

namespace io
{

template<>
std::string enum_to_string<jmapgen_flags>( jmapgen_flags v )
{
    switch( v ) {
        // *INDENT-OFF*
        case jmapgen_flags::allow_terrain_under_other_data: return "ALLOW_TERRAIN_UNDER_OTHER_DATA";
        case jmapgen_flags::dismantle_all_before_placing_terrain:
            return "DISMANTLE_ALL_BEFORE_PLACING_TERRAIN";
        case jmapgen_flags::erase_all_before_placing_terrain:
            return "ERASE_ALL_BEFORE_PLACING_TERRAIN";
        case jmapgen_flags::allow_terrain_under_furniture: return "ALLOW_TERRAIN_UNDER_FURNITURE";
        case jmapgen_flags::dismantle_furniture_before_placing_terrain:
            return "DISMANTLE_FURNITURE_BEFORE_PLACING_TERRAIN";
        case jmapgen_flags::erase_furniture_before_placing_terrain:
            return "ERASE_FURNITURE_BEFORE_PLACING_TERRAIN";
        case jmapgen_flags::allow_terrain_under_trap: return "ALLOW_TERRAIN_UNDER_TRAP";
        case jmapgen_flags::dismantle_trap_before_placing_terrain:
            return "DISMANTLE_TRAP_BEFORE_PLACING_TERRAIN";
        case jmapgen_flags::erase_trap_before_placing_terrain:
            return "ERASE_TRAP_BEFORE_PLACING_TERRAIN";
        case jmapgen_flags::allow_terrain_under_items: return "ALLOW_TERRAIN_UNDER_ITEMS";
        case jmapgen_flags::erase_items_before_placing_terrain:
            return "ERASE_ITEMS_BEFORE_PLACING_TERRAIN";
        case jmapgen_flags::no_underlying_rotate: return "NO_UNDERLYING_ROTATE";
        case jmapgen_flags::avoid_creatures: return "AVOID_CREATURES";
        // *INDENT-ON*
        case jmapgen_flags::last:
            break;
    }
    debugmsg( "unknown jmapgen_flags %d", static_cast<int>( v ) );
    return "";
}

template<>
std::string enum_to_string<mapgen_parameter_scope>( mapgen_parameter_scope v )
{
    switch( v ) {
        // *INDENT-OFF*
        case mapgen_parameter_scope::overmap_special: return "overmap_special";
        case mapgen_parameter_scope::omt_stack: return "omt_stack";
        case mapgen_parameter_scope::omt: return "omt";
        case mapgen_parameter_scope::nest: return "nest";
        // *INDENT-ON*
        case mapgen_parameter_scope::last:
            break;
    }
    debugmsg( "unknown mapgen_parameter_scope %d", static_cast<int>( v ) );
    return "";
}

} // namespace io

mapgen_parameter::mapgen_parameter() = default;

mapgen_parameter::mapgen_parameter( const mapgen_value<std::string> &def, cata_variant_type type,
                                    mapgen_parameter_scope scope )
    : scope_( scope )
    , type_( type )
    , default_( make_shared_fast<mapgen_value<std::string>>( def ) )
{}

void mapgen_parameter::deserialize( const JsonObject &jo )
{
    optional( jo, false, "scope", scope_, mapgen_parameter_scope::overmap_special );
    jo.read( "type", type_, true );
    default_ = make_shared_fast<mapgen_value<std::string>>( jo.get_member( "default" ) );
}

cata_variant_type mapgen_parameter::type() const
{
    return type_;
}

cata_variant mapgen_parameter::get( const mapgendata &md ) const
{
    return cata_variant::from_string( type_, default_->get( md ) );
}

std::vector<std::string> mapgen_parameter::all_possible_values(
    const mapgen_parameters &params ) const
{
    return default_->all_possible_results( params );
}

void mapgen_parameter::check( const mapgen_parameters &params, const std::string &context ) const
{
    default_->check( context, params );
    for( const std::string &value : all_possible_values( params ) ) {
        if( !cata_variant::from_string( type_, std::string( value ) ).is_valid() ) {
            debugmsg( "%s can take value %s which is not a valid value of type %s",
                      context, value, io::enum_to_string( type_ ) );
        }
    }
}

void mapgen_parameter::check_consistent_with(
    const mapgen_parameter &other, const std::string &context ) const
{
    if( scope_ != other.scope_ ) {
        debugmsg( "mismatched scope for mapgen parameters %s (%s vs %s)",
                  context, io::enum_to_string( scope_ ), io::enum_to_string( other.scope_ ) );
    }
    if( type_ != other.type_ ) {
        debugmsg( "mismatched type for mapgen parameters %s (%s vs %s)",
                  context, io::enum_to_string( type_ ), io::enum_to_string( other.type_ ) );
    }
    default_->check_consistent_with( *other.default_, context );
}

auto mapgen_parameters::add_unique_parameter(
    const std::string &prefix, const mapgen_value<std::string> &def, cata_variant_type type,
    mapgen_parameter_scope scope ) -> iterator
{
    uint64_t i = 0;
    std::string candidate_name;
    while( true ) {
        candidate_name = string_format( "%s%d", prefix, i );
        if( map.find( candidate_name ) == map.end() ) {
            break;
        }
        ++i;
    }

    return map.emplace( candidate_name, mapgen_parameter( def, type, scope ) ).first;
}

mapgen_parameters mapgen_parameters::params_for_scope( mapgen_parameter_scope scope ) const
{
    mapgen_parameters result;
    for( const std::pair<const std::string, mapgen_parameter> &p : map ) {
        const mapgen_parameter &param = p.second;
        if( param.scope() == scope ) {
            result.map.insert( p );
        }
    }
    return result;
}

mapgen_arguments mapgen_parameters::get_args(
    const mapgendata &md, mapgen_parameter_scope scope ) const
{
    std::unordered_map<std::string, cata_variant> result;
    for( const std::pair<const std::string, mapgen_parameter> &p : map ) {
        const std::string &param_name = p.first;
        const mapgen_parameter &param = p.second;
        if( param.scope() == scope ) {
            cata_variant value = md.get_arg_or( param_name, param.get( md ) );
            result.emplace( param_name, value );
        }
        if( param.scope() == mapgen_parameter_scope::omt_stack && scope == mapgen_parameter_scope::omt ) {
            const point_abs_omt &p_scope = md.pos().xy();
            overmap *om = overmap_buffer.get_existing_om_global( p_scope ).om;
            std::optional<cata_variant> v = om->get_existing_omt_stack_argument( p_scope, param_name );
            if( !!v ) {
                result.emplace( param_name, *v );
            } else {
                cata_variant value = md.get_arg_or( param_name, param.get( md ) );
                om->add_omt_stack_argument( p_scope, param_name, value );
                result.emplace( param_name, value );
            }
        }
    }
    return mapgen_arguments{ result };
}

void mapgen_parameters::check_and_merge( const mapgen_parameters &other,
        const std::string &context, mapgen_parameter_scope up_to_scope )
{
    for( const std::pair<const std::string, mapgen_parameter> &p : other.map ) {
        const mapgen_parameter &other_param = p.second;
        if( other_param.scope() >= up_to_scope ) {
            continue;
        }
        auto insert_result = map.insert( p );
        if( !insert_result.second ) {
            const std::string &name = p.first;
            const mapgen_parameter &this_param = insert_result.first->second;
            this_param.check_consistent_with(
                // NOLINTNEXTLINE(cata-translate-string-literal)
                other_param, string_format( "parameter %s in %s", name, context ) );
        }
    }
}

ret_val<void> jmapgen_piece_with_has_vehicle_collision::has_vehicle_collision(
    const mapgendata &dat,
    const tripoint_rel_ms &p ) const
{
    const optional_vpart_position &vp = dat.m.veh_at( tripoint_bub_ms( p.x(), p.y(),
                                        dat.zlevel() + p.z() ) );
    if( vp.has_value() ) {
        return ret_val<void>::make_failure( vp.value().vehicle().disp_name() );
    } else {
        return ret_val<void>::make_success();
    }
}


/**
 * This is a generic mapgen piece, the template parameter PieceType should be another specific
 * type of jmapgen_piece. This class contains a vector of those objects and will chose one of
 * it at random.
 */
template<typename PieceType>
class jmapgen_alternatively : public jmapgen_piece_with_has_vehicle_collision
{
    public:
        // Note: this bypasses virtual function system, all items in this vector are of type
        // PieceType, they *can not* be of any other type.
        std::vector<PieceType> alternatives;
        jmapgen_alternatively() = default;
        mapgen_phase phase() const override {
            if( alternatives.empty() ) {
                return mapgen_phase::default_;
            }
            return alternatives[0].phase();
        }
        void check( const std::string &context, const mapgen_parameters &params,
                    const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z ) const override {
            if( alternatives.empty() ) {
                debugmsg( "zero alternatives in jmapgen_alternatively in %s", context );
            }
            for( const PieceType &piece : alternatives ) {
                piece.check( context, params, x, y, z );
            }
        }
        void merge_parameters_into( mapgen_parameters &params,
                                    const std::string &outer_context ) const override {
            for( const PieceType &piece : alternatives ) {
                piece.merge_parameters_into( params, outer_context );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &context ) const override {
            if( const auto chosen = random_entry_opt( alternatives ) ) {
                chosen->get().apply( dat, x, y, z, context );
            }
        }
};

template<typename Value>
class jmapgen_constrained : public jmapgen_piece
{
    public:
        jmapgen_constrained( shared_ptr_fast<const jmapgen_piece> und,
                             const std::vector<mapgen_constraint<Value>> &cons )
            : underlying_piece( std::move( und ) )
            , constraints( cons )
        {}

        shared_ptr_fast<const jmapgen_piece> underlying_piece;
        std::vector<mapgen_constraint<Value>> constraints;

        mapgen_phase phase() const override {
            return underlying_piece->phase();
        }
        void check( const std::string &context, const mapgen_parameters &params,
                    const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z ) const override {
            underlying_piece->check( context, params, x, y, z );
        }

        void merge_parameters_into( mapgen_parameters &params, const std::string &outer_context
                                  ) const override {
            underlying_piece->merge_parameters_into( params, outer_context );
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &context ) const override {
            for( const mapgen_constraint<Value> &constraint : constraints ) {
                Value param_value = dat.get_arg<Value>( constraint.parameter_name );
                if( param_value != constraint.value ) {
                    return;
                }
            }
            underlying_piece->apply( dat, x, y, z, context );
        }
};

/**
 * Places fields on the map.
 * "field": field type ident.
 * "intensity": initial field intensity.
 * "age": initial field age.
 */
class jmapgen_field : public jmapgen_piece
{
    public:
        mapgen_value<field_type_id> ftype;
        std::vector<int> intensities;
        time_duration age;
        int chance;
        bool remove;
        jmapgen_field( const JsonObject &jsi, std::string_view/*context*/ ) :
            ftype( jsi.get_member( "field" ) )
            , age( time_duration::from_turns( jsi.get_int( "age", 0 ) ) )
            , chance( jsi.get_int( "chance", 100 ) )
            , remove( jsi.get_bool( "remove", false ) ) {
            if( jsi.has_array( "intensity" ) ) {
                for( JsonValue jv : jsi.get_array( "intensity" ) ) {
                    intensities.push_back( jv.get_int() );
                }
            }
            if( intensities.empty() ) {
                intensities.push_back( jsi.get_int( "intensity", 1 ) );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            field_type_id chosen_id = ftype.get( dat );
            if( chosen_id.id().is_null() ) {
                return;
            }
            if( remove ) {
                if( x_in_y( chance, 100 ) ) {
                    dat.m.remove_field( tripoint_bub_ms( x.get(), y.get(), dat.zlevel() + z.get() ), chosen_id );
                }
            } else {
                if( x_in_y( chance, 100 ) ) {
                    dat.m.add_field( tripoint_bub_ms( x.get(), y.get(), dat.zlevel() + z.get() ), chosen_id,
                                     random_entry( intensities ), age );
                }
            }
        }

        void check( const std::string &oter_name, const mapgen_parameters &parameters,
                    const jmapgen_int &/*x*/, const jmapgen_int &/*y*/, const jmapgen_int &/*z*/
                  ) const override {
            ftype.check( oter_name, parameters );
        }
};
/**
 * Place an NPC.
 * "class": the npc class, see @ref map::place_npc
 */
class jmapgen_npc : public jmapgen_piece
{
    public:
        mapgen_value<string_id<npc_template>> npc_class;
        bool target;
        std::vector<trait_id> traits;
        std::string unique_id;
        jmapgen_npc( const JsonObject &jsi, std::string_view/*context*/ ) :
            npc_class( jsi.get_member( "class" ) )
            , target( jsi.get_bool( "target", false ) ) {
            if( jsi.has_string( "add_trait" ) ) {
                traits.emplace_back();
                jsi.read( "add_trait", traits.back() );
            } else if( jsi.has_array( "add_trait" ) ) {
                jsi.read( "add_trait", traits );
            }
            if( jsi.has_string( "unique_id" ) ) {
                jsi.read( "unique_id", unique_id );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            string_id<npc_template> chosen_id = npc_class.get( dat );
            if( chosen_id.is_null() ) {
                return;
            }
            if( !unique_id.empty() && g->unique_npc_exists( unique_id ) ) {
                add_msg_debug( debugmode::DF_NPC, "NPC with unique id %s already exists.", unique_id );
                return;
            }
            tripoint_bub_ms const dst( x.get(), y.get(), dat.zlevel() + z.get() );
            // TODO: Make place_npc 3D aware.
            character_id npc_id = dat.m.place_npc( dst.xy(), chosen_id );
            if( reality_bubble().inbounds( dat.m.get_abs( dst ) ) ) {
                dat.m.queue_main_cleanup();
            }
            if( dat.mission() && target ) {
                dat.mission()->set_target_npc_id( npc_id );
            }
            npc *p = g->find_npc( npc_id );
            if( p != nullptr ) {
                for( const trait_id &new_trait : traits ) {
                    p->set_mutation( new_trait );
                }
                if( !unique_id.empty() ) {
                    p->set_unique_id( unique_id );
                }
            }
        }

        void check( const std::string &oter_name, const mapgen_parameters &parameters,
                    const jmapgen_int &/*x*/, const jmapgen_int &/*y*/, const jmapgen_int &/*z*/
                  ) const override {
            npc_class.check( oter_name, parameters );
        }
};
/**
* Place ownership area
*/
class jmapgen_faction : public jmapgen_piece
{
    public:
        mapgen_value<faction_id> id;
        jmapgen_faction( const JsonObject &jsi, std::string_view/*context*/ )
            : id( jsi.get_member( "id" ) ) {
        }
        mapgen_phase phase() const override {
            return mapgen_phase::faction_ownership;
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y,
                    const jmapgen_int &/*z*/,
                    const std::string &/*context*/ ) const override {
            faction_id chosen_id = id.get( dat );
            if( chosen_id.is_null() ) {
                return;
            }
            // TODO: Make apply_faction_ownership 3D aware.
            dat.m.apply_faction_ownership( point_bub_ms( x.val, y.val ), point_bub_ms( x.valmax, y.valmax ),
                                           chosen_id );
        }

        void check( const std::string &oter_name, const mapgen_parameters &parameters,
                    const jmapgen_int &/*x*/, const jmapgen_int &/*y*/, const jmapgen_int &/*z*/
                  ) const override {
            id.check( oter_name, parameters );
        }
};
/**
 * Place a sign with some text.
 * "signage": the text on the sign.
 */
class jmapgen_sign : public jmapgen_piece_with_has_vehicle_collision
{
    private:
        furn_id sign_furniture;
    public:
        translation signage;
        std::string snippet;
        jmapgen_sign( const JsonObject &jsi, std::string_view/*context*/ ) :
            snippet( jsi.get_string( "snippet", "" ) ) {
            jsi.read( "signage", signage );
            optional( jsi, false, "furniture", sign_furniture, furn_f_sign );
            if( signage.empty() && snippet.empty() ) {
                jsi.throw_error( "jmapgen_sign: needs either signage or snippet" );
            }
            if( !sign_furniture->has_flag( ter_furn_flag::TFLAG_SIGN ) ) {
                jsi.throw_error( "jmapgen_sign: specified furniture needs SIGN flag" );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            const tripoint_bub_ms r( x.get(), y.get(), dat.zlevel() + z.get() );
            dat.m.furn_set( r, furn_str_id::NULL_ID() );
            dat.m.furn_set( r, sign_furniture );

            std::string signtext;

            if( !snippet.empty() ) {
                // select a snippet from the category
                signtext = SNIPPET.random_from_category( snippet ).value_or( translation() ).translated();
            } else if( !signage.empty() ) {
                signtext = signage.translated();
            }
            if( !signtext.empty() ) {
                // replace tags
                std::string cityname = "illegible city name";
                tripoint_abs_sm abs_sub = dat.m.get_abs_sub();
                const city *c = overmap_buffer.closest_city( abs_sub ).city;
                if( c != nullptr ) {
                    cityname = c->name;
                }
                signtext = apply_all_tags( signtext, cityname );
            }
            dat.m.set_signage( r, signtext );
        }
        std::string apply_all_tags( std::string signtext, const std::string &cityname ) const {
            signtext = SNIPPET.expand( signtext );
            replace_city_tag( signtext, cityname );
            return signtext;
        }
};
/**
 * Place graffiti with some text or a snippet.
 * "text": the text of the graffiti.
 * "snippet": snippet category to pull from for text instead.
 */
class jmapgen_graffiti : public jmapgen_piece
{
    public:
        translation text;
        std::string snippet;
        jmapgen_graffiti( const JsonObject &jsi, std::string_view/*context*/ ) :
            snippet( jsi.get_string( "snippet", "" ) ) {
            jsi.read( "text", text );
            if( text.empty() && snippet.empty() ) {
                jsi.throw_error( "jmapgen_graffiti: needs either text or snippet" );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            const tripoint_bub_ms r( x.get(), y.get(), dat.zlevel() + z.get() );

            std::string graffiti;

            if( !snippet.empty() ) {
                // select a snippet from the category
                graffiti = SNIPPET.random_from_category( snippet ).value_or( translation() ).translated();
            } else if( !text.empty() ) {
                graffiti = text.translated();
            }
            if( !graffiti.empty() ) {
                // replace tags
                std::string cityname = "illegible city name";
                tripoint_abs_sm abs_sub = dat.m.get_abs_sub();
                const city *c = overmap_buffer.closest_city( abs_sub ).city;
                if( c != nullptr ) {
                    cityname = c->name;
                }
                graffiti = apply_all_tags( graffiti, cityname );
            }
            dat.m.set_graffiti( r, graffiti );
        }
        std::string apply_all_tags( std::string graffiti, const std::string &cityname ) const {
            graffiti = SNIPPET.expand( graffiti );
            replace_city_tag( graffiti, cityname );
            return graffiti;
        }
};
/**
 * Place a vending machine with content.
 * "item_group": the item group that is used to generate the content of the vending machine.
 */
class jmapgen_vending_machine : public jmapgen_piece_with_has_vehicle_collision
{
    public:
        bool reinforced;
        mapgen_value<item_group_id> group_id;
        bool lootable;
        bool powered;
        bool networked;
        jmapgen_vending_machine( const JsonObject &jsi, std::string_view/*context*/ ) :
            reinforced( jsi.get_bool( "reinforced", false ) )
            , lootable( jsi.get_bool( "lootable", false ) )
            , powered( jsi.get_bool( "powered", false ) )
            , networked( jsi.get_bool( "networked", false ) ) {
            if( jsi.has_member( "item_group" ) ) {
                group_id = mapgen_value<item_group_id>( jsi.get_member( "item_group" ) );
            } else {
                group_id = mapgen_value<item_group_id>( "default_vending_machine" );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            const tripoint_bub_ms r( x.get(), y.get(), dat.zlevel() + z.get() );
            dat.m.furn_set( r, furn_str_id::NULL_ID() );
            item_group_id chosen_id = group_id.get( dat );
            if( chosen_id.is_null() ) {
                return;
            }
            dat.m.place_vending( r, chosen_id, reinforced, lootable, powered, networked );
        }

        void check( const std::string &oter_name, const mapgen_parameters &parameters,
                    const jmapgen_int &/*x*/, const jmapgen_int &/*y*/, const jmapgen_int &/*z*/
                  ) const override {
            group_id.check( oter_name, parameters );
        }
};
/**
 * Place a toilet with (dirty) water in it.
 * "amount": number of water charges to place.
 */
class jmapgen_toilet : public jmapgen_piece_with_has_vehicle_collision
{
    public:
        jmapgen_int amount;
        jmapgen_toilet( const JsonObject &jsi, std::string_view/*context*/ ) :
            amount( jsi, "amount", 0, 0 ) {
        }
        mapgen_phase phase() const override {
            return mapgen_phase::furniture;
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            const tripoint_bub_ms r( x.get(), y.get(), dat.zlevel() + z.get() );
            const int charges = amount.get();
            dat.m.furn_set( r, furn_str_id::NULL_ID() );
            if( charges == 0 ) {
                dat.m.place_toilet( r ); // Use the default charges supplied as default values
            } else {
                dat.m.place_toilet( r, charges );
            }
        }
};
/**
 * Place a gas pump with fuel in it.
 * "amount": number of fuel charges to place.
 */
class jmapgen_gaspump : public jmapgen_piece_with_has_vehicle_collision
{
    public:
        jmapgen_int amount;
        mapgen_value<itype_id> fuel;
        jmapgen_gaspump( const JsonObject &jsi, std::string_view/*context*/ ) :
            amount( jsi, "amount", 0, 0 ) {
            if( jsi.has_member( "fuel" ) ) {
                jsi.read( "fuel", fuel );
            }
        }

        void check( const std::string &oter_name, const mapgen_parameters &parameters,
                    const jmapgen_int &/*x*/, const jmapgen_int &/*y*/, const jmapgen_int &/*z*/
                  ) const override {
            fuel.check( oter_name, parameters );
            static const std::unordered_set<itype_id> valid_fuels = {
                itype_id::NULL_ID(), itype_gasoline, itype_diesel, itype_jp8, itype_avgas
            };
            for( const itype_id &possible_fuel : fuel.all_possible_results( parameters ) ) {
                // may want to not force this, if we want to support other fuels for some reason
                if( !valid_fuels.count( possible_fuel ) ) {
                    debugmsg( "invalid fuel %s in %s", possible_fuel.str(), oter_name );
                }
            }
        }

        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            const tripoint_bub_ms r( x.get(), y.get(), dat.zlevel() + z.get() );
            int charges = amount.get() * 100;
            dat.m.furn_set( r, furn_str_id::NULL_ID() );
            if( charges == 0 ) {
                charges = rng( 10000, 50000 );
            }
            itype_id chosen_fuel = fuel.get( dat );
            if( chosen_fuel.is_null() ) {
                dat.m.place_gas_pump( r, charges );
            } else {
                dat.m.place_gas_pump( r, charges, chosen_fuel );
            }
        }
};

/**
 * Place a specific liquid into the map.
 * "liquid": id of the liquid item (item should use charges)
 * "amount": quantity of liquid placed (a value of -1 uses the default amount)
 * "chance": chance of liquid being placed, see @ref map::place_items
 */
class jmapgen_liquid_item : public jmapgen_piece
{
    public:
        jmapgen_int amount;
        mapgen_value<itype_id> liquid;
        jmapgen_int chance;
        jmapgen_liquid_item( const JsonObject &jsi, std::string_view/*context*/ ) :
            amount( jsi, "amount", -1, -1 )
            , liquid( jsi.get_member( "liquid" ) )
            , chance( jsi, "chance", 1, 1 ) {
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            if( one_in( chance.get() ) ) {
                itype_id chosen_id = liquid.get( dat );
                if( chosen_id.is_null() ) {
                    return;
                }
                // Itemgroups apply migrations when being loaded, but we need to migrate
                // individual items here.
                itype_id migrated = item_controller->migrate_id( chosen_id );
                item newliquid( migrated, calendar::start_of_cataclysm );

                if( amount.val > -1 ) {
                    if( amount.valmax > -1 ) {
                        newliquid.charges = amount.get();
                    } else {
                        newliquid.charges = amount.val;
                    }
                    if( migrated == itype_gasoline ||
                        migrated == itype_avgas ||
                        migrated == itype_diesel ||
                        migrated == itype_jp8 ) {
                        newliquid.charges *= 100;
                    }
                }
                if( newliquid.charges > 0 ) {
                    dat.m.add_item_or_charges(
                        tripoint_bub_ms( x.get(), y.get(), dat.zlevel() + z.get() ), newliquid );
                }
            }
        }

        void check( const std::string &oter_name, const mapgen_parameters &parameters,
                    const jmapgen_int &/*x*/, const jmapgen_int &/*y*/, const jmapgen_int & /*z*/
                  ) const override {
            liquid.check( oter_name, parameters );
        }
};

/**
 * Place a corpse of a random monster from a monster group into the map.
 * "group": id of monster group to choose from
 * "age": age (in days) of monster's corpse
 */
class jmapgen_corpse : public jmapgen_piece
{
    public:
        mongroup_id group;
        time_duration age;
        jmapgen_corpse( const JsonObject &jsi, std::string_view/*context*/ ) :
            group( jsi.get_member( "group" ) )
            , age( time_duration::from_days( jsi.get_int( "age", 0 ) ) ) {
        }

        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            const std::vector<mtype_id> monster_group =
                MonsterGroupManager::GetMonstersFromGroup( group, true );
            const mtype_id &corpse_type = random_entry_ref( monster_group );
            item corpse = item::make_corpse( corpse_type,
                                             std::max( calendar::turn - age, calendar::start_of_cataclysm ) );
            dat.m.add_item_or_charges( tripoint_bub_ms( x.get(), y.get(), dat.zlevel() + z.get() ),
                                       corpse );
        }
};

/**
 * Place items from an item group.
 * "item": id of the item group.
 * "chance": chance of items being placed, see @ref map::place_items
 * "repeat": number of times to apply this piece
 */
class jmapgen_item_group : public jmapgen_piece
{
    public:
        item_group_id group_id;
        jmapgen_int chance;
        std::string faction;
        jmapgen_item_group( const JsonObject &jsi, std::string_view context ) :
            chance( jsi, "chance", 100, 100 ) {
            JsonValue group = jsi.get_member( "item" );
            group_id = item_group::load_item_group( group, "collection",
                                                    str_cat( "mapgen item group ", context ) );
            if( jsi.has_int( "prob" ) ) {
                debugmsg( "prob definition in group %s with context %s should be replaced with chance where chance is a percent and defaults to 100",
                          group_id.c_str(), context );
            }
            repeat = jmapgen_int( jsi, "repeat", 1, 1 );
            if( jsi.has_string( "faction" ) ) {
                faction = jsi.get_string( "faction" );
            }
        }
        void check( const std::string &context, const mapgen_parameters &,
                    const jmapgen_int &/*x*/, const jmapgen_int &/*y*/, const jmapgen_int &/*z*/ ) const override {
            if( !group_id.is_valid() ) {
                debugmsg( "Invalid item_group_id \"%s\" in %s", group_id.str(), context );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            dat.m.place_items( group_id, chance.get(), point_bub_ms( x.val, y.val ), point_bub_ms( x.valmax,
                               y.valmax ),
                               dat.zlevel() + z.get(), true,
                               calendar::start_of_cataclysm, 0, 0, faction );
        }
};

/** Place items from an item group */
class jmapgen_loot : public jmapgen_piece
{
        friend jmapgen_objects;

    public:
        explicit jmapgen_loot( const JsonObject &jsi ) :
            result_group( Item_group::Type::G_COLLECTION, 100, jsi.get_int( "ammo", 0 ),
                          jsi.get_int( "magazine", 0 ), "mapgen loot entry" )
            , chance( jsi.get_int( "chance", 100 ) ) {
            const item_group_id group( jsi.get_string( "group", std::string() ) );
            itype_id ity;
            jsi.read( "item", ity );

            if( group.is_empty() == ity.is_empty() ) {
                jsi.throw_error( "must provide either item or group" );
            }
            if( !group.is_empty() && !item_group::group_is_defined( group ) ) {
                set_mapgen_defer( jsi, "group", "no such item group" );
            }
            if( !ity.is_empty() && !item::type_is_defined( ity ) ) {
                set_mapgen_defer( jsi, "item", "no such item type '" + ity.str() + "'" );
            }

            // All the probabilities are 100 because we do the roll in @ref apply.
            if( group.is_empty() ) {
                // Migrations are applied to item *groups* on load, but single item spawns must be
                // migrated individually
                std::string variant;
                if( jsi.has_string( "variant" ) ) {
                    variant = jsi.get_string( "variant" );
                }
                result_group.add_item_entry( item_controller->migrate_id( ity ), 100, variant );
            } else {
                result_group.add_group_entry( group, 100 );
            }
        }

        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            if( rng( 0, 99 ) < chance ) {
                const Item_spawn_data *const isd = &result_group;
                std::vector<item> spawn;
                spawn.reserve( 20 );
                isd->create( spawn, calendar::start_of_cataclysm,
                             spawn_flags::use_spawn_rate );
                dat.m.spawn_items( tripoint_bub_ms( rng( x.val, x.valmax ), rng( y.val, y.valmax ),
                                                    dat.zlevel() + z.get() ), spawn );
            }
        }

    private:
        Item_group result_group;
        int chance;
};

/**
 * Place spawn points for a monster group (actual monster spawning is done later).
 * "monster": id of the monster group.
 * "chance": see @ref map::place_spawns
 * "density": see @ref map::place_spawns
 */
class jmapgen_monster_group : public jmapgen_piece
{
    public:
        mapgen_value<mongroup_id> id;
        float density;
        jmapgen_int chance;
        jmapgen_monster_group( const JsonObject &jsi, std::string_view/*context*/ ) :
            id( jsi.get_member( "monster" ) )
            , density( jsi.get_float( "density", -1.0f ) )
            , chance( jsi, "chance", 1, 1 ) {
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            mongroup_id chosen_id = id.get( dat );
            if( chosen_id.is_null() ) {
                return;
            }
            dat.m.place_spawns( chosen_id, chance.get(), point_bub_ms( x.val, y.val ),
                                point_bub_ms( x.valmax, y.valmax ), dat.zlevel() + z.get(),
                                density == -1.0f ? dat.monster_density() : density );
        }

        void check( const std::string &oter_name, const mapgen_parameters &parameters,
                    const jmapgen_int &/*x*/, const jmapgen_int &/*y*/, const jmapgen_int &/*z*/
                  ) const override {
            id.check( oter_name, parameters );
        }
};
/**
 * Place spawn points for a specific monster.
 * "monster": id of the monster. or "group": id of the monster group.
 * "friendly": whether the new monster is friendly to the player character.
 * "name": the name of the monster (if it has one).
 * "chance": the percentage chance of a monster, affected by spawn density
 *     If high density means greater than one hundred percent, can place multiples.
 * "repeat": roll this many times for creatures, potentially spawning multiples.
 * "pack_size": place this many creatures each time a roll is successful.
 * "one_or_none": place max of 1 (or pack_size) monsters, even if spawn density > 1.
 *     Defaults to true if repeat and pack_size are unset, false if one is set.
 */
class jmapgen_monster : public jmapgen_piece
{
    public:
        weighted_int_list<mapgen_value<mtype_id>> ids;
        mapgen_value<mongroup_id> m_id;
        jmapgen_int chance;
        jmapgen_int pack_size;
        bool one_or_none;
        bool friendly;
        std::optional<translation> name = std::nullopt;
        std::string random_name_str;
        bool target;
        bool use_pack_size;
        struct spawn_data data;
        jmapgen_monster( const JsonObject &jsi, std::string_view/*context*/ ) :
            chance( jsi, "chance", 100, 100 )
            , pack_size( jsi, "pack_size", 1, 1 )
            , one_or_none( jsi.get_bool( "one_or_none",
                                         !( jsi.has_member( "repeat" ) ||
                                            jsi.has_member( "pack_size" ) ) ) )
            , friendly( jsi.get_bool( "friendly", false ) )
            , random_name_str( jsi.get_string( "random_name", "" ) )
            , target( jsi.get_bool( "target", false ) )
            , use_pack_size( jsi.get_bool( "use_pack_size", false ) ) {

            {
                translation translated_name;
                if( jsi.read( "name", translated_name ) ) {
                    name.emplace( std::move( translated_name ) );
                } else if( random_name_str == "snippet" ) {
                    debugmsg( "Field \"name\" is missing for random name 'snippet'" );
                }
            }

            if( jsi.has_member( "group" ) ) {
                jsi.read( "group", m_id );
            } else if( jsi.has_array( "monster" ) ) {
                load_weighted_list( jsi.get_member( "monster" ), ids, 100 );
            } else {
                mapgen_value<mtype_id> id( jsi.get_member( "monster" ) );
                ids.add( id, 100 );
            }

            std::set<std::string> valid_random_name_strs = { "random", "female", "male", "snippet", "" };
            if( valid_random_name_strs.count( random_name_str ) == 0 ) {
                debugmsg( "Invalid random name '%s'", random_name_str );
            }

            if( jsi.has_object( "spawn_data" ) ) {
                const JsonObject &sd = jsi.get_object( "spawn_data" );
                if( sd.has_array( "ammo" ) ) {
                    const JsonArray &ammos = sd.get_array( "ammo" );
                    for( const JsonObject adata : ammos ) {
                        data.ammo.emplace( itype_id( adata.get_string( "ammo_id" ) ),
                                           jmapgen_int( adata, "qty" ) );
                    }
                }
                if( sd.has_array( "patrol" ) ) {
                    const JsonArray &patrol_pts = sd.get_array( "patrol" );
                    for( const JsonObject p_pt : patrol_pts ) {
                        jmapgen_int ptx = jmapgen_int( p_pt, "x" );
                        jmapgen_int pty = jmapgen_int( p_pt, "y" );
                        //"unnecessary" temporary object created while calling emplace_back [modernize-use-emplace,-warnings-as-errors]
                        const point_rel_ms work_around = point_rel_ms( ptx.get(), pty.get() );
                        data.patrol_points_rel_ms.emplace_back( work_around );
                    }
                }
            }
        }

        void check( const std::string &oter_name, const mapgen_parameters &parameters,
                    const jmapgen_int &/*x*/, const jmapgen_int &/*y*/, const jmapgen_int &/*z*/
                  ) const override {
            for( const weighted_object<int, mapgen_value<mtype_id>> &id : ids ) {
                id.obj.check( oter_name, parameters );
            }
            m_id.check( oter_name, parameters );
        }

        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {

            int raw_odds = chance.get();

            // Handle spawn density: Increase odds, but don't let the odds of absence go below
            // half the odds at density 1.
            // Instead, apply a multiplier to the number of monsters for really high densities.
            // For example, a 50% chance at spawn density 4 becomes a 75% chance of ~2.7 monsters.
            int odds_after_density = raw_odds * get_option<float>( "SPAWN_DENSITY" );
            int max_odds = ( 100 + raw_odds ) / 2;
            float density_multiplier = 1.0f;
            if( odds_after_density > max_odds ) {
                density_multiplier = 1.0f * odds_after_density / max_odds;
                odds_after_density = max_odds;
            }

            int mission_id = -1;
            if( dat.mission() && target ) {
                mission_id = dat.mission()->get_id();
            }

            int spawn_count = roll_remainder( density_multiplier );

            // don't let high spawn density alone cause more than 1 to spawn.
            if( one_or_none ) {
                spawn_count = std::min( spawn_count, 1 );
            }
            // don't spawn less than 1 if odds were 100%, even with low spawn density.
            if( raw_odds == 100 ) {
                spawn_count = std::max( spawn_count, 1 );
            } else {
                if( !x_in_y( odds_after_density, 100 ) ) {
                    return;
                }
            }

            mongroup_id chosen_group = m_id.get( dat );
            std::optional<std::string> chosen_name = std::nullopt;
            if( !random_name_str.empty() ) {
                if( random_name_str == "female" ) {
                    chosen_name.emplace( SNIPPET.expand( "<female_given_name>" ) );
                } else if( random_name_str == "male" ) {
                    chosen_name.emplace( SNIPPET.expand( "<male_given_name>" ) );
                } else if( random_name_str == "random" ) {
                    chosen_name.emplace( SNIPPET.expand( "<given_name>" ) );
                } else if( random_name_str == "snippet" && name.has_value() ) {
                    chosen_name.emplace( SNIPPET.expand( name.value().translated() ) );
                }
            }
            if( !chosen_name.has_value() && name.has_value() ) {
                chosen_name.emplace( name.value().translated() );
            }
            if( !chosen_group.is_null() ) {
                std::vector<MonsterGroupResult> spawn_details =
                    MonsterGroupManager::GetResultFromGroup( chosen_group, nullptr, nullptr, false, nullptr,
                            use_pack_size );
                for( const MonsterGroupResult &mgr : spawn_details ) {
                    dat.m.add_spawn( mgr.id, spawn_count * pack_size.get(),
                    { x.get(), y.get(), dat.zlevel() + z.get()},
                    friendly, -1, mission_id, chosen_name, data );
                }
            } else if( ids.is_valid() ) {
                mtype_id chosen_type = ids.pick()->get( dat );
                if( !chosen_type.is_null() ) {
                    dat.m.add_spawn( chosen_type, spawn_count * pack_size.get(),
                    { x.get(), y.get(), dat.zlevel() + z.get()},
                    friendly, -1, mission_id, chosen_name, data );
                }
            }
        }
};

static inclusive_rectangle<point> vehicle_bounds( const vehicle_prototype &vp )
{
    point min( INT_MAX, INT_MAX );
    point max( INT_MIN, INT_MIN );

    cata_assert( !vp.parts.empty() );

    for( const vehicle_prototype::part_def &part : vp.parts ) {
        min.x = std::min( min.x, part.pos.x() );
        max.x = std::max( max.x, part.pos.x() );
        min.y = std::min( min.y, part.pos.y() );
        max.y = std::max( max.y, part.pos.y() );
    }

    return { min, max };
}

/**
 * Place a vehicle.
 * "vehicle": id of the vehicle.
 * "chance": chance of spawning the vehicle: 0...100
 * "rotation": rotation of the vehicle, see @ref vehicle::vehicle
 * "fuel": fuel status of the vehicle, see @ref vehicle::vehicle
 * "status": overall (damage) status of the vehicle, see @ref vehicle::vehicle
 */
class jmapgen_vehicle : public jmapgen_piece_with_has_vehicle_collision
{
    public:
        mapgen_value<vgroup_id> type;
        int chance;
        std::vector<units::angle> rotation;
        int fuel;
        int status;
        std::string faction;
        jmapgen_vehicle( const JsonObject &jsi, std::string_view/*context*/ ) :
            type( jsi.get_member( "vehicle" ) )
            , chance( jsi.get_int( "chance", 100 ) )
            //, rotation( jsi.get_int( "rotation", 0 ) ) // unless there is a way for the json parser to
            // return a single int as a list, we have to manually check this in the constructor below
            , fuel( jsi.get_int( "fuel", -1 ) )
            , status( jsi.get_int( "status", -1 ) ) {
            if( jsi.has_array( "rotation" ) ) {
                for( const JsonValue elt : jsi.get_array( "rotation" ) ) {
                    rotation.push_back( units::from_degrees( elt.get_int() ) );
                }
            } else {
                rotation.push_back( units::from_degrees( jsi.get_int( "rotation", 0 ) ) );
            }

            if( jsi.has_string( "faction" ) ) {
                faction = jsi.get_string( "faction" );
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            if( !x_in_y( chance, 100 ) ) {
                return;
            }
            vgroup_id chosen_id = type.get( dat );
            if( chosen_id.is_null() ) {
                return;
            }
            tripoint_bub_ms const dst( x.get(), y.get(), dat.zlevel() + z.get() );
            vehicle *veh = dat.m.add_vehicle( chosen_id->pick(), dst, random_entry( rotation ),
                                              fuel, status );
            if( veh && !faction.empty() ) {
                veh->set_owner( faction_id( faction ) );
            }
            if( reality_bubble().inbounds( dat.m.get_abs( dst ) ) ) {
                dat.m.queue_main_cleanup();
            }
        }

        void check( const std::string &context, const mapgen_parameters &parameters,
                    const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &/*z*/
                  ) const override {
            type.check( context, parameters );

            if( x.val == -1 ) {
                // We are in a palette, so no need to verify that the positions
                // are valid
                return;
            }

            // The rest of this function is devoted to ensuring that this
            // vehicle placement does not lead to a vehicle overlapping an OMT
            // boundary, because that causes issues (e.g. if the vehicle is
            // damaged and tries to drop items during mapgen, they may drop on
            // points outside the map).

            point min( INT_MAX, INT_MAX );
            point max( INT_MIN, INT_MIN );
            vgroup_id min_x_vg;
            vproto_id min_x_vp;
            vgroup_id max_x_vg;
            vproto_id max_x_vp;
            vgroup_id min_y_vg;
            vproto_id min_y_vp;
            vgroup_id max_y_vg;
            vproto_id max_y_vp;
            for( const vgroup_id &vg_id : type.all_possible_results( parameters ) ) {
                for( const vproto_id &vp_id : vg_id->all_possible_results() ) {
                    inclusive_rectangle<point> bounds = vehicle_bounds( *vp_id );
                    if( bounds.p_min.x < min.x ) {
                        min_x_vg = vg_id;
                        min_x_vp = vp_id;
                        min.x = bounds.p_min.x;
                    }
                    if( bounds.p_max.x > max.x ) {
                        max_x_vg = vg_id;
                        max_x_vp = vp_id;
                        max.x = bounds.p_max.x;
                    }
                    if( bounds.p_min.y < min.y ) {
                        min_y_vg = vg_id;
                        min_y_vp = vp_id;
                        min.y = bounds.p_min.y;
                    }
                    if( bounds.p_max.y > max.y ) {
                        max_y_vg = vg_id;
                        max_y_vp = vp_id;
                        max.y = bounds.p_max.y;
                    }
                }
            }

            for( units::angle rot : rotation ) {
                int degrees = to_degrees( rot );
                while( degrees < 0 ) {
                    degrees += 360;
                }
                if( degrees % 90 != 0 ) {
                    // TODO: support non-rectilinear vehicle placement
                    continue;
                }
                int turns = degrees / 90;
                point rotated_min = min.rotate( turns );
                point rotated_max = max.rotate( turns );
                point new_min( std::min( rotated_min.x, rotated_max.x ),
                               std::min( rotated_min.y, rotated_max.y ) );
                point new_max( std::max( rotated_min.x, rotated_max.x ),
                               std::max( rotated_min.y, rotated_max.y ) );

                new_min += point( x.val, y.val );
                new_max += point( x.valmax, y.valmax );

                cube_direction bad_rotated_direction = cube_direction::last;
                std::string extreme_coord;

                if( new_min.x < 0 ) {
                    bad_rotated_direction = cube_direction::west;
                    extreme_coord = string_format( "x = %d (should be at least 0)", new_min.x );
                } else if( new_max.x >= 24 ) {
                    bad_rotated_direction = cube_direction::east;
                    extreme_coord = string_format( "x = %d (should be at most 23)", new_max.x );
                } else if( new_min.y < 0 ) {
                    extreme_coord = string_format( "y = %d (should be at least 0)", new_min.y );
                    bad_rotated_direction = cube_direction::north;
                } else if( new_max.y >= 24 ) {
                    bad_rotated_direction = cube_direction::south;
                    extreme_coord = string_format( "y = %d (should be at most 23)", new_max.y );
                } else {
                    continue;
                }

                cube_direction bad_direction = bad_rotated_direction - turns;

                std::string bad_vehicle;
                auto format_option = []( const vgroup_id & vg, const vproto_id & vp ) {
                    return string_format(
                               "[vgroup_id %s; vproto_id %s]", vg.str(), vp.str() );
                };
                switch( bad_direction ) {
                    case cube_direction::north:
                        bad_vehicle = format_option( min_y_vg, min_y_vp );
                        break;
                    case cube_direction::south:
                        bad_vehicle = format_option( max_y_vg, max_y_vp );
                        break;
                    case cube_direction::east:
                        bad_vehicle = format_option( max_x_vg, max_x_vp );
                        break;
                    case cube_direction::west:
                        bad_vehicle = format_option( min_x_vg, min_x_vp );
                        break;
                    default:
                        cata_fatal( "Invalid bad_direction %d", bad_direction );
                }
                debugmsg( "In %s, vehicle placement at x:[%d,%d], y:[%d,%d]: "
                          "potential placement of vehicle out of bounds.  "
                          "At rotation %d the vehicle %s extends too far %s, "
                          "reaching coordinate %s",
                          context, x.val, x.valmax, y.val, y.valmax, degrees,
                          bad_vehicle, io::enum_to_string( bad_rotated_direction ),
                          extreme_coord );
                // Early exit because we don't want too much error spam
                // from the same mapgen piece
                return;
            }
        }
};
/**
 * Place a specific item.
 * "item": id of item type to spawn.
 * "chance": chance of spawning it (1 = always, otherwise one_in(chance)).
 * "amount": amount of items to spawn.
 * "repeat": number of times to apply this piece
 */
class jmapgen_spawn_item : public jmapgen_piece
{
    public:
        mapgen_value<itype_id> type;
        std::string variant;
        std::string faction;
        jmapgen_int amount;
        jmapgen_int chance;
        std::set<flag_id> flags;
        jmapgen_spawn_item( const JsonObject &jsi, std::string_view/*context*/ ) :
            type( jsi.get_member( "item" ) )
            , amount( jsi, "amount", 1, 1 )
            , chance( jsi, "chance", 100, 100 )
            , flags( jsi.get_tags<flag_id>( "custom-flags" ) ) {
            if( jsi.has_string( "variant" ) ) {
                variant = jsi.get_string( "variant" );
            }
            if( jsi.has_string( "faction" ) ) {
                faction = jsi.get_string( "faction" );
            }
            repeat = jmapgen_int( jsi, "repeat", 1, 1 );
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            itype_id chosen_id = type.get( dat );
            if( chosen_id.is_null() ) {
                return;
            }
            // Itemgroups apply migrations when being loaded, but we need to migrate
            // individual items here.
            chosen_id = item_controller->migrate_id( chosen_id );

            const int c = chance.get();

            // 100% chance = exactly 1 item, otherwise scale by item spawn rate.
            const float spawn_rate = get_option<float>( "ITEM_SPAWNRATE" );
            int spawn_count = ( c == 100 ) ? 1 : roll_remainder( c * spawn_rate / 100.0f );
            for( int i = 0; i < spawn_count; i++ ) {
                dat.m.spawn_item( tripoint_bub_ms( x.get(), y.get(), dat.zlevel() + z.get() ), chosen_id,
                                  amount.get(),
                                  0, calendar::start_of_cataclysm, 0, flags, variant, faction );
            }
        }

        void check( const std::string &oter_name, const mapgen_parameters &parameters,
                    const jmapgen_int &/*x*/, const jmapgen_int &/*y*/, const jmapgen_int &/*z*/
                  ) const override {
            type.check( oter_name, parameters );
        }
};
/**
 * Place a trap.
 * "trap": id of the trap.
 */
class jmapgen_trap : public jmapgen_piece_with_has_vehicle_collision
{
    public:
        mapgen_value<trap_id> id;

        jmapgen_trap( const JsonObject &jsi, std::string_view/*context*/ ) :
            jmapgen_trap( jsi.get_member( "trap" ) ) {}
        explicit jmapgen_trap( const JsonValue &fid ) : id( fid ) {}

        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            trap_id chosen_id = id.get( dat );
            if( chosen_id.id().is_null() ) {
                return;
            }
            const tripoint_bub_ms actual_loc{ x.get(), y.get(), dat.zlevel() + z.get() };
            dat.m.trap_set( actual_loc, chosen_id );
        }

        void check( const std::string &oter_name, const mapgen_parameters &parameters,
                    const jmapgen_int &/*x*/, const jmapgen_int &/*y*/, const jmapgen_int &/*z*/
                  ) const override {
            id.check( oter_name, parameters );
        }
};
/**
 * Place a furniture.
 * "furn": id of the furniture.
 */
class jmapgen_furniture : public jmapgen_piece_with_has_vehicle_collision
{
    public:
        mapgen_value<furn_id> id;
        jmapgen_furniture( const JsonObject &jsi, std::string_view/*context*/ ) :
            jmapgen_furniture( jsi.get_member( "furn" ) ) {}
        explicit jmapgen_furniture( const JsonValue &fid ) : id( fid ) {}
        mapgen_phase phase() const override {
            return mapgen_phase::furniture;
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &context ) const override {
            const furn_id &chosen_id = id.get( dat );
            if( chosen_id.id().is_null() ) {
                return;
            }
            if( !dat.m.furn_set( tripoint_bub_ms( x.get(), y.get(), dat.zlevel() + z.get() ), chosen_id ) ) {
                debugmsg( "Problem setting furniture in %s", context );
            }
        }

        void check( const std::string &oter_name, const mapgen_parameters &parameters,
                    const jmapgen_int &/*x*/, const jmapgen_int &/*y*/, const jmapgen_int &/*z*/
                  ) const override {
            id.check( oter_name, parameters );
        }
};
/**
 * Place terrain.
 * "ter": id of the terrain.
 */
class jmapgen_terrain : public jmapgen_piece_with_has_vehicle_collision
{
    private:
        enum apply_action {
            act_unknown, act_ignore, act_dismantle, act_erase
        };
    public:
        mapgen_value<ter_id> id;
        jmapgen_terrain( const JsonObject &jsi, std::string_view/*context*/ ) :
            jmapgen_terrain( jsi.get_member( "ter" ) ) {}
        explicit jmapgen_terrain( const JsonValue &tid ) : id( mapgen_value<ter_id>( tid ) ) {}

        bool is_nop() const override {
            return id.is_null();
        }
        mapgen_phase phase() const override {
            return mapgen_phase::terrain;
        }

        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &context ) const override {
            const ter_id &chosen_id = id.get( dat );
            if( chosen_id.id().is_null() ) {
                return;
            }
            tripoint_bub_ms p( x.get(), y.get(), dat.zlevel() + z.get() );

            const ter_id &terrain_here = dat.m.ter( p );
            const ter_t &chosen_ter = *chosen_id;
            const bool is_wall = chosen_ter.has_flag( ter_furn_flag::TFLAG_WALL );
            const bool place_item = chosen_ter.has_flag( ter_furn_flag::TFLAG_PLACE_ITEM );
            const bool is_boring_wall = is_wall && !place_item;

            apply_action act_furn = apply_action::act_unknown;
            apply_action act_trap = apply_action::act_unknown;
            apply_action act_item = apply_action::act_unknown;

            // shorthand flags
            if( dat.has_flag( jmapgen_flags::allow_terrain_under_other_data ) ) {
                act_furn = apply_action::act_ignore;
                act_trap = apply_action::act_ignore;
                act_item = apply_action::act_ignore;
            } else if( dat.has_flag( jmapgen_flags::dismantle_all_before_placing_terrain ) ) {
                act_furn = apply_action::act_dismantle;
                act_trap = apply_action::act_dismantle;
                act_item = apply_action::act_ignore;
            } else if( dat.has_flag( jmapgen_flags::erase_all_before_placing_terrain ) ) {
                act_furn = apply_action::act_erase;
                act_trap = apply_action::act_erase;
                act_item = apply_action::act_erase;
            }

            // specific flags override shorthand flags
            if( dat.has_flag( jmapgen_flags::allow_terrain_under_furniture ) ) {
                act_furn = apply_action::act_ignore;
            } else if( dat.has_flag( jmapgen_flags::dismantle_furniture_before_placing_terrain ) ) {
                act_furn = apply_action::act_dismantle;
            } else if( dat.has_flag( jmapgen_flags::erase_furniture_before_placing_terrain ) ) {
                act_furn = apply_action::act_erase;
            }
            if( dat.has_flag( jmapgen_flags::allow_terrain_under_trap ) ) {
                act_trap = apply_action::act_ignore;
            } else if( dat.has_flag( jmapgen_flags::dismantle_trap_before_placing_terrain ) ) {
                act_trap = apply_action::act_dismantle;
            } else if( dat.has_flag( jmapgen_flags::erase_trap_before_placing_terrain ) ) {
                act_trap = apply_action::act_erase;
            }
            if( dat.has_flag( jmapgen_flags::allow_terrain_under_items ) ) {
                act_item = apply_action::act_ignore;
            } else if( dat.has_flag( jmapgen_flags::erase_items_before_placing_terrain ) ) {
                act_item = apply_action::act_erase;
            }

            if( act_item == apply_action::act_erase &&
                ( act_furn == apply_action::act_dismantle || act_trap == apply_action::act_dismantle ) ) {
                debugmsg( "In %s on %s, the mapgen is configured to dismantle preexisting furniture "
                          "and/or traps, but will also erase preexisting items.  This is probably a "
                          "mistake, as any dismantle outputs will not be preserved.",
                          context, dat.terrain_type().id().str() );
            }

            if( is_boring_wall || act_furn == apply_action::act_erase ) {
                dat.m.furn_clear( p );
                // remove sign writing data from the submap
                dat.m.delete_signage( p );
            } else if( act_furn == apply_action::act_dismantle ) {
                int max_recurse = 10; // insurance against infinite looping
                std::string initial_furn = dat.m.furn( p ) != furn_str_id::NULL_ID() ? dat.m.furn(
                                               p ).id().str() : "";
                while( dat.m.has_furn( p ) && --max_recurse > 0 ) {
                    const furn_t &f = dat.m.furn( p ).obj();
                    if( f.deconstruct ) {
                        if( f.deconstruct->furn_set.str().empty() ) {
                            dat.m.furn_clear( p );
                        } else {
                            dat.m.furn_set( p, f.deconstruct->furn_set );
                        }
                        dat.m.spawn_items( p, item_group::items_from( f.deconstruct->drop_group, calendar::turn ) );
                    } else {
                        const std::optional<map_furn_bash_info> &furn_bash = f.bash;
                        if( furn_bash ) {
                            if( furn_bash->furn_set.str().empty() ) {
                                dat.m.furn_clear( p );
                            } else {
                                dat.m.furn_set( p, furn_bash->furn_set );
                            }
                            dat.m.drop_bash_results( f, p );
                        } else {
                            break;
                        }
                    }
                }
                if( max_recurse <= 0 ) {
                    dat.m.furn_clear( p );
                    debugmsg( "In %s on %s, the mapgen failed to arrive at an empty tile after "
                              "dismantling preexisting furniture %s at %s 10 times in succession.",
                              context, dat.terrain_type().id().str(), initial_furn, p.to_string() );
                }
                // remove sign writing data from the submap
                dat.m.delete_signage( p );
            }

            if( is_boring_wall || act_trap == apply_action::act_erase ) {
                dat.m.remove_trap( p );
            } else if( act_trap == apply_action::act_dismantle ) {
                dat.m.tr_at( p ).on_disarmed( dat.m, p );
            }

            if( is_boring_wall || act_item == apply_action::act_erase ) {
                dat.m.i_clear( p );
            }

            if( chosen_id != terrain_here ) {
                std::string error;
                trap_str_id trap_here = dat.m.tr_at( p ).id;
                if( act_furn != apply_action::act_ignore && dat.m.furn( p ) != furn_str_id::NULL_ID() ) {
                    // NOLINTNEXTLINE(cata-translate-string-literal)
                    error = string_format( "furniture was %s", dat.m.furn( p ).id().str() );
                } else if( act_trap != apply_action::act_ignore && !trap_here.is_null() &&
                           trap_here.id() != terrain_here->trap ) {
                    // NOLINTNEXTLINE(cata-translate-string-literal)
                    error = string_format( "trap %s existed", trap_here.str() );
                } else if( act_item != apply_action::act_ignore && !dat.m.i_at( p ).empty() ) {
                    // NOLINTNEXTLINE(cata-translate-string-literal)
                    error = string_format( "item %s existed",
                                           dat.m.i_at( p ).begin()->typeId().str() );
                }
                if( !error.empty() ) {
                    debugmsg( "In %s on %s, setting terrain to %s (from %s) at %s when %s.  "
                              "Resolve this either by removing the terrain from this mapgen, "
                              "adding suitable removal commands to the mapgen, or by adding an "
                              "appropriate clearing flag to the innermost layered mapgen.  "
                              "Consult the \"mapgen flags\" section in MAPGEN.md for options.",
                              context, dat.terrain_type().id().str(), chosen_id.id().str(),
                              terrain_here.id().str(), p.to_string(), error );
                }
            }
            dat.m.ter_set( p, chosen_id );
        }

        void check( const std::string &oter_name, const mapgen_parameters &parameters,
                    const jmapgen_int &/*x*/, const jmapgen_int &/*y*/, const jmapgen_int &/*z*/
                  ) const override {
            id.check( oter_name, parameters );
        }
};
/**
 * Run a transformation.
 * "transform": id of the ter_furn_transform to run.
 */
class jmapgen_ter_furn_transform: public jmapgen_piece
{
    public:
        mapgen_value<ter_furn_transform_id> id;
        jmapgen_ter_furn_transform( const JsonObject &jsi, std::string_view/*context*/ ) :
            id( jsi.get_member( "transform" ) ) {}

        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            ter_furn_transform_id chosen_id = id.get( dat );
            if( chosen_id.is_null() ) {
                return;
            }
            for( int ix = x.val; ix <= x.valmax; ix++ ) {
                for( int iy = y.val; iy <= y.valmax; iy++ ) {
                    chosen_id->transform( dat.m, tripoint_bub_ms( ix, iy, dat.zlevel() + z.get() ) );
                }
            }
        }

        void check( const std::string &oter_name, const mapgen_parameters &parameters,
                    const jmapgen_int &/*x*/, const jmapgen_int &/*y*/, const jmapgen_int &/*z*/
                  ) const override {
            id.check( oter_name, parameters );
        }
};
/**
 * Calls @ref map::make_rubble to create rubble and destroy the existing terrain/furniture.
 * See map::make_rubble for explanation of the parameters.
 */
class jmapgen_make_rubble : public jmapgen_piece
{
    public:
        mapgen_value<furn_id> rubble_type = mapgen_value<furn_id>( furn_f_rubble );
        bool items = false;
        mapgen_value<ter_id> floor_type = mapgen_value<ter_id>( ter_t_dirt );
        bool overwrite = false;
        jmapgen_make_rubble( const JsonObject &jsi, std::string_view/*context*/ ) {
            if( jsi.has_member( "rubble_type" ) ) {
                rubble_type = mapgen_value<furn_id>( jsi.get_member( "rubble_type" ) );
            }
            jsi.read( "items", items );
            if( jsi.has_member( "floor_type" ) ) {
                floor_type = mapgen_value<ter_id>( jsi.get_member( "floor_type" ) );
            }
            jsi.read( "overwrite", overwrite );
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            const furn_id &chosen_rubble_type = rubble_type.get( dat );
            ter_id chosen_floor_type = floor_type.get( dat );
            if( chosen_rubble_type.id().is_null() ) {
                return;
            }
            if( chosen_floor_type.id().is_null() ) {
                debugmsg( "null floor type when making rubble" );
                chosen_floor_type = ter_t_dirt;
            }
            dat.m.make_rubble( tripoint_bub_ms( x.get(), y.get(), dat.zlevel() + z.get() ),
                               chosen_rubble_type, items, chosen_floor_type, overwrite );
        }

        void check( const std::string &oter_name, const mapgen_parameters &parameters,
                    const jmapgen_int &/*x*/, const jmapgen_int &/*y*/, const jmapgen_int &/*z*/
                  ) const override {
            rubble_type.check( oter_name, parameters );
            floor_type.check( oter_name, parameters );
        }
};

/**
 * Place a computer (console) with given stats and effects.
 * @param options Array of @ref computer_option
 * @param failures Array of failure effects (see @ref computer_failure)
 */
class jmapgen_computer : public jmapgen_piece_with_has_vehicle_collision
{
    public:
        translation name;
        translation access_denied;
        int security;
        std::vector<computer_option> options;
        std::vector<computer_failure> failures;
        std::vector<std::string> chat_topics;
        std::vector<effect_on_condition_id> eocs;
        bool target;
        jmapgen_computer( const JsonObject &jsi, std::string_view/*context*/ ) {
            jsi.read( "name", name );
            jsi.read( "access_denied", access_denied );
            security = jsi.get_int( "security", 0 );
            target = jsi.get_bool( "target", false );
            if( jsi.has_array( "options" ) ) {
                for( JsonObject jo : jsi.get_array( "options" ) ) {
                    options.emplace_back( computer_option::from_json( jo ) );
                }
            }
            if( jsi.has_array( "failures" ) ) {
                for( JsonObject jo : jsi.get_array( "failures" ) ) {
                    failures.emplace_back( computer_failure::from_json( jo ) );
                }
            }
            if( jsi.has_array( "eocs" ) ) {
                for( const std::string &eoc : jsi.get_string_array( "eocs" ) ) {
                    eocs.emplace_back( eoc );
                }
            }
            if( jsi.has_array( "chat_topics" ) ) {
                for( const std::string &topic : jsi.get_string_array( "chat_topics" ) ) {
                    chat_topics.emplace_back( topic );
                }
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            const tripoint_bub_ms r( x.get(), y.get(), dat.zlevel() + z.get() );
            dat.m.furn_set( r, furn_f_console );
            computer *cpu =
                dat.m.add_computer( r, name.translated(),
                                    security );
            for( const computer_option &opt : options ) {
                cpu->add_option( opt );
            }
            for( const computer_failure &opt : failures ) {
                cpu->add_failure( opt );
            }
            if( target && dat.mission() ) {
                cpu->set_mission( dat.mission()->get_id() );
            }
            for( const effect_on_condition_id &eoc : eocs ) {
                cpu->add_eoc( eoc );
            }
            for( const std::string &chat_topic : chat_topics ) {
                cpu->add_chat_topic( chat_topic );
            }
            // The default access denied message is defined in computer's constructor
            if( !access_denied.empty() ) {
                cpu->set_access_denied_msg( access_denied.translated() );
            }
        }
};

/**
 * Place an item in furniture (expected to be used with NOITEM SEALED furniture like plants).
 * "item": item to spawn (object with usual parameters).
 * "items": item group to spawn (object with usual parameters).
 * "furniture": furniture to create around it.
 */
class jmapgen_sealed_item : public jmapgen_piece_with_has_vehicle_collision
{
    public:
        mapgen_value<furn_id> furniture;
        jmapgen_int chance;
        std::optional<jmapgen_spawn_item> item_spawner;
        std::optional<jmapgen_item_group> item_group_spawner;
        jmapgen_sealed_item( const JsonObject &jsi, const std::string &context )
            : furniture( jsi.get_member( "furniture" ) )
            , chance( jsi, "chance", 100, 100 ) {
            if( jsi.has_object( "item" ) ) {
                JsonObject item_obj = jsi.get_object( "item" );
                item_spawner = jmapgen_spawn_item( item_obj, "item for " + context );
            }
            if( jsi.has_object( "items" ) ) {
                JsonObject items_obj = jsi.get_object( "items" );
                item_group_spawner = jmapgen_item_group( items_obj, "items for " + context );
            }
        }

        void check( const std::string &context, const mapgen_parameters &params,
                    const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z ) const override {
            std::string short_summary =
                string_format( "sealed_item special in json mapgen for %s", context );
            if( !item_spawner && !item_group_spawner ) {
                debugmsg( "%s specifies neither an item nor an item group.  "
                          "It should specify at least one.",
                          short_summary );
                return;
            }

            for( const furn_str_id &f : furniture.all_possible_results( params ) ) {
                std::string summary =
                    string_format( "%s using furniture %s", short_summary, f.str() );

                if( !f.is_valid() ) {
                    debugmsg( "%s which is not valid furniture", summary );
                    return;
                }

                const furn_t &furn = *f;

                if( furn.has_flag( ter_furn_flag::TFLAG_PLANT ) ) {
                    // plant furniture requires exactly one seed item within it
                    if( item_spawner && item_group_spawner ) {
                        debugmsg( "%s (with flag PLANT) specifies both an item and an item group.  "
                                  "It should specify exactly one.",
                                  summary );
                        return;
                    }

                    if( item_spawner ) {
                        item_spawner->check( context, params, x, y, z );
                        int count = item_spawner->amount.get();
                        if( count != 1 ) {
                            debugmsg( "%s (with flag PLANT) spawns %d items; it should spawn "
                                      "exactly one.", summary, count );
                            return;
                        }
                        int item_chance = item_spawner->chance.get();
                        if( item_chance != 100 ) {
                            debugmsg( "%s (with flag PLANT) spawns an item with probability %d%%; "
                                      "it should always spawn.  You can move the \"chance\" up to "
                                      "the sealed_item instead of the \"item\" within.",
                                      summary, item_chance );
                            return;
                        }
                        for( const itype_id &t :
                             item_spawner->type.all_possible_results( params ) ) {
                            const itype *spawned_type = item::find_type( t );
                            if( !spawned_type->seed ) {
                                debugmsg( "%s (with flag PLANT) spawns item type %s which is not a "
                                          "seed.", summary, spawned_type->get_id().str() );
                                return;
                            }
                        }
                    }

                    if( item_group_spawner ) {
                        item_group_spawner->check( context, params, x, y, z );
                        int ig_chance = item_group_spawner->chance.get();
                        if( ig_chance != 100 ) {
                            debugmsg( "%s (with flag PLANT) spawns item group %s with chance %d.  "
                                      "It should have chance 100.  You can move the \"chance\" up "
                                      "to the sealed_item instead of the \"items\" within.",
                                      summary, item_group_spawner->group_id.str(), ig_chance );
                            return;
                        }
                        item_group_id group_id = item_group_spawner->group_id;
                        for( const itype *type :
                             item_group::every_possible_item_from( group_id ) ) {
                            if( !type->seed ) {
                                debugmsg( "%s (with flag PLANT) spawns item group %s which can "
                                          "spawn item %s which is not a seed.",
                                          summary, group_id.str(), type->get_id().str() );
                                return;
                            }
                        }

                    }
                }
            }
        }

        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &context ) const override {
            const int c = chance.get();

            // 100% chance = always generate, otherwise scale by item spawn rate.
            // (except is capped at 1)
            const float spawn_rate = get_option<float>( "ITEM_SPAWNRATE" );
            if( !x_in_y( ( c == 100 ) ? 1 : c * spawn_rate / 100.0f, 1 ) ) {
                return;
            }

            dat.m.furn_set( tripoint_bub_ms( x.get(), y.get(), dat.zlevel() + z.get() ),
                            furn_str_id::NULL_ID() );
            if( item_spawner ) {
                item_spawner->apply( dat, x, y, z, context );
            }
            if( item_group_spawner ) {
                item_group_spawner->apply( dat, x, y, z,  context );
            }
            const furn_id &chosen_furn = furniture.get( dat );
            dat.m.furn_set( tripoint_bub_ms( x.get(), y.get(), dat.zlevel() + z.get() ), chosen_furn );
        }
};

/**
 * Place a zone
 */
class jmapgen_zone : public jmapgen_piece
{
    public:
        mapgen_value<zone_type_id> zone_type;
        mapgen_value<faction_id> faction;
        std::string name;
        std::string filter;
        jmapgen_zone( const JsonObject &jsi, std::string_view/*context*/ )
            : zone_type( jsi.get_member( "type" ) )
            , faction( jsi.get_member( "faction" ) ) {
            if( jsi.has_string( "name" ) ) {
                name = jsi.get_string( "name" );
            }
            if( jsi.has_string( "filter" ) ) {
                filter = jsi.get_string( "filter" );
            }
        }
        mapgen_phase phase() const override {
            return mapgen_phase::zones;
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            zone_type_id chosen_zone_type = zone_type.get( dat );
            faction_id chosen_faction = faction.get( dat );
            const tripoint_abs_ms start = dat.m.get_abs( tripoint_bub_ms( int( x.val ), int( y.val ),
                                          dat.zlevel() + z.get() ) );
            const tripoint_abs_ms end = dat.m.get_abs( tripoint_bub_ms( int( x.valmax ), int( y.valmax ),
                                        dat.zlevel() + z.get() ) );
            mapgen_place_zone( start, end, chosen_zone_type, chosen_faction, name, filter, &dat.m );
        }

        void check( const std::string &oter_name, const mapgen_parameters &parameters,
                    const jmapgen_int &/*x*/, const jmapgen_int &/*y*/, const jmapgen_int &/*z*/
                  ) const override {
            zone_type.check( oter_name, parameters );
            faction.check( oter_name, parameters );
        }
};

/**
 * Place a variable
 */
class jmapgen_variable : public jmapgen_piece
{
    public:
        std::string name;
        jmapgen_variable( const JsonObject &jsi, std::string_view /*context*/ ) {
            name = jsi.get_string( "name" );
        }
        mapgen_phase phase() const override {
            return mapgen_phase::zones;
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            queued_points[name] = dat.m.get_abs( tripoint_bub_ms( int( x.val ), int( y.val ),
                                                 dat.zlevel() + z.get() ) );
        }
};

/**
 * Removes vehicles
 */
class jmapgen_remove_vehicles : public jmapgen_piece
{
    public:
        std::vector<vproto_id> vehicles_to_remove;
        jmapgen_remove_vehicles( const JsonObject &jo, std::string_view/*context*/ ) {
            for( std::string vehicle_prototype_id : jo.get_string_array( "vehicles" ) ) {
                vehicles_to_remove.emplace_back( vehicle_prototype_id );
            }
        }
        mapgen_phase phase() const override {
            return mapgen_phase::removal;
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {
            const tripoint_bub_ms start( int( x.val ), int( y.val ), dat.zlevel() + z.get() );
            const tripoint_bub_ms end( int( x.valmax ), int( y.valmax ), dat.zlevel() + z.get() );
            const tripoint_range<tripoint_bub_ms> range = tripoint_range<tripoint_bub_ms>( start, end );
            auto is_in_vehicles_to_remove = [&]( vproto_id & vproto ) -> bool {
                return std::find( vehicles_to_remove.begin(), vehicles_to_remove.end(), vproto ) != vehicles_to_remove.end();
            };
            for( const tripoint_bub_ms &p : range ) {
                optional_vpart_position vp = dat.m.veh_at( p );
                if( vp && ( vehicles_to_remove.empty() || is_in_vehicles_to_remove( vp->vehicle().type ) ) ) {
                    reality_bubble().remove_vehicle_from_cache( &vp->vehicle(), start.z(), end.z() );
                    dat.m.destroy_vehicle( &vp->vehicle() );
                }
            }
        }
};

class jmapgen_remove_npcs : public jmapgen_piece
{
    public:
        std::string npc_class;
        std::string unique_id;
        jmapgen_remove_npcs( const JsonObject &jsi, std::string_view /*context*/ ) {
            optional( jsi, false, "class", npc_class );
            optional( jsi, false, "unique_id", unique_id );
        }
        void apply( const mapgendata &dat, const jmapgen_int & /*x*/, const jmapgen_int & /*y*/,
                    const jmapgen_int &z,
                    const std::string & /*context*/ ) const override {
            tripoint_abs_omt omt_pos = project_to<coords::omt>( dat.m.get_abs_sub() );
            omt_pos.z() += z.get();
            for( auto const &npc : overmap_buffer.get_npcs_near_omt(
                     omt_pos, 0 ) ) {
                if( !npc->is_dead() &&
                    ( npc_class.empty() || npc->idz == string_id<npc_template>( npc_class ) ) &&
                    ( unique_id.empty() || unique_id == npc->get_unique_id() ) ) {
                    overmap_buffer.remove_npc( npc->getID() );
                    if( !unique_id.empty() ) {
                        g->unique_npc_despawn( unique_id );
                    }
                    if( reality_bubble().inbounds( npc->pos_abs() ) ) {
                        g->remove_npc( npc->getID() );
                        get_avatar().get_mon_visible().remove_npc( npc.get() );
                    }
                }
            }
        }
};

// Removes furniture, traps, vehicles, items, fields, graffiti
class jmapgen_remove_all : public jmapgen_piece
{
    public:
        jmapgen_remove_all( const JsonObject &/*jo*/, std::string_view/*context*/ ) {
        }
        mapgen_phase phase() const override {
            return mapgen_phase::removal;
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &/*context*/ ) const override {

            const tripoint_bub_ms start = tripoint_bub_ms( int( x.val ), int( y.val ), dat.zlevel() + z.get() );
            const tripoint_bub_ms end = tripoint_bub_ms( int( x.valmax ), int( y.valmax ),
                                        dat.zlevel() + z.get() );
            for( const tripoint_bub_ms &p : tripoint_range<tripoint_bub_ms>( start, end ) ) {
                dat.m.furn_clear( p );
                dat.m.i_clear( p );
                dat.m.remove_trap( p );
                dat.m.clear_fields( p );
                dat.m.delete_graffiti( p );
                if( optional_vpart_position vp = dat.m.veh_at( p ) ) {
                    map &bubble_map = reality_bubble();

                    if( bubble_map.inbounds( dat.m.get_abs( start ) ) ) {
                        bubble_map.remove_vehicle_from_cache( &vp->vehicle(), start.z(), end.z() );
                    }
                    dat.m.destroy_vehicle( &vp->vehicle() );
                }
            }
        }
};

/**
 * Calls another mapgen call inside the current one.
 * Note: can't use regular overmap ids.
 * @param entries list of pairs [nested mapgen id, weight].
 */
class jmapgen_nested : public jmapgen_piece
{
    private:
        class neighbor_oter_check
        {
            private:
                std::unordered_map<direction, cata::flat_set<std::pair<std::string, ot_match_type>>> neighbors;
            public:
                explicit neighbor_oter_check( const JsonObject &jsi ) {
                    for( direction dir : all_enum_values<direction>() ) {
                        std::string location = io::enum_to_string( dir );
                        cata::flat_set<std::pair<std::string, ot_match_type>> dir_neighbors;
                        if( !jsi.has_string( location ) ) {
                            for( const JsonValue entry : jsi.get_array( location ) ) {
                                std::pair<std::string, ot_match_type> dir_neighbor;
                                if( entry.test_string() ) {
                                    dir_neighbor.first = entry.get_string();
                                    dir_neighbor.second = ot_match_type::contains;
                                } else {
                                    JsonObject jo = entry.get_object();
                                    dir_neighbor.first = jo.get_string( "om_terrain" );
                                    dir_neighbor.second = jo.get_enum_value<ot_match_type>( "om_terrain_match_type",
                                                          ot_match_type::contains );
                                }
                                dir_neighbors.insert( dir_neighbor );
                            }
                        } else {
                            std::pair<std::string, ot_match_type> dir_neighbor;
                            dir_neighbor.first = jsi.get_string( location );
                            dir_neighbor.second = ot_match_type::contains;
                            dir_neighbors.insert( dir_neighbor );
                        }
                        if( !dir_neighbors.empty() ) {
                            neighbors[dir] = std::move( dir_neighbors );
                        }
                    }
                }

                bool test( const mapgendata &dat ) const {
                    for( const std::pair<const direction, cata::flat_set<std::pair<std::string, ot_match_type>>> &p :
                         neighbors ) {
                        const direction dir = p.first;
                        const cata::flat_set<std::pair<std::string, ot_match_type>> &allowed_neighbors = p.second;

                        cata_assert( !allowed_neighbors.empty() );

                        bool this_direction_matches = false;
                        for( const std::pair<std::string, ot_match_type> &allowed_neighbor : allowed_neighbors ) {
                            this_direction_matches |=
                                is_ot_match( allowed_neighbor.first, dat.neighbor_at( dir ).id(),
                                             allowed_neighbor.second );
                        }
                        if( !this_direction_matches ) {
                            return false;
                        }
                    }
                    return true;
                }
        };

        class neighbor_join_check
        {
            private:
                std::unordered_map<cube_direction, cata::flat_set<std::string>> neighbors;
            public:
                explicit neighbor_join_check( const JsonObject &jsi ) {
                    for( cube_direction dir : all_enum_values<cube_direction>() ) {
                        cata::flat_set<std::string> dir_neighbors =
                            jsi.get_tags<std::string, cata::flat_set<std::string>>(
                                io::enum_to_string( dir ) );
                        if( !dir_neighbors.empty() ) {
                            neighbors[dir] = std::move( dir_neighbors );
                        }
                    }
                }

                void check( std::string_view, const mapgen_parameters & ) const {
                    // TODO: check join ids are valid
                }

                bool test( const mapgendata &dat ) const {
                    for( const std::pair<const cube_direction, cata::flat_set<std::string>> &p :
                         neighbors ) {
                        const cube_direction dir = p.first;
                        const cata::flat_set<std::string> &allowed_joins = p.second;

                        cata_assert( !allowed_joins.empty() );

                        bool this_direction_has_join = false;
                        for( const std::string &allowed_join : allowed_joins ) {
                            this_direction_has_join |= dat.has_join( dir, allowed_join );
                        }
                        if( !this_direction_has_join ) {
                            return false;
                        }
                    }
                    return true;
                }
        };
        class neighbor_flag_check
        {
            private:
                std::unordered_map<direction, cata::flat_set<oter_flags>> neighbors;
            public:
                explicit neighbor_flag_check( const JsonObject &jsi ) {
                    for( direction dir : all_enum_values<direction>() ) {
                        cata::flat_set<oter_flags> dir_neighbors;
                        std::string location = io::enum_to_string( dir );
                        optional( jsi, false, location, dir_neighbors );
                        if( !dir_neighbors.empty() ) {
                            neighbors[dir] = std::move( dir_neighbors );
                        }
                    }
                }

                bool test( const mapgendata &dat ) const {
                    for( const std::pair<const direction, cata::flat_set<oter_flags>> &p :
                         neighbors ) {
                        const direction dir = p.first;
                        const cata::flat_set<oter_flags> &allowed_flags = p.second;

                        cata_assert( !allowed_flags.empty() );

                        bool this_direction_has_flag = false;
                        for( const oter_flags &allowed_flag : allowed_flags ) {
                            this_direction_has_flag |=
                                dat.neighbor_at( dir )->has_flag( allowed_flag );
                        }
                        if( !this_direction_has_flag ) {
                            return false;
                        }
                    }
                    return true;
                }
        };
        // TO DO: Remove this and replace it with a smarter logical syntax for neighbor_join_check and neighbor_flag_check
        class neighbor_flag_any_check
        {
            private:
                std::unordered_map<direction, cata::flat_set<oter_flags>> neighbors;
            public:
                explicit neighbor_flag_any_check( const JsonObject &jsi ) {
                    for( direction dir : all_enum_values<direction>() ) {
                        cata::flat_set<oter_flags> dir_neighbors;
                        std::string location = io::enum_to_string( dir );
                        optional( jsi, false, location, dir_neighbors );
                        if( !dir_neighbors.empty() ) {
                            neighbors[dir] = std::move( dir_neighbors );
                        }
                    }
                }

                bool test( const mapgendata &dat ) const {
                    for( const std::pair<const direction, cata::flat_set<oter_flags>> &p :
                         neighbors ) {
                        const direction dir = p.first;
                        const cata::flat_set<oter_flags> &allowed_flags = p.second;

                        cata_assert( !allowed_flags.empty() );

                        for( const oter_flags &allowed_flag : allowed_flags ) {
                            if( dat.neighbor_at( dir )->has_flag( allowed_flag ) ) {
                                return true;
                            }
                        }
                    }
                    return neighbors.empty();
                }
        };
        class predecessor_oter_check
        {
            private:
                cata::flat_set<std::pair<std::string, ot_match_type>> allowed_predecessors;
            public:
                explicit predecessor_oter_check( const JsonArray &jarr ) {
                    for( const JsonValue entry : jarr ) {
                        std::pair<std::string, ot_match_type> allowed_predecessor;
                        if( entry.test_string() ) {
                            allowed_predecessor.first = entry.get_string();
                            allowed_predecessor.second = ot_match_type::contains;
                        } else {
                            JsonObject jo = entry.get_object();
                            allowed_predecessor.first = jo.get_string( "om_terrain" );
                            allowed_predecessor.second = jo.get_enum_value<ot_match_type>( "om_terrain_match_type",
                                                         ot_match_type::contains );
                        }
                        allowed_predecessors.insert( allowed_predecessor );
                    }
                }

                bool test( const mapgendata &dat ) const {
                    const std::vector<oter_id> predecessors = dat.get_predecessors();
                    for( const std::pair<std::string, ot_match_type> &allowed_predecessor : allowed_predecessors ) {
                        for( const oter_id &predecessor : predecessors ) {
                            if( is_ot_match( allowed_predecessor.first, predecessor, allowed_predecessor.second ) ) {
                                return true;
                            }
                        }
                    }
                    return allowed_predecessors.empty();
                }
        };
        class correct_z_level_check
        {
            private:
                std::unordered_set<int> required_z;
            public:
                explicit correct_z_level_check( const JsonArray &jarr ) {
                    for( const JsonValue entry : jarr ) {
                        required_z.emplace( entry.get_int() );
                    }
                }
                bool test( const mapgendata &dat ) const {
                    return required_z.empty() || required_z.count( dat.zlevel() ) == 1;
                }
        };

    public:
        weighted_dbl_or_var_list<mapgen_value<nested_mapgen_id>> entries;
        weighted_dbl_or_var_list<mapgen_value<nested_mapgen_id>> else_entries;
        neighbor_oter_check neighbor_oters;
        neighbor_join_check neighbor_joins;
        neighbor_flag_check neighbor_flags;
        neighbor_flag_any_check neighbor_flags_any;
        predecessor_oter_check predecessors;
        correct_z_level_check correct_z_level;
        jmapgen_nested( const JsonObject &jsi, std::string_view/*context*/ )
            : neighbor_oters( jsi.get_object( "neighbors" ) )
            , neighbor_joins( jsi.get_object( "joins" ) )
            , neighbor_flags( jsi.get_object( "flags" ) )
            , neighbor_flags_any( jsi.get_object( "flags_any" ) )
            , predecessors( jsi.get_array( "predecessors" ) )
            , correct_z_level( jsi.get_array( "check_z" ) ) {
            if( jsi.has_member( "chunks" ) ) {
                entries.load( jsi.get_member( "chunks" ), 100 );
            }
            if( jsi.has_member( "else_chunks" ) ) {
                else_entries.load( jsi.get_member( "else_chunks" ), 100 );
            }
        }
        mapgen_phase phase() const override {
            return mapgen_phase::nested_mapgen;
        }
        void merge_parameters_into( mapgen_parameters &params,
                                    const std::string &outer_context ) const override {
            auto merge_from = [&]( const mapgen_value<nested_mapgen_id> &val ) {
                for( const nested_mapgen_id &id : val.all_possible_results( params ) ) {
                    if( id.is_null() ) {
                        return;
                    }
                    const auto iter = nested_mapgens.find( id );
                    if( iter == nested_mapgens.end() ) {
                        debugmsg( "Unknown nested mapgen function id '%s'", id.str() );
                        return;
                    }
                    using Obj = weighted_object<int, std::shared_ptr<mapgen_function_json_nested>>;
                    for( const Obj &nested : iter->second.funcs() ) {
                        nested.obj->merge_non_nest_parameters_into( params, outer_context );
                    }
                }
            };
            for( const weighted_object<dbl_or_var, mapgen_value<nested_mapgen_id>> &name : entries ) {
                merge_from( name.obj );
            }
            for( const weighted_object<dbl_or_var, mapgen_value<nested_mapgen_id>> &name : else_entries ) {
                merge_from( name.obj );
            }
        }
        const weighted_dbl_or_var_list<mapgen_value<nested_mapgen_id>> &get_entries(
        const mapgendata &dat ) const {
            if( neighbor_oters.test( dat ) && neighbor_joins.test( dat ) && neighbor_flags.test( dat ) &&
                neighbor_flags_any.test( dat ) && predecessors.test( dat ) && correct_z_level.test( dat ) ) {
                return entries;
            } else {
                return else_entries;
            }
        }
        void apply( const mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z,
                    const std::string &context ) const override {
            const mapgen_value<nested_mapgen_id> *val = get_entries( dat ).pick();
            if( val == nullptr ) {
                return;
            }

            const nested_mapgen_id res = val->get( dat );
            if( res.is_empty() || res.is_null() ) {
                // This will be common when neighbors.test(...) is false, since else_entires is
                // often empty.
                return;
            }
            const auto iter = nested_mapgens.find( res );
            if( iter == nested_mapgens.end() ) {
                debugmsg( "Unknown nested mapgen function id %s", res.str() );
                return;
            }

            // A second roll? Let's allow it for now
            const auto &ptr = iter->second.funcs().pick();
            if( ptr == nullptr ) {
                return;
            }

            ( *ptr )->nest( dat, tripoint_rel_ms( x.get(), y.get(), z.get() ), context );
        }
        void check( const std::string &oter_name, const mapgen_parameters &parameters,
                    const jmapgen_int &x, const jmapgen_int &y, const jmapgen_int &z
                  ) const override {
            for( const weighted_object<dbl_or_var, mapgen_value<nested_mapgen_id>> &p : entries ) {
                p.obj.check( oter_name, parameters );
            }
            for( const weighted_object<dbl_or_var, mapgen_value<nested_mapgen_id>> &p : entries ) {
                p.obj.check( oter_name, parameters );
            }
            neighbor_joins.check( oter_name, parameters );

            // Check whether any of the nests can attempt to place stuff out of
            // bounds
            std::unordered_map<point_rel_ms, nested_mapgen_id> nest_placement_coords;
            auto add_coords_from = [&]( const mapgen_value<nested_mapgen_id> &nest_id_val ) {
                for( const nested_mapgen_id &nest_id :
                     nest_id_val.all_possible_results( parameters ) ) {
                    if( nest_id.is_null() ) {
                        continue;
                    }
                    for( const point_rel_ms &p : nest_id->all_placement_coords() ) {
                        nest_placement_coords.emplace( p, nest_id );
                    }
                }
            };
            for( const weighted_object<dbl_or_var, mapgen_value<nested_mapgen_id>> &p : entries ) {
                add_coords_from( p.obj );
            }
            for( const weighted_object<dbl_or_var, mapgen_value<nested_mapgen_id>> &p : else_entries ) {
                add_coords_from( p.obj );
            }

            point max_relative;
            nested_mapgen_id offending_nest_x;
            nested_mapgen_id offending_nest_y;

            for( const std::pair<const point_rel_ms, nested_mapgen_id> &p : nest_placement_coords ) {
                if( p.first.x() > max_relative.x ) {
                    max_relative.x = p.first.x();
                    offending_nest_x = p.second;
                }
                if( p.first.y() > max_relative.y ) {
                    max_relative.y = p.first.y();
                    offending_nest_y = p.second;
                }
            }

            static constexpr int omt_size = SEEX * 2;
            point max = point( x.valmax % omt_size, y.valmax % omt_size ) + max_relative;

            if( max.x >= omt_size ) {
                debugmsg( "nest %s within %s can place something at x = %d, which is in a "
                          "different OMT from the smallest nest origin (%d, %d), and could lead to "
                          "an out of bounds placement",
                          offending_nest_x.str(), oter_name, x.valmax + max_relative.x, x.val,
                          y.val );
            }

            if( max.y >= omt_size ) {
                debugmsg( "nest %s within %s can place something at y = %d, which is in a "
                          "different OMT from the smallest nest origin (%d, %d), and could lead to "
                          "an out of bounds placement",
                          offending_nest_y.str(), oter_name, y.valmax + max_relative.y, x.val,
                          y.val );
            }
            if( z.val != z.valmax ) {
                debugmsg( "nest %s within %s defines a range for Z level placement, which isn't"
                          "permitted.  Range: %d - %d.",
                          offending_nest_x.str(), oter_name, z.val, z.valmax );
            }
        }
        ret_val<void> has_vehicle_collision( const mapgendata &dat,
                                             const tripoint_rel_ms &p ) const override {
            const weighted_dbl_or_var_list<mapgen_value<nested_mapgen_id>> &selected_entries =
                        get_entries( dat );
            if( selected_entries.empty() ) {
                return ret_val<void>::make_success();
            }

            for( const auto &entry : selected_entries ) {
                nested_mapgen_id id = entry.obj.get( dat );
                if( id.is_null() ) {
                    continue;
                }
                const auto iter = nested_mapgens.find( id );
                if( iter == nested_mapgens.end() ) {
                    return ret_val<void>::make_success();
                }
                for( const auto &nest : iter->second.funcs() ) {
                    const ret_val<void> has_vehicle_collision = nest.obj->has_vehicle_collision( dat, p );
                    if( !has_vehicle_collision.success() ) {
                        return has_vehicle_collision;
                    }
                }
            }

            return ret_val<void>::make_success();
        }
};

jmapgen_objects::jmapgen_objects( const tripoint_rel_ms &offset, const point_rel_ms &mapsize,
                                  const point_rel_ms &tot_size )
    : m_offset( offset )
    , mapgensize( mapsize )
    , total_size( tot_size )
{}

bool jmapgen_objects::check_bounds( const jmapgen_place &place, const JsonObject &jso )
{
    return common_check_bounds( place.x, place.y, place.z, mapgensize, jso );
}

void jmapgen_objects::add( const jmapgen_place &place,
                           const shared_ptr_fast<const jmapgen_piece> &piece )
{
    objects.emplace_back( place, piece );
}

template<typename PieceType>
void jmapgen_objects::load_objects( const JsonArray &parray, std::string_view context )
{
    for( JsonObject jsi : parray ) {
        jmapgen_place where( jsi );
        where.offset( m_offset );

        if( check_bounds( where, jsi ) ) {
            add( where, make_shared_fast<PieceType>( jsi, context ) );
        } else {
            jsi.allow_omitted_members();
        }
    }
}

template<>
void jmapgen_objects::load_objects<jmapgen_loot>(
    const JsonArray &parray, std::string_view/*context*/ )
{
    for( JsonObject jsi : parray ) {
        jmapgen_place where( jsi );
        where.offset( m_offset );

        if( !check_bounds( where, jsi ) ) {
            jsi.allow_omitted_members();
            continue;
        }

        auto loot = make_shared_fast<jmapgen_loot>( jsi );
        // spawn rates < 1 are handled in item_group
        const float rate = std::max( get_option<float>( "ITEM_SPAWNRATE" ), 1.0f );

        if( where.repeat.valmax != 1 ) {
            // if loot can repeat scale according to rate
            where.repeat.val = std::max( static_cast<int>( where.repeat.val * rate ), 1 );
            where.repeat.valmax = std::max( static_cast<int>( where.repeat.valmax * rate ), 1 );

        } else if( loot->chance != 100 ) {
            // otherwise except where chance is 100% scale probability
            loot->chance = std::max( std::min( static_cast<int>( loot->chance * rate ), 100 ), 1 );
        }

        add( where, loot );
    }
}

template<typename PieceType>
void jmapgen_objects::load_objects( const JsonObject &jsi, const std::string &member_name,
                                    const std::string &context )
{
    if( !jsi.has_member( member_name ) ) {
        return;
    }
    load_objects<PieceType>( jsi.get_array( member_name ), member_name + " in " + context );
}

template<typename PieceType>
void load_place_mapings( const JsonObject &jobj, mapgen_palette::placing_map::mapped_type &vect,
                         const std::string &context )
{
    vect.push_back( make_shared_fast<PieceType>( jobj, context ) );
}

/*
This is the default load function for mapgen pieces that only support loading from a json object,
not from a simple string.
Most non-trivial mapgen pieces (like item spawn which contains at least the item group and chance)
are like this. Other pieces (trap, furniture ...) can be loaded from a single string and have
an overload below.
The mapgen piece is loaded from the member of the json object named key.
*/
template<typename PieceType>
void load_place_mapings( const JsonValue &value, mapgen_palette::placing_map::mapped_type &vect,
                         const std::string &context )
{
    if( value.test_object() ) {
        load_place_mapings<PieceType>( value.get_object(), vect, context );
    } else {
        for( JsonObject jo : value.get_array() ) {
            load_place_mapings<PieceType>( jo, vect, context );
        }
    }
}

/*
This function allows loading the mapgen pieces from a single string, *or* a json object.
*/
template<typename PieceType>
void load_place_mapings_string(
    const JsonValue &value, mapgen_palette::placing_map::mapped_type &vect,
    const std::string &context )
{
    if( value.test_string() || value.test_object() ) {
        try {
            vect.push_back( make_shared_fast<PieceType>( value ) );
        } catch( const std::runtime_error &err ) {
            // Using the json object here adds nice formatting and context information
            value.throw_error( err.what() );
        }
    } else {
        for( const JsonValue entry : value.get_array() ) {
            if( entry.test_string() ) {
                try {
                    vect.push_back( make_shared_fast<PieceType>( entry ) );
                } catch( const std::runtime_error &err ) {
                    // Using the json object here adds nice formatting and context information
                    entry.throw_error( err.what() );
                }
            } else {
                load_place_mapings<PieceType>( entry.get_object(), vect, context );
            }
        }
    }
}
/*
This function is like load_place_mapings_string, except if the input is an array it will create an
instance of jmapgen_alternatively which will chose the mapgen piece to apply to the map randomly.
Use this with terrain or traps or other things that can not be applied twice to the same place.
*/
template<typename PieceType>
void load_place_mapings_alternatively(
    const JsonValue &value, mapgen_palette::placing_map::mapped_type &vect,
    const std::string &context )
{
    if( !value.test_array() ) {
        load_place_mapings_string<PieceType>( value, vect, context );
    } else {
        auto alter = make_shared_fast< jmapgen_alternatively<PieceType> >();
        for( const JsonValue entry : value.get_array() ) {
            if( entry.test_string() ) {
                try {
                    alter->alternatives.emplace_back( entry );
                } catch( const std::runtime_error &err ) {
                    // Using the json object here adds nice formatting and context information
                    entry.throw_error( err.what() );
                }
            } else if( entry.test_object() ) {
                JsonObject jsi = entry.get_object();
                alter->alternatives.emplace_back( jsi, context );
            } else if( entry.test_array() ) {
                // If this is an array, it means it is an entry followed by a desired total count of instances.
                JsonArray piece_and_count_jarr = entry.get_array();
                if( piece_and_count_jarr.size() != 2 ) {
                    piece_and_count_jarr.throw_error( "Array must have exactly two entries: the object, then the count." );
                }

                // Test if this is a string or object, and then just emplace it.
                if( piece_and_count_jarr.test_string() || piece_and_count_jarr.test_object() ) {
                    try {
                        alter->alternatives.emplace_back( piece_and_count_jarr.next_value() );
                    } catch( const std::runtime_error &err ) {
                        piece_and_count_jarr.throw_error( err.what() );
                    }
                } else {
                    piece_and_count_jarr.throw_error( "First entry must be a string or object." );
                }

                if( piece_and_count_jarr.test_int() ) {
                    // We already emplaced the first instance, so do one less.
                    int repeat = std::max( 0, piece_and_count_jarr.next_int() - 1 );
                    PieceType piece_to_repeat = alter->alternatives.back();
                    for( int i = 0; i < repeat; i++ ) {
                        alter->alternatives.emplace_back( piece_to_repeat );
                    }
                } else {
                    piece_and_count_jarr.throw_error( "Second entry must be an integer." );
                }
            }
        }
        vect.push_back( alter );
    }
}

template<>
void load_place_mapings<jmapgen_trap>(
    const JsonValue &value, mapgen_palette::placing_map::mapped_type &vect,
    const std::string &context )
{
    load_place_mapings_alternatively<jmapgen_trap>( value, vect, context );
}

template<>
void load_place_mapings<jmapgen_furniture>( const JsonValue &value,
        mapgen_palette::placing_map::mapped_type &vect, const std::string &context )
{
    load_place_mapings_alternatively<jmapgen_furniture>( value, vect, context );
}

template<>
void load_place_mapings<jmapgen_terrain>( const JsonValue &value,
        mapgen_palette::placing_map::mapped_type &vect, const std::string &context )
{
    load_place_mapings_alternatively<jmapgen_terrain>( value, vect, context );
}

template<typename PieceType>
void mapgen_palette::load_place_mapings( const JsonObject &jo, const std::string &member_name,
        placing_map &format_placings, const std::string &context )
{
    if( jo.has_object( "mapping" ) ) {
        for( const JsonMember member : jo.get_object( "mapping" ) ) {
            const map_key key( member );
            JsonObject sub = member.get_object();
            sub.allow_omitted_members();
            if( !sub.has_member( member_name ) ) {
                continue;
            }
            auto &vect = format_placings[ key ];
            // NOLINTNEXTLINE(cata-translate-string-literal)
            std::string this_context = string_format( "%s in mapping in %s", member_name, context );
            ::load_place_mapings<PieceType>( sub.get_member( member_name ), vect, this_context );
        }
    }
    if( !jo.has_object( member_name ) ) {
        return;
    }
    for( const JsonMember member : jo.get_object( member_name ) ) {
        const map_key key( member );
        auto &vect = format_placings[ key ];
        std::string this_context =
            // NOLINTNEXTLINE(cata-translate-string-literal)
            string_format( "%s %s in %s", member_name, member.name(), context );
        ::load_place_mapings<PieceType>( member, vect, this_context );
    }
}

static std::map<palette_id, mapgen_palette> palettes;

template<>
const mapgen_palette &string_id<mapgen_palette>::obj() const
{
    auto it = palettes.find( *this );
    if( it == palettes.end() ) {
        static const mapgen_palette null_palette;
        return null_palette;
    }
    return it->second;
}

void debug_palettes::debug_view_all_palettes()
{
    uilist palette_list;
    palette_list.title = _( "Viewing all palettes" );
    // awfulness. we need a vector so we can track the return value
    std::vector<palette_id> all_palettes;
    all_palettes.reserve( palettes.size() );
    for( auto &pair : palettes ) {
        all_palettes.push_back( pair.first );
    }
    // sort them alphabetically before display, why not.
    std::sort( all_palettes.begin(), all_palettes.end(), [](
    const palette_id & lhs, const palette_id & rhs ) {
        return localized_compare( lhs.str(), rhs.str() );
    } );
    for( palette_id &some_palette : all_palettes ) {
        palette_list.addentry( some_palette.str() );
    }
    palette_list.query();

    const int index_num = palette_list.ret;
    if( index_num < 0 || static_cast<size_t>( index_num ) >= all_palettes.size() ) {
        return; // user cancelled
    }

    const mapgen_palette &actual_palette = mapgen_palette::get( all_palettes[index_num] );
    // why do we put *these* in a vector? Solely so we can sort alphabetically.
    std::vector<map_key> symbols_list;
    for( const auto &placing : actual_palette.format_placings ) {
        symbols_list.push_back( placing.first );
    }

    std::sort( symbols_list.begin(), symbols_list.end(), [](
    const map_key & lhs, const map_key & rhs ) {
        return localized_compare( lhs.str, rhs.str );
    } );

    uilist palette_symbols;
    //FIXME: extract for translation!
    palette_symbols.title = string_format( "Viewing palette '%s'  ",
                                           all_palettes[index_num].c_str() );

    for( const map_key &something : symbols_list ) {
        // First three arguments are dummy parameters, uilist is just a means of display. our return value is discarded.
        // FIXME: Find a way to map the symbol to the underlying ID of what will be placed, put it into the second column instead of empty string
        palette_symbols.addentry_col( 0, true, input_event(), something.str, "" );
    }

    palette_symbols.query();

    // little trick, return to palette listing after second uilist unless they quit out of the first uilist
    debug_palettes::debug_view_all_palettes();

}

template<>
bool string_id<mapgen_palette>::is_valid() const
{
    return palettes.find( *this ) != palettes.end();
}

void mapgen_palette::check() const
{
    std::string context = "palette " + id.str();
    jmapgen_int fake_coord( -1 );
    mapgen_parameters no_parameters;
    for( const std::pair<const std::string, mapgen_parameter> &param : parameters.map ) {
        std::string this_context = string_format( "parameter %s in %s", param.first, context );
        param.second.check( no_parameters, this_context );
    }
    for( const std::pair<const map_key, std::vector<shared_ptr_fast<const jmapgen_piece>>> &p :
         format_placings ) {
        for( const shared_ptr_fast<const jmapgen_piece> &j : p.second ) {
            j->check( context, parameters, fake_coord, fake_coord, fake_coord );
        }
    }
}

mapgen_palette mapgen_palette::load_temp( const JsonObject &jo, std::string_view src,
        const std::string &context )
{
    return load_internal( jo, src, context, false, true );
}

void mapgen_palette::load( const JsonObject &jo, std::string_view src )
{
    mapgen_palette ret = load_internal( jo, src, "", true, false );
    if( ret.id.is_empty() ) {
        jo.throw_error( "Named palette needs an id" );
    }

    palettes[ ret.id ] = ret;
}

const mapgen_palette &mapgen_palette::get( const palette_id &id )
{
    const auto iter = palettes.find( id );
    if( iter != palettes.end() ) {
        return iter->second;
    }

    debugmsg( "Requested palette with unknown id %s", id.c_str() );
    static mapgen_palette dummy;
    return dummy;
}

void mapgen_palette::check_definitions()
{
    for( const auto &p : palettes ) {
        p.second.check();
    }
}

void mapgen_palette::reset()
{
    palettes.clear();
}

void mapgen_palette::add( const mapgen_value<std::string> &rh, const add_palette_context &context )
{
    std::vector<std::string> possible_values =
        rh.all_possible_results( *context.current_parameters );
    if( possible_values.empty() ) {
        if( const std::string *param_name = rh.get_name_if_parameter() ) {
            debugmsg( "Parameter %s used for palette id in %s but has no possible values",
                      *param_name, context.context );
        } else {
            debugmsg( "Mapgen value for used for palette id in %s has no possible values",
                      context.context );
        }
    } else if( possible_values.size() == 1 ) {
        add( palette_id( possible_values.front() ), context );
    } else {
        std::string param_name;
        if( const std::string *name = rh.get_name_if_parameter() ) {
            param_name = *name;
        } else {
            const auto param_it =
                context.top_level_parameters->add_unique_parameter(
                    "palette_choice_", rh, cata_variant_type::palette_id,
                    mapgen_parameter_scope::overmap_special );
            param_name = param_it->first;
        }
        add_palette_context context_with_extra_constraint( context );
        for( const std::string &value : possible_values ) {
            palette_id val_id( value );
            context_with_extra_constraint.constraints.emplace_back( param_name, val_id );
            add( val_id, context_with_extra_constraint );
            context_with_extra_constraint.constraints.pop_back();
        }
    }
}

void mapgen_palette::add( const palette_id &rh, const add_palette_context &context )
{
    add( get( rh ), context );
}

void mapgen_palette::add( const mapgen_palette &rh, const add_palette_context &context )
{
    std::string actual_context = id.is_empty() ? context.context : "palette " + id.str();

    if( !rh.id.is_empty() ) {
        const std::vector<palette_id> &ancestors = context.ancestors;
        auto loop_start = std::find( ancestors.begin(), ancestors.end(), rh.id );
        if( loop_start != ancestors.end() ) {
            std::string loop_ids = enumerate_as_string( loop_start, ancestors.end(),
            []( const palette_id & i ) {
                return i.str();
            }, enumeration_conjunction::arrow );
            debugmsg( "loop in palette references: %s", loop_ids );
            return;
        }
    }
    add_palette_context new_context = context;
    new_context.ancestors.push_back( rh.id );
    new_context.current_parameters = &rh.parameters;

    for( const mapgen_value<std::string> &recursive_palette : rh.palettes_used ) {
        add( recursive_palette, new_context );
    }
    for( const auto &placing : rh.format_placings ) {
        const std::vector<mapgen_constraint<palette_id>> &constraints = context.constraints;
        std::vector<shared_ptr_fast<const jmapgen_piece>> constrained_placings = placing.second;
        if( !constraints.empty() ) {
            for( shared_ptr_fast<const jmapgen_piece> &piece : constrained_placings ) {
                piece = make_shared_fast<jmapgen_constrained<palette_id>>(
                            std::move( piece ), constraints );
            }
        }
        std::vector<shared_ptr_fast<const jmapgen_piece>> &these_placings =
                    format_placings[placing.first];
        these_placings.insert( these_placings.end(),
                               constrained_placings.begin(), constrained_placings.end() );
    }
    for( const map_key &placing : rh.keys_with_terrain ) {
        keys_with_terrain.insert( placing );
    }
    parameters.check_and_merge( rh.parameters, actual_context );
}

mapgen_palette mapgen_palette::load_internal( const JsonObject &jo, std::string_view src,
        const std::string &context, bool require_id, bool allow_recur )
{
    mapgen_palette new_pal;
    bool extending = src != "dda" && jo.has_bool( "extending" ) && jo.get_bool( "extending" );
    require_id |= extending;
    mapgen_palette::placing_map &format_placings = new_pal.format_placings;
    auto &keys_with_terrain = new_pal.keys_with_terrain;
    if( require_id ) {
        new_pal.id = palette_id( jo.get_string( "id" ) );
        const auto iter = palettes.find( new_pal.id );
        extending &= iter != palettes.end();
    }

    jo.read( "parameters", new_pal.parameters.map );

    if( jo.has_array( "palettes" ) ) {
        jo.read( "palettes", new_pal.palettes_used );
        if( allow_recur ) {
            // allow_recur means that it's safe to assume all the palettes have
            // been defined and we can inline now.  Otherwise we just leave the
            // list in our palettes_used array and it will be consumed
            // recursively by calls to add which add this palette.
            add_palette_context add_context{ context, &new_pal.parameters };
            for( auto &p : new_pal.palettes_used ) {
                new_pal.add( p, add_context );
            }
            new_pal.palettes_used.clear();
        }
    }

    if( extending ) {
        add_palette_context add_context{ context, &new_pal.parameters };
        new_pal.add( new_pal.id, add_context );
    }

    // mandatory: every character in rows must have matching entry, unless fill_ter is set
    // "terrain": { "a": "t_grass", "b": "t_lava" }.  To help enforce this we
    // keep track of everything in the "terrain" object
    if( jo.has_member( "terrain" ) ) {
        for( const JsonMember member : jo.get_object( "terrain" ) ) {
            keys_with_terrain.insert( map_key( member ) );
        }
    }

    std::string c = "palette " + new_pal.id.str();
    new_pal.load_place_mapings<jmapgen_terrain>( jo, "terrain", format_placings, c );
    new_pal.load_place_mapings<jmapgen_furniture>( jo, "furniture", format_placings, c );
    new_pal.load_place_mapings<jmapgen_field>( jo, "fields", format_placings, c );
    new_pal.load_place_mapings<jmapgen_npc>( jo, "npcs", format_placings, c );
    new_pal.load_place_mapings<jmapgen_sign>( jo, "signs", format_placings, c );
    new_pal.load_place_mapings<jmapgen_vending_machine>( jo, "vendingmachines", format_placings, c );
    new_pal.load_place_mapings<jmapgen_toilet>( jo, "toilets", format_placings, c );
    new_pal.load_place_mapings<jmapgen_gaspump>( jo, "gaspumps", format_placings, c );
    new_pal.load_place_mapings<jmapgen_item_group>( jo, "items", format_placings, c );
    new_pal.load_place_mapings<jmapgen_monster_group>( jo, "monsters", format_placings, c );
    new_pal.load_place_mapings<jmapgen_vehicle>( jo, "vehicles", format_placings, c );
    // json member name is not optimal, it should be plural like all the others above, but that conflicts
    // with the items entry which refers to item groups.
    new_pal.load_place_mapings<jmapgen_spawn_item>( jo, "item", format_placings, c );
    new_pal.load_place_mapings<jmapgen_remove_vehicles>( jo, "remove_vehicles", format_placings, c );
    new_pal.load_place_mapings<jmapgen_remove_all>( jo, "remove_all", format_placings, c );
    new_pal.load_place_mapings<jmapgen_trap>( jo, "traps", format_placings, c );
    new_pal.load_place_mapings<jmapgen_monster>( jo, "monster", format_placings, c );
    new_pal.load_place_mapings<jmapgen_make_rubble>( jo, "rubble", format_placings, c );
    new_pal.load_place_mapings<jmapgen_computer>( jo, "computers", format_placings, c );
    new_pal.load_place_mapings<jmapgen_sealed_item>( jo, "sealed_item", format_placings, c );
    new_pal.load_place_mapings<jmapgen_nested>( jo, "nested", format_placings, c );
    new_pal.load_place_mapings<jmapgen_liquid_item>( jo, "liquids", format_placings, c );
    new_pal.load_place_mapings<jmapgen_corpse>( jo, "corpses", format_placings, c );
    new_pal.load_place_mapings<jmapgen_graffiti>( jo, "graffiti", format_placings, c );
    new_pal.load_place_mapings<jmapgen_zone>( jo, "zones", format_placings, c );
    new_pal.load_place_mapings<jmapgen_ter_furn_transform>( jo, "ter_furn_transforms",
            format_placings, c );

    for( mapgen_palette::placing_map::value_type &p : format_placings ) {
        p.second.erase(
            std::remove_if(
                p.second.begin(), p.second.end(),
        []( const shared_ptr_fast<const jmapgen_piece> &placing ) {
            return placing->is_nop();
        } ), p.second.end() );
    }
    return new_pal;
}

mapgen_palette::add_palette_context::add_palette_context(
    const std::string &ctx, mapgen_parameters *params )
    : context( ctx )
    , top_level_parameters( params )
    , current_parameters( params )
{}

bool mapgen_function_json::setup_internal( const JsonObject &jo )
{
    // Just to make sure no one does anything stupid
    if( jo.has_member( "mapgensize" ) ) {
        jo.throw_error( "\"mapgensize\" only allowed for nested mapgen" );
    }

    if( jo.has_member( "fill_ter" ) ) {
        fill_ter = cata::make_value<mapgen_value<ter_id>>( jo.get_member( "fill_ter" ) );
    }

    if( jo.has_member( "rotation" ) ) {
        rotation = jmapgen_int( jo, "rotation" );
    }

    if( jo.has_member( "predecessor_mapgen" ) ) {
        predecessor_mapgen = oter_str_id( jo.get_string( "predecessor_mapgen" ) ).id();
    } else {
        predecessor_mapgen = oter_str_id::NULL_ID();
    }

    jo.read( "fallback_predecessor_mapgen", fallback_predecessor_mapgen_ );

    return fill_ter || predecessor_mapgen != oter_str_id::NULL_ID() ||
           fallback_predecessor_mapgen_ != oter_str_id::NULL_ID();
}

bool mapgen_function_json_nested::setup_internal( const JsonObject &jo )
{
    // Mandatory - nested mapgen must be explicitly sized
    if( jo.has_array( "mapgensize" ) ) {
        JsonArray jarr = jo.get_array( "mapgensize" );
        mapgensize = point_rel_ms( jarr.get_int( 0 ), jarr.get_int( 1 ) );
        if( mapgensize.x() == 0 || mapgensize.x() != mapgensize.y() ) {
            // Non-square sizes not implemented yet
            jo.throw_error( "\"mapgensize\" must be an array of two identical, positive numbers" );
        }
        total_size = mapgensize;
    } else {
        jo.throw_error( "Nested mapgen must have \"mapgensize\" set" );
    }

    if( jo.has_member( "rotation" ) ) {
        rotation = jmapgen_int( jo, "rotation" );
    }

    // Nested mapgen is always halal because it can assume underlying map is.
    return true;
}

void mapgen_function_json::setup()
{
    setup_common();
}

void mapgen_function_json_nested::setup()
{
    setup_common();
}

void update_mapgen_function_json::setup()
{
    setup_common();
}

void mapgen_function_json::finalize_parameters()
{
    finalize_parameters_common();
}

void mapgen_function_json_nested::finalize_parameters()
{
    finalize_parameters_common();
}

void update_mapgen_function_json::finalize_parameters()
{
    finalize_parameters_common();
}

struct phase_comparator {
    mapgen_phase get_phase( mapgen_phase p ) const {
        return p;
    }

    mapgen_phase get_phase( const jmapgen_setmap &s ) const {
        return s.phase();
    }

    template<typename PiecePtr>
    mapgen_phase get_phase( const std::pair<jmapgen_place, PiecePtr> &p ) const {
        return p.second->phase();
    }

    template<typename T, typename U>
    bool operator()( const T &l, const U &r ) const {
        return get_phase( l ) < get_phase( r );
    }
};

static const phase_comparator compare_phases{};

/*
 * Parse json, pre-calculating values for stuff, then cheerfully throw json away. Faster than regular mapf, in theory
 */
void mapgen_function_json_base::setup_common()
{
    if( is_ready ) {
        return;
    }
    JsonObject jo = jsobj;
    mapgen_defer::defer = false;
    if( !setup_common( jo ) ) {
        jo.throw_error( "format: no terrain map" );
    }
    if( mapgen_defer::defer ) {
        mapgen_defer::jsi.throw_error_at( mapgen_defer::member, mapgen_defer::message );
    } else {
        mapgen_defer::jsi = JsonObject();
    }
}

bool mapgen_function_json_base::setup_common( const JsonObject &jo )
{
    bool fallback_terrain_exists = setup_internal( jo );
    JsonArray parray;
    JsonArray sparray;
    JsonObject pjo;

    jo.read( "flags", flags_ );

    // mandatory: mapgensize rows of mapgensize character lines, each of which must have a
    // matching key in "terrain", unless fill_ter is set
    // "rows:" [ "aaaajustlikeinmapgen.cpp", "this.must!be!exactly.24!", "and_must_match_terrain_", .... ]
    point_rel_ms expected_dim = mapgensize + m_offset.xy();
    cata_assert( expected_dim.x() >= 0 );
    cata_assert( expected_dim.y() >= 0 );
    const std::string default_row( expected_dim.x(), ' ' );
    const bool default_rows = !jo.has_array( "rows" );
    if( !default_rows ) {
        parray = jo.get_array( "rows" );
        if( static_cast<int>( parray.size() ) < expected_dim.y() ) {
            parray.throw_error( string_format( "format: rows: must have at least %d rows, not %d",
                                               expected_dim.y(), parray.size() ) );
        }
        if( static_cast<int>( parray.size() ) != total_size.y() ) {
            parray.throw_error(
                string_format( "format: rows: must have %d rows, not %d; check mapgensize if applicable",
                               total_size.y(), parray.size() ) );
        }
    }

    // just like mapf::basic_bind("stuff",blargle("foo", etc) ), only json input and faster when applying
    mapgen_palette palette = mapgen_palette::load_temp( jo, "dda", context_ );
    auto &keys_with_terrain = palette.keys_with_terrain;
    mapgen_palette::placing_map &format_placings = palette.format_placings;

    if( palette.keys_with_terrain.empty() && !fallback_terrain_exists ) {
        return false;
    }

    parameters = palette.get_parameters();

    for( int c = m_offset.y(); c < expected_dim.y(); c++ ) {
        const std::string row = default_rows ? default_row : parray.get_string( c );
        static std::vector<std::string_view> row_keys;
        row_keys.clear();
        row_keys.reserve( total_size.x() );
        utf8_display_split_into( row, row_keys );
        if( row_keys.size() < static_cast<size_t>( expected_dim.x() ) ) {
            cata_assert( !default_rows );
            parray.throw_error(
                string_format( "format: row %d must have at least %d columns, not %d",
                               c + 1, expected_dim.x(), row_keys.size() ) );
        }
        if( row_keys.size() != static_cast<size_t>( total_size.x() ) ) {
            cata_assert( !default_rows );
            parray.throw_error(
                string_format( "format: row %d must have %d columns, not %d; check mapgensize if applicable",
                               c + 1, total_size.x(), row_keys.size() ) );
        }
        for( int i = m_offset.x(); i < expected_dim.x(); i++ ) {
            const tripoint_rel_ms p = tripoint_rel_ms( i, c, 0 ) - m_offset;
            const map_key key{ std::string( row_keys[i] ) };
            const auto iter_ter = keys_with_terrain.find( key );
            const auto fpi = format_placings.find( key );

            const bool has_terrain = iter_ter != keys_with_terrain.end();
            const bool has_placing = fpi != format_placings.end();

            if( !has_terrain && !fallback_terrain_exists ) {
                cata_assert( !default_rows );
                parray.string_error( c, i + 1,
                                     string_format( "format: rows: row %d column %d: '%s' is not in 'terrain',"
                                                    "and no 'fill_ter', 'predecessor_mapgen' or 'fallback_predecessor_mapgen' is set!",
                                                    c + 1, i + 1, key.str ) );
            }
            if( !has_terrain && !has_placing && key.str != " " && key.str != "." ) {
                try {
                    cata_assert( !default_rows );
                    parray.string_error( c, i + 1,
                                         string_format( "format: rows: row %d column %d: '%s' has no terrain, furniture, or other definition",
                                                        c + 1, i + 1, key.str ) );
                } catch( const JsonError &e ) {
                    debugmsg( "(json-error)\n%s", e.what() );
                }
            }
            if( has_placing ) {
                jmapgen_place where( p );
                for( auto &what : fpi->second ) {
                    objects.add( where, what );
                }
            }
        }
    }
    fallback_terrain_exists = true;

    // TODO: Pointless check???
    if( !fallback_terrain_exists ) {
        jo.throw_error(
            "Need one of 'fill_ter', 'predecessor_mapgen', 'fallback_predecessor_mapgen', 'terrain' or a palette providing 'terrain'." );
        // TODO: write TFM.
    }

    if( jo.has_array( "set" ) ) {
        setup_setmap( jo.get_array( "set" ) );
    }

    objects.load_objects<jmapgen_remove_all>( jo, "place_remove_all", context_ );
    objects.load_objects<jmapgen_spawn_item>( jo, "place_item", context_ );
    objects.load_objects<jmapgen_field>( jo, "place_fields", context_ );
    objects.load_objects<jmapgen_npc>( jo, "place_npcs", context_ );
    objects.load_objects<jmapgen_sign>( jo, "place_signs", context_ );
    objects.load_objects<jmapgen_vending_machine>( jo, "place_vendingmachines", context_ );
    objects.load_objects<jmapgen_toilet>( jo, "place_toilets", context_ );
    objects.load_objects<jmapgen_liquid_item>( jo, "place_liquids", context_ );
    objects.load_objects<jmapgen_corpse>( jo, "place_corpses", context_ );
    objects.load_objects<jmapgen_gaspump>( jo, "place_gaspumps", context_ );
    objects.load_objects<jmapgen_item_group>( jo, "place_items", context_ );
    objects.load_objects<jmapgen_loot>( jo, "place_loot", context_ );
    objects.load_objects<jmapgen_monster_group>( jo, "place_monsters", context_ );
    objects.load_objects<jmapgen_vehicle>( jo, "place_vehicles", context_ );
    objects.load_objects<jmapgen_remove_vehicles>( jo, "remove_vehicles", context_ );
    objects.load_objects<jmapgen_remove_npcs>( jo, "remove_npcs", context_ );
    objects.load_objects<jmapgen_trap>( jo, "place_traps", context_ );
    objects.load_objects<jmapgen_furniture>( jo, "place_furniture", context_ );
    objects.load_objects<jmapgen_terrain>( jo, "place_terrain", context_ );
    objects.load_objects<jmapgen_monster>( jo, "place_monster", context_ );
    objects.load_objects<jmapgen_make_rubble>( jo, "place_rubble", context_ );
    objects.load_objects<jmapgen_computer>( jo, "place_computers", context_ );
    objects.load_objects<jmapgen_nested>( jo, "place_nested", context_ );
    objects.load_objects<jmapgen_graffiti>( jo, "place_graffiti", context_ );
    objects.load_objects<jmapgen_zone>( jo, "place_zones", context_ );
    objects.load_objects<jmapgen_ter_furn_transform>( jo, "place_ter_furn_transforms", context_ );
    objects.load_objects<jmapgen_variable>( jo, "place_variables", context_ );
    // Needs to be last as it affects other placed items
    objects.load_objects<jmapgen_faction>( jo, "faction_owner", context_ );

    objects.finalize();

    std::stable_sort( setmap_points.begin(), setmap_points.end(), compare_phases );

    if( !mapgen_defer::defer ) {
        is_ready = true; // skip setup attempts from any additional pointers
    }
    return true;
}

void mapgen_function_json::check() const
{
    check_common();

    if( predecessor_mapgen != oter_str_id::NULL_ID() && expects_predecessor() ) {
        debugmsg( "%s uses both predecessor_mapgen and expects_predecessor; these features are "
                  "incompatible", context_ );
    }
}

void mapgen_function_json::check_consistent_with( const oter_t &ter ) const
{
    bool requires_predecessor = ter.has_flag( oter_flags::requires_predecessor );
    if( expects_predecessor() && !requires_predecessor ) {
        debugmsg( "mapgen for oter_t %s expects a predecessor terrain but oter_type_t %s lacks the "
                  "REQUIRES_PREDECESSOR flag", ter.id.str(), ter.get_type_id().str() );
    }
}

void mapgen_function_json_nested::check() const
{
    check_common();
}

static bool check_furn( const furn_id &id, const std::string &context )
{
    const furn_t &furn = id.obj();
    if( furn.has_flag( ter_furn_flag::TFLAG_PLANT ) ) {
        debugmsg( "json mapgen for %s specifies furniture %s, which has flag "
                  "PLANT.  Such furniture must be specified in a \"sealed_item\" special.",
                  context, furn.id.str() );
        // Only report once per mapgen object, otherwise the reports are
        // very repetitive
        return true;
    }
    return false;
}

void mapgen_function_json_base::check_common() const
{
    if( static_cast<int>( flags_.test( jmapgen_flags::allow_terrain_under_other_data ) ) +
        static_cast<int>( flags_.test( jmapgen_flags::dismantle_all_before_placing_terrain ) ) +
        static_cast<int>( flags_.test( jmapgen_flags::erase_all_before_placing_terrain ) ) > 1 ) {
        debugmsg( "In %s, the mutually exclusive flags ERASE_ALL_BEFORE_PLACING_TERRAIN, "
                  "DISMANTLE_ALL_BEFORE_PLACING_TERRAIN and ALLOW_TERRAIN_UNDER_OTHER_DATA "
                  "cannot be used together.", context_ );
    }
    if( static_cast<int>( flags_.test( jmapgen_flags::allow_terrain_under_furniture ) ) +
        static_cast<int>( flags_.test( jmapgen_flags::dismantle_furniture_before_placing_terrain ) ) +
        static_cast<int>( flags_.test( jmapgen_flags::erase_furniture_before_placing_terrain ) ) > 1 ) {
        debugmsg( "In %s, the mutually exclusive flags ALLOW_TERRAIN_UNDER_FURNITURE, "
                  "DISMANTLE_FURNITURE_BEFORE_PLACING_TERRAIN and ERASE_FURNITURE_BEFORE_PLACING_TERRAIN "
                  "cannot be used together.", context_ );
    }
    if( static_cast<int>( flags_.test( jmapgen_flags::allow_terrain_under_trap ) ) +
        static_cast<int>( flags_.test( jmapgen_flags::dismantle_trap_before_placing_terrain ) ) +
        static_cast<int>( flags_.test( jmapgen_flags::erase_trap_before_placing_terrain ) ) > 1 ) {
        debugmsg( "In %s, the mutually exclusive flags ALLOW_TERRAIN_UNDER_TRAP, "
                  "DISMANTLE_TRAP_BEFORE_PLACING_TERRAIN and ERASE_TRAP_BEFORE_PLACING_TERRAIN "
                  "cannot be used together.", context_ );
    }
    if( static_cast<int>( flags_.test( jmapgen_flags::allow_terrain_under_items ) ) +
        static_cast<int>( flags_.test( jmapgen_flags::erase_items_before_placing_terrain ) ) > 1 ) {
        debugmsg( "In %s, the mutually exclusive flags "
                  "ALLOW_TERRAIN_UNDER_ITEMS and ERASE_ITEMS_BEFORE_PLACING_TERRAIN "
                  "cannot be used together.", context_ );
    }
    for( const jmapgen_setmap &setmap : setmap_points ) {
        if( setmap.op != JMAPGEN_SETMAP_FURN &&
            setmap.op != JMAPGEN_SETMAP_LINE_FURN &&
            setmap.op != JMAPGEN_SETMAP_SQUARE_FURN ) {
            continue;
        }
        furn_id id( setmap.val.get() );
        if( check_furn( id, context_ ) ) {
            return;
        }
    }

    objects.check( context_, parameters );
}

void mapgen_function_json_base::add_placement_coords_to( std::unordered_set<point_rel_ms> &result )
const
{
    objects.add_placement_coords_to( result );
}

std::unordered_set<point_rel_ms> nested_mapgen::all_placement_coords() const
{
    std::unordered_set<point_rel_ms> result;
    for( const weighted_object<int, std::shared_ptr<mapgen_function_json_nested>> &o : funcs_ ) {
        o.obj->add_placement_coords_to( result );
    }
    return result;
}

void nested_mapgen::add( const std::shared_ptr<mapgen_function_json_nested> &p, int weight )
{
    funcs_.add( p, weight );
}

void update_mapgen::add( std::unique_ptr<update_mapgen_function_json> &&p )
{
    funcs_.push_back( std::move( p ) );
}

void jmapgen_objects::finalize()
{
    std::stable_sort( objects.begin(), objects.end(), compare_phases );
    objects.shrink_to_fit();
}

void jmapgen_objects::check( const std::string &context, const mapgen_parameters &parameters ) const
{
    for( const jmapgen_obj &obj : objects ) {
        const jmapgen_place &where = obj.first;
        const jmapgen_piece &what = *obj.second;
        what.check( context, parameters, where.x, where.y, where.z );
    }
}

void jmapgen_objects::merge_parameters_into( mapgen_parameters &params,
        const std::string &outer_context ) const
{
    for( const jmapgen_obj &obj : objects ) {
        obj.second->merge_parameters_into( params, outer_context );
    }
}

void jmapgen_objects::add_placement_coords_to( std::unordered_set<point_rel_ms> &result ) const
{
    for( const jmapgen_obj &obj : objects ) {
        const jmapgen_place &where = obj.first;
        for( int x = where.x.val; x <= where.x.valmax; ++x ) {
            for( int y = where.y.val; y <= where.y.valmax; ++y ) {
                result.emplace( x, y );
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
///// 3 - mapgen (gameplay)
///// stuff below is the actual in-game map generation (ill)logic

mapgen_phase jmapgen_setmap::phase() const
{
    switch( op ) {
        case JMAPGEN_SETMAP_TER:
        case JMAPGEN_SETMAP_LINE_TER:
        case JMAPGEN_SETMAP_SQUARE_TER:
            return mapgen_phase::terrain;
        case JMAPGEN_SETMAP_FURN:
        case JMAPGEN_SETMAP_LINE_FURN:
        case JMAPGEN_SETMAP_SQUARE_FURN:
            return mapgen_phase::furniture;
        case JMAPGEN_SETMAP_TRAP:
        case JMAPGEN_SETMAP_TRAP_REMOVE:
        case JMAPGEN_SETMAP_CREATURE_REMOVE:
        case JMAPGEN_SETMAP_ITEM_REMOVE:
        case JMAPGEN_SETMAP_FIELD_REMOVE:
        case JMAPGEN_SETMAP_LINE_TRAP:
        case JMAPGEN_SETMAP_LINE_TRAP_REMOVE:
        case JMAPGEN_SETMAP_LINE_CREATURE_REMOVE:
        case JMAPGEN_SETMAP_LINE_ITEM_REMOVE:
        case JMAPGEN_SETMAP_LINE_FIELD_REMOVE:
        case JMAPGEN_SETMAP_SQUARE_TRAP:
        case JMAPGEN_SETMAP_SQUARE_TRAP_REMOVE:
        case JMAPGEN_SETMAP_SQUARE_CREATURE_REMOVE:
        case JMAPGEN_SETMAP_SQUARE_ITEM_REMOVE:
        case JMAPGEN_SETMAP_SQUARE_FIELD_REMOVE:
            return mapgen_phase::default_;
        case JMAPGEN_SETMAP_RADIATION:
        case JMAPGEN_SETMAP_BASH:
        case JMAPGEN_SETMAP_BURN:
        case JMAPGEN_SETMAP_VARIABLE:
        case JMAPGEN_SETMAP_LINE_RADIATION:
        case JMAPGEN_SETMAP_SQUARE_RADIATION:
            return mapgen_phase::transform;
        case JMAPGEN_SETMAP_OPTYPE_POINT:
        case JMAPGEN_SETMAP_OPTYPE_LINE:
        case JMAPGEN_SETMAP_OPTYPE_SQUARE:
            break;
    }
    debugmsg( "Invalid jmapgen_setmap::op %d", static_cast<int>( op ) );
    return mapgen_phase::default_;
}

/*
 * (set|line|square)_(ter|furn|trap|radiation); simple (x, y, int) or (x1,y1,x2,y2, int) functions
 * TODO: optimize, though gcc -O2 optimizes enough that splitting the switch has no effect
 */
bool jmapgen_setmap::apply( const mapgendata &dat, const tripoint_rel_ms &offset ) const
{
    if( chance != 1 && !one_in( chance ) ) {
        return true;
    }

    static const auto get = []( const jmapgen_int & v, int offset ) {
        return v.get() + offset;
    };
    const auto x_get = [this, &offset]() {
        return get( x, offset.x() );
    };
    const auto y_get = [this, &offset]() {
        return get( y, offset.y() );
    };
    const auto z_get = [this, &offset]() {
        return get( z, offset.z() );
    };
    const auto x2_get = [this, &offset]() {
        return get( x2, offset.x() );
    };
    const auto y2_get = [this, &offset]() {
        return get( y2, offset.y() );
    };

    map &m = dat.m;
    const int trepeat = repeat.get();
    const tripoint_bub_ms target_pos( x_get(), y_get(), dat.zlevel() + z_get() );
    const int z_level = dat.zlevel() + z_get();

    for( int i = 0; i < trepeat; i++ ) {
        switch( op ) {
            case JMAPGEN_SETMAP_TER: {
                // TODO: the ter_id should be stored separately and not be wrapped in an jmapgen_int
                m.ter_set( target_pos, ter_id( val.get() ),
                           dat.has_flag( jmapgen_flags::avoid_creatures ) );
            }
            break;
            case JMAPGEN_SETMAP_FURN: {
                // TODO: the furn_id should be stored separately and not be wrapped in an jmapgen_int
                m.furn_set( target_pos, furn_id( val.get() ),
                            dat.has_flag( jmapgen_flags::avoid_creatures ) );
            }
            break;
            case JMAPGEN_SETMAP_TRAP: {
                // TODO: the trap_id should be stored separately and not be wrapped in an jmapgen_int
                mtrap_set( &m, target_pos, trap_id( val.get() ),
                           dat.has_flag( jmapgen_flags::avoid_creatures ) );
            }
            break;
            case JMAPGEN_SETMAP_RADIATION: {
                m.set_radiation( target_pos, val.get() );
            }
            break;
            case JMAPGEN_SETMAP_TRAP_REMOVE: {
                mremove_trap( &m, target_pos, trap_id( val.get() ).id() );
            }
            break;
            case JMAPGEN_SETMAP_CREATURE_REMOVE: {
                Creature *tmp_critter = get_creature_tracker().creature_at( m.get_abs(
                                            target_pos ), true );
                if( tmp_critter && !tmp_critter->is_avatar() ) {
                    tmp_critter->die( &m, nullptr );
                }
            }
            break;
            case JMAPGEN_SETMAP_ITEM_REMOVE: {
                m.i_clear( target_pos );
            }
            break;
            case JMAPGEN_SETMAP_FIELD_REMOVE: {
                mremove_fields( &m, target_pos );
            }
            break;
            case JMAPGEN_SETMAP_BASH: {
                m.bash( target_pos, 9999, true );
                break;
            }
            case JMAPGEN_SETMAP_BURN: {
                const furn_str_id &furn = m.furn( target_pos ).id();
                const ter_str_id &ter = m.ter( target_pos ).id();
                if( ter_furn_has_flag( *ter, *furn, ter_furn_flag::TFLAG_FLAMMABLE ) ||
                    ter_furn_has_flag( *ter, *furn, ter_furn_flag::TFLAG_FLAMMABLE_HARD ) ) {
                    while( m.is_bashable( target_pos ) ) { // one is not enough
                        m.bash( target_pos, 9999, true );
                    }
                    if( one_in( 5 ) && !ter->has_flag( ter_furn_flag::TFLAG_LIQUID ) ) {
                        // This gives very little *wood* ash because the terrain is not flagged as flammable
                        m.spawn_item( target_pos, itype_ash, 1, rng( 1, 10 ) );
                    }
                } else if( ter_furn_has_flag( *ter, *furn, ter_furn_flag::TFLAG_FLAMMABLE_ASH ) ) {
                    while( m.is_bashable( target_pos ) ) {
                        m.bash( target_pos, 9999, true );
                    }
                    if( !m.is_open_air( target_pos ) ) {
                        m.furn_set( target_pos, furn_f_ash );
                        if( !ter->has_flag( ter_furn_flag::TFLAG_LIQUID ) ) {
                            m.spawn_item( target_pos, itype_ash, 1, rng( 10, 1000 ) );
                        }
                    }
                }
                while( m.flammable_items_at( target_pos ) ) {
                    map_stack stack = m.i_at( target_pos );
                    for( auto it = stack.begin(); it != stack.end(); ) {
                        if( it->flammable() ) {
                            m.create_burnproducts( target_pos, *it, it->weight() );
                            it = stack.erase( it );
                        } else {
                            ++it;
                        }
                    }
                }
            }
            break;
            case JMAPGEN_SETMAP_VARIABLE: {
                queued_points[string_val] = m.get_abs( target_pos );
            }
            break;
            case JMAPGEN_SETMAP_LINE_TER: {
                // TODO: the ter_id should be stored separately and not be wrapped in an jmapgen_int
                m.draw_line_ter( ter_id( val.get() ), point_bub_ms( x_get(), y_get() ), point_bub_ms( x2_get(),
                                 y2_get() ),
                                 z_level,
                                 dat.has_flag( jmapgen_flags::avoid_creatures ) );
            }
            break;
            case JMAPGEN_SETMAP_LINE_FURN: {
                // TODO: the furn_id should be stored separately and not be wrapped in an jmapgen_int
                m.draw_line_furn( furn_id( val.get() ), point_bub_ms( x_get(), y_get() ), point_bub_ms( x2_get(),
                                  y2_get() ),
                                  z_level,
                                  dat.has_flag( jmapgen_flags::avoid_creatures ) );
            }
            break;
            case JMAPGEN_SETMAP_LINE_TRAP: {
                const std::vector<point_bub_ms> line = line_to( point_bub_ms( x_get(), y_get() ),
                                                       point_bub_ms( x2_get(), y2_get() ),
                                                       0 );
                for( const point_bub_ms &i : line ) {
                    // TODO: the trap_id should be stored separately and not be wrapped in an jmapgen_int
                    mtrap_set( &m, tripoint_bub_ms( i.x(), i.y(), z_level ), trap_id( val.get() ),
                               dat.has_flag( jmapgen_flags::avoid_creatures ) );
                }
            }
            break;
            case JMAPGEN_SETMAP_LINE_TRAP_REMOVE: {
                const std::vector<point_bub_ms> line = line_to( point_bub_ms( x_get(), y_get() ),
                                                       point_bub_ms( x2_get(), y2_get() ),
                                                       0 );
                for( const point_bub_ms &i : line ) {
                    // TODO: the trap_id should be stored separately and not be wrapped in an jmapgen_int
                    mremove_trap( &m, tripoint_bub_ms( i.x(), i.y(), z_level ), trap_id( val.get() ).id() );
                }
            }
            break;
            case JMAPGEN_SETMAP_LINE_CREATURE_REMOVE: {
                const std::vector<point_bub_ms> line = line_to( point_bub_ms( x_get(), y_get() ),
                                                       point_bub_ms( x2_get(), y2_get() ),
                                                       0 );
                for( const point_bub_ms &i : line ) {
                    Creature *tmp_critter = get_creature_tracker().creature_at( tripoint_abs_ms( m.get_abs(
                                                tripoint_bub_ms( i.x(), i.y(),
                                                        z_level ) ) ), true );
                    if( tmp_critter && !tmp_critter->is_avatar() ) {
                        tmp_critter->die( &m, nullptr );
                    }
                }
            }
            break;
            case JMAPGEN_SETMAP_LINE_ITEM_REMOVE: {
                const std::vector<point_bub_ms> line = line_to( point_bub_ms( x_get(), y_get() ),
                                                       point_bub_ms( x2_get(), y2_get() ),
                                                       0 );
                for( const point_bub_ms &i : line ) {
                    m.i_clear( tripoint_bub_ms( i.x(), i.y(), z_level ) );
                }
            }
            break;
            case JMAPGEN_SETMAP_LINE_FIELD_REMOVE: {
                const std::vector<point_bub_ms> line = line_to( point_bub_ms( x_get(), y_get() ),
                                                       point_bub_ms( x2_get(), y2_get() ),
                                                       0 );
                for( const point_bub_ms &i : line ) {
                    mremove_fields( &m, tripoint_bub_ms( i.x(), i.y(), z_level ) );
                }
            }
            break;
            case JMAPGEN_SETMAP_LINE_RADIATION: {
                const std::vector<point_bub_ms> line = line_to( point_bub_ms( x_get(), y_get() ),
                                                       point_bub_ms( x2_get(), y2_get() ),
                                                       0 );
                for( const point_bub_ms &i : line ) {
                    m.set_radiation( tripoint_bub_ms( i.x(), i.y(), z_level ), static_cast<int>( val.get() ) );
                }
            }
            break;
            case JMAPGEN_SETMAP_SQUARE_TER: {
                // TODO: the ter_id should be stored separately and not be wrapped in an jmapgen_int
                m.draw_square_ter( ter_id( val.get() ), point_bub_ms( x_get(), y_get() ), point_bub_ms( x2_get(),
                                   y2_get() ),
                                   z_level,
                                   dat.has_flag( jmapgen_flags::avoid_creatures ) );
            }
            break;
            case JMAPGEN_SETMAP_SQUARE_FURN: {
                // TODO: the furn_id should be stored separately and not be wrapped in an jmapgen_int
                m.draw_square_furn( furn_id( val.get() ), point_bub_ms( x_get(), y_get() ), point_bub_ms( x2_get(),
                                    y2_get() ),
                                    z_level,
                                    dat.has_flag( jmapgen_flags::avoid_creatures ) );
            }
            break;
            case JMAPGEN_SETMAP_SQUARE_TRAP: {
                const point_rel_ms c( x_get(), y_get() );
                const int cx2 = x2_get();
                const int cy2 = y2_get();
                for( int tx = c.x(); tx <= cx2; tx++ ) {
                    for( int ty = c.y(); ty <= cy2; ty++ ) {
                        // TODO: the trap_id should be stored separately and not be wrapped in an jmapgen_int
                        mtrap_set( &m, tripoint_bub_ms( tx, ty, z_level ), trap_id( val.get() ),
                                   dat.has_flag( jmapgen_flags::avoid_creatures ) );
                    }
                }
            }
            break;
            case JMAPGEN_SETMAP_SQUARE_TRAP_REMOVE: {
                const point_rel_ms c( x_get(), y_get() );
                const int cx2 = x2_get();
                const int cy2 = y2_get();
                for( int tx = c.x(); tx <= cx2; tx++ ) {
                    for( int ty = c.y(); ty <= cy2; ty++ ) {
                        // TODO: the trap_id should be stored separately and not be wrapped in an jmapgen_int
                        mremove_trap( &m, tripoint_bub_ms( tx, ty, z_level ), trap_id( val.get() ).id() );
                    }
                }
            }
            break;
            case JMAPGEN_SETMAP_SQUARE_CREATURE_REMOVE: {
                const point_rel_ms c( x_get(), y_get() );
                const int cx2 = x2_get();
                const int cy2 = y2_get();
                for( int tx = c.x(); tx <= cx2; tx++ ) {
                    for( int ty = c.y(); ty <= cy2; ty++ ) {
                        Creature *tmp_critter = get_creature_tracker().creature_at( tripoint_abs_ms( m.get_abs(
                                                    tripoint_bub_ms( tx,
                                                            ty, z_level ) ) ), true );
                        if( tmp_critter && !tmp_critter->is_avatar() ) {
                            tmp_critter->die( &m, nullptr );
                        }
                    }
                }
            }
            break;
            case JMAPGEN_SETMAP_SQUARE_ITEM_REMOVE: {
                const point_rel_ms c( x_get(), y_get() );
                const int cx2 = x2_get();
                const int cy2 = y2_get();
                for( int tx = c.x(); tx <= cx2; tx++ ) {
                    for( int ty = c.y(); ty <= cy2; ty++ ) {
                        m.i_clear( tripoint_bub_ms( tx, ty, z_level ) );
                    }
                }
            }
            break;
            case JMAPGEN_SETMAP_SQUARE_FIELD_REMOVE: {
                const point_rel_ms c( x_get(), y_get() );
                const int cx2 = x2_get();
                const int cy2 = y2_get();
                for( int tx = c.x(); tx <= cx2; tx++ ) {
                    for( int ty = c.y(); ty <= cy2; ty++ ) {
                        mremove_fields( &m, tripoint_bub_ms( tx, ty, z_level ) );
                    }
                }
            }
            break;
            case JMAPGEN_SETMAP_SQUARE_RADIATION: {
                const point_rel_ms c2( x_get(), y_get() );
                const int cx2 = x2_get();
                const int cy2 = y2_get();
                for( int tx = c2.x(); tx <= cx2; tx++ ) {
                    for( int ty = c2.y(); ty <= cy2; ty++ ) {
                        m.set_radiation( tripoint_bub_ms( tx, ty, z_level ), static_cast<int>( val.get() ) );
                    }
                }
            }
            break;

            default:
                //Suppress warnings
                break;
        }
    }
    return true;
}

ret_val<void> jmapgen_setmap::has_vehicle_collision( const mapgendata &dat,
        const tripoint_rel_ms &offset ) const
{
    static const auto get = []( const jmapgen_int & v, int v_offset ) {
        return v.get() + v_offset;
    };
    const auto x_get = [this, &offset]() {
        return get( x, offset.x() );
    };
    const auto y_get = [this, &offset]() {
        return get( y, offset.y() );
    };
    const auto z_get = [this, &offset]() {
        return get( z, offset.z() );
    };
    const auto x2_get = [this, &offset]() {
        return get( x2, offset.x() );
    };
    const auto y2_get = [this, &offset]() {
        return get( y2, offset.y() );
    };
    const tripoint_bub_ms start = tripoint_bub_ms( x_get(), y_get(), dat.zlevel() + z_get() );
    tripoint_bub_ms end = start;
    switch( op ) {
        case JMAPGEN_SETMAP_TER:
        case JMAPGEN_SETMAP_FURN:
        case JMAPGEN_SETMAP_TRAP:
            break;
        /* lines and squares are the same thing for this purpose */
        case JMAPGEN_SETMAP_LINE_TER:
        case JMAPGEN_SETMAP_LINE_FURN:
        case JMAPGEN_SETMAP_LINE_TRAP:
        case JMAPGEN_SETMAP_LINE_TRAP_REMOVE:
        case JMAPGEN_SETMAP_LINE_ITEM_REMOVE:
        case JMAPGEN_SETMAP_SQUARE_TER:
        case JMAPGEN_SETMAP_SQUARE_FURN:
        case JMAPGEN_SETMAP_SQUARE_TRAP:
        case JMAPGEN_SETMAP_SQUARE_TRAP_REMOVE:
        case JMAPGEN_SETMAP_SQUARE_ITEM_REMOVE:
            end.x() = x2_get();
            end.y() = y2_get();
            break;
        /* if it's not a terrain, furniture, or trap, it can't collide */
        default:
            return ret_val<void>::make_success();
    }
    for( const tripoint_bub_ms &p : dat.m.points_in_rectangle( start, end ) ) {
        const optional_vpart_position &vp = dat.m.veh_at( p );
        if( vp ) {
            return ret_val<void>::make_failure( vp.value().vehicle().disp_name() );
        }
    }
    return ret_val<void>::make_success();
}

ret_val<void> mapgen_function_json_base::has_vehicle_collision(
    const mapgendata &dat, const tripoint_rel_ms &offset ) const
{
    for( const jmapgen_setmap &elem : setmap_points ) {
        const ret_val<void> has_vehicle_collision = elem.has_vehicle_collision( dat, offset );
        if( !has_vehicle_collision.success() ) {
            return has_vehicle_collision;
        }
    }

    return objects.has_vehicle_collision( dat, offset );
}

static ret_val<void> apply_mapgen_in_phases(
    const mapgendata &md, const std::vector<jmapgen_setmap> &setmap_points,
    const jmapgen_objects &objects, const tripoint_rel_ms &offset, const std::string &context,
    bool verify = false )
{
    const ret_val<void> has_vehicle_collision = objects.has_vehicle_collision( md, offset );
    if( verify &&  !has_vehicle_collision.success() ) {

        return has_vehicle_collision;
    }

    // We must apply all the mapgen in phases, but the mapgen is split between
    // setmap_points and objects.  So we have to make an outer loop over
    // phases, and apply each type restricted to each phase.
    auto setmap_point = setmap_points.begin();
    for( mapgen_phase phase : all_enum_values<mapgen_phase>() ) {
        if( std::find( md.skip.begin(), md.skip.end(), phase ) != md.skip.end() ) {
            continue;
        }
        for( ; setmap_point != setmap_points.end(); ++setmap_point ) {
            const jmapgen_setmap &elem = *setmap_point;
            if( elem.phase() != phase ) {
                break;
            }
            const ret_val<void> has_vehicle_collision = elem.has_vehicle_collision( md, offset );
            if( verify && !has_vehicle_collision.success() ) {
                return has_vehicle_collision;
            }
            elem.apply( md, offset );
        }

        objects.apply( md, phase, offset, context );
    }
    cata_assert( setmap_point == setmap_points.end() );

    resolve_regional_terrain_and_furniture( md );

    return ret_val<void>::make_success();
}

/*
 * Apply mapgen as per a derived-from-json recipe; in theory fast, but not very versatile
 */
void mapgen_function_json::generate( mapgendata &md )
{
    map *const m = &md.m;
    const oter_t &ter = *md.terrain_type();

    auto do_predecessor_mapgen = [&]( mapgendata & predecessor_md ) {
        const std::optional<ter_str_id> uniform_ter = predecessor_md.terrain_type()->get_uniform_terrain();
        if( uniform_ter ) {
            m->draw_fill_background( *uniform_ter );
        } else {
            const std::string function_key = predecessor_md.terrain_type()->get_mapgen_id();
            bool success = run_mapgen_func( function_key, predecessor_md );

            if( !success ) {
                debugmsg( "predecessor mapgen with key %s failed", function_key );
            }
        }
    };

    if( predecessor_mapgen != oter_str_id::NULL_ID() ) {
        mapgendata predecessor_md( md, predecessor_mapgen );
        do_predecessor_mapgen( predecessor_md );
    } else if( expects_predecessor() ) {
        if( md.has_predecessor() ) {
            mapgendata predecessor_md( md, md.last_predecessor() );
            predecessor_md.pop_last_predecessor();
            do_predecessor_mapgen( predecessor_md );
        } else {
            mapgendata predecessor_md( md, fallback_predecessor_mapgen_ );
            do_predecessor_mapgen( predecessor_md );
        }
    }

    // Now we have to do some rotation shenanigans. We need to ensure that
    // our predecessor is not rotated out of alignment as part of rotating this location,
    // and there are actually two sources of rotation--the mapgen can rotate explicitly, and
    // the entire overmap terrain may be rotatable. To ensure we end up in the right rotation,
    // we basically have to initially reverse the rotation that we WILL do in the future so that
    // when we apply that rotation, our predecessor is back in its original state while this
    // location is rotated as desired.
    // Note that we need to perform rotations even if there is no predecessor, as other Z levels
    // have to be kept aligned regardless.

    // rotation.get can return a random value if val differs from valmax. Use same value in both directions.
    const int rot = rotation.get() % 4;
    m->rotate( 4 - rot );

    if( ter.is_rotatable() || ter.is_linear() ) {
        m->rotate( ( -ter.get_rotation() + 4 ) % 4 );
    }

    mapgendata md_with_params( md, get_args( md, mapgen_parameter_scope::omt ), flags_ );

    if( fill_ter ) {
        const ter_id chosen_fill_ter = fill_ter->get( md_with_params );
        if( chosen_fill_ter != t_null ) {
            m->draw_fill_background( chosen_fill_ter );
        } else {
            debugmsg( "fill_ter resolved to t_null" );
        }
    }

    apply_mapgen_in_phases( md_with_params, setmap_points, objects, tripoint_rel_ms::zero,
                            context_ );

    m->rotate( rot );

    if( ter.is_rotatable() || ter.is_linear() ) {
        m->rotate( ter.get_rotation() );
    }
    set_queued_points();
}

bool mapgen_function_json::expects_predecessor() const
{
    return fallback_predecessor_mapgen_ != oter_str_id::NULL_ID();
}

mapgen_parameters mapgen_function_json::get_mapgen_params( mapgen_parameter_scope scope ) const
{
    return parameters.params_for_scope( scope );
}

void mapgen_function_json_nested::nest( const mapgendata &md, const tripoint_rel_ms &offset,
                                        const std::string &outer_context ) const
{
    // TODO: Make rotation work for submaps, then pass this value into elem & objects apply.
    //int chosen_rotation = rotation.get() % 4;

    mapgendata md_with_params( md, get_args( md, mapgen_parameter_scope::nest ), flags_ );

    std::string context = context_;
    context += " in ";
    context += outer_context;
    apply_mapgen_in_phases( md_with_params, setmap_points, objects, offset, context );
}

/*
 * Apply mapgen as per a derived-from-json recipe; in theory fast, but not very versatile
 */
void jmapgen_objects::apply( const mapgendata &dat, mapgen_phase phase,
                             const std::string &context ) const
{
    apply( dat, phase, tripoint_rel_ms::zero, context );
}

void jmapgen_objects::apply( const mapgendata &dat, mapgen_phase phase,
                             const tripoint_rel_ms &offset,
                             const std::string &context ) const
{
    bool terrain_resolved = false;

    auto range_at_phase = std::equal_range( objects.begin(), objects.end(), phase, compare_phases );

    for( auto it = range_at_phase.first; it != range_at_phase.second; ++it ) {
        const jmapgen_obj &obj = *it;
        jmapgen_place where = obj.first;
        where.offset( tripoint_rel_ms( -offset.raw() ) );
        const jmapgen_piece &what = *obj.second;

        cata_assert( what.phase() == phase );

        if( !terrain_resolved && typeid( what ) == typeid( jmapgen_vehicle ) ) {
            // In order to determine collisions between vehicles and local "terrain" the terrain has to be resolved
            // This code is based on two assumptions:
            // 1. The terrain part of a definition is always placed first.
            // 2. Only vehicles require the terrain to be resolved. The general solution is to use a virtual function.
            resolve_regional_terrain_and_furniture( dat );
            terrain_resolved = true;
        }

        // The user will only specify repeat once in JSON, but it may get loaded both
        // into the what and where in some cases--we just need the greater value of the two.
        const int repeat = std::max( where.repeat.get(), what.repeat.get() );
        for( int i = 0; i < repeat; i++ ) {
            what.apply( dat, where.x, where.y, where.z, context );
        }
    }
}

ret_val<void> jmapgen_objects::has_vehicle_collision( const mapgendata &dat,
        const tripoint_rel_ms &offset ) const
{
    for( const jmapgen_obj &obj : objects ) {
        jmapgen_place where = obj.first;
        where.offset( tripoint_rel_ms( - offset.raw() ) );
        const auto &what = *obj.second;
        const ret_val<void> has_vehicle_collision = what.has_vehicle_collision( dat,
                tripoint_rel_ms( where.x.get(),
                                 where.y.get(),
                                 where.z.get() ) );
        if( !has_vehicle_collision.success() ) {
            return has_vehicle_collision;
        }
    }
    return ret_val<void>::make_success();
}

void map::draw_map( mapgendata &dat )
{
    const oter_id &terrain_type = dat.terrain_type();
    const std::string function_key = terrain_type->get_mapgen_id();

    if( !run_mapgen_func( function_key, dat ) ) {
        if( is_ot_match( "lab", terrain_type, ot_match_type::contains ) ) {
            draw_lab( dat );
        } else {
            debugmsg( "Error: tried to generate map for omtype %s, \"%s\" (id_mapgen %s)",
                      terrain_type.id().c_str(), terrain_type->get_name( om_vision_level::full ), function_key.c_str() );
            // Fallback to the default mapgen for this z-level or just a grass fill if something has gone horribly wrong
            const oter_id &default_oter = dat.region.default_oter[OVERMAP_DEPTH + dat.zlevel()].id();
            if( terrain_type == default_oter || !run_mapgen_func( default_oter->get_mapgen_id(), dat ) ) {
                fill_background( this, ter_t_grass );
            }
        }
    }

    resolve_regional_terrain_and_furniture( dat );
}

static const int SOUTH_EDGE = 2 * SEEY - 1;
static const int EAST_EDGE = 2 * SEEX  - 1;

// NOLINTNEXTLINE(readability-function-size)
void map::draw_lab( mapgendata &dat )
{
    const oter_id &terrain_type = dat.terrain_type();
    // To distinguish between types of labs
    bool ice_lab = true;
    bool central_lab = false;
    bool tower_lab = false;

    point_bub_ms p2;

    int lw = 0;
    int rw = 0;
    int tw = 0;
    int bw = 0;

    if( terrain_type == oter_lab || terrain_type == oter_lab_stairs
        || terrain_type == oter_lab_core || terrain_type == oter_ants_lab
        || terrain_type == oter_ants_lab_stairs || terrain_type == oter_ice_lab
        || terrain_type == oter_ice_lab_stairs || terrain_type == oter_ice_lab_core
        || terrain_type == oter_central_lab || terrain_type == oter_central_lab_stairs
        || terrain_type == oter_central_lab_core || terrain_type == oter_tower_lab
        || terrain_type == oter_tower_lab_stairs ) {

        ice_lab = is_ot_match( "ice_lab", terrain_type, ot_match_type::prefix );
        central_lab = is_ot_match( "central_lab", terrain_type, ot_match_type::prefix );
        tower_lab = is_ot_match( "tower_lab", terrain_type, ot_match_type::prefix );

        if( ice_lab ) {
            units::temperature_delta temperature = units::from_fahrenheit_delta( -20 + 30 * dat.zlevel() );
            set_temperature_mod( p2, temperature );
            set_temperature_mod( p2 + point( SEEX, 0 ), temperature );
            set_temperature_mod( p2 + point( 0, SEEY ), temperature );
            set_temperature_mod( p2 + point( SEEX, SEEY ), temperature );
        }

        // Check for adjacent sewers; used below
        tw = 0;
        rw = 0;
        bw = 0;
        lw = 0;
        if( ( dat.north()->get_type_id() == oter_type_sewer ) &&
            connects_to( dat.north(), 2 ) ) {
            tw = SOUTH_EDGE + 1;
        }
        if( ( dat.east()->get_type_id() == oter_type_sewer ) &&
            connects_to( dat.east(), 3 ) ) {
            rw = EAST_EDGE + 1;
        }
        if( ( dat.south()->get_type_id() == oter_type_sewer ) &&
            connects_to( dat.south(), 0 ) ) {
            bw = SOUTH_EDGE + 1;
        }
        if( ( dat.west()->get_type_id() == oter_type_sewer ) &&
            connects_to( dat.west(), 1 ) ) {
            lw = EAST_EDGE + 1;
        }
        if( dat.zlevel() == 0 ) { // We're on ground level
            int rot = 0;

            if( dat.east()->get_type_id() == oter_type_road ) {
                rot = 1;
            } else if( dat.south()->get_type_id() == oter_type_road ) {
                rot = 2;
            } else if( dat.west()->get_type_id() == oter_type_road ) {
                rot = 3;
            }

            // Rotate the map backwards so contents can be placed in the 'normal' orientation.
            rotate( 4 - rot );

            for( int i = 0; i < SEEX * 2; i++ ) {
                for( int j = 0; j < SEEY * 2; j++ ) {
                    if( i <= 1 || i >= SEEX * 2 - 2 ||
                        j <= 1 || j >= SEEY * 2 - 2 ||
                        i == SEEX - 2 || i == SEEX + 1 ) {
                        ter_set( point_bub_ms( i, j ), ter_t_concrete_wall );
                    } else {
                        ter_set( point_bub_ms( i, j ), ter_t_floor );
                    }
                }
            }
            ter_set( point_bub_ms( SEEX - 1, 0 ), ter_t_door_metal_locked );
            ter_set( point_bub_ms( SEEX - 1, 1 ), ter_t_floor );
            ter_set( point_bub_ms( SEEX, 0 ), ter_t_door_metal_locked );
            ter_set( point_bub_ms( SEEX, 1 ), ter_t_floor );
            ter_set( point_bub_ms( SEEX - 2 + rng( 0, 1 ) * 3, 0 ), ter_t_card_science );
            ter_set( point_bub_ms( SEEX - 2, SEEY ), ter_t_door_metal_c );
            ter_set( point_bub_ms( SEEX + 1, SEEY ), ter_t_door_metal_c );
            ter_set( point_bub_ms( SEEX - 2, SEEY - 1 ), ter_t_door_metal_c );
            ter_set( point_bub_ms( SEEX + 1, SEEY - 1 ), ter_t_door_metal_c );
            ter_set( point_bub_ms( SEEX - 1, SEEY * 2 - 3 ), ter_t_stairs_down );
            ter_set( point_bub_ms( SEEX, SEEY * 2 - 3 ), ter_t_stairs_down );
            science_room( this, point_bub_ms( 2, 2 ), point_bub_ms( SEEX - 3, SEEY * 2 - 3 ), dat.zlevel(), 1 );
            science_room( this, point_bub_ms( SEEX + 2, 2 ), point_bub_ms( SEEX * 2 - 3, SEEY * 2 - 3 ),
                          dat.zlevel(), 3 );

            place_spawns( GROUP_TURRET, 1, point_bub_ms( SEEX, 5 ), point_bub_ms( SEEX, 5 ), dat.zlevel(), 1,
                          true );

            // Rotate everything back to normal, giving rotated addition its proper rotation.
            rotate( rot );

        } else if( tw != 0 || rw != 0 || lw != 0 || bw != 0 ) { // Sewers!
            for( int i = 0; i < SEEX * 2; i++ ) {
                for( int j = 0; j < SEEY * 2; j++ ) {
                    ter_set( point_bub_ms( i, j ), ter_t_thconc_floor );
                    if( ( ( i < lw || i > EAST_EDGE - rw ) && j > SEEY - 3 && j < SEEY + 2 ) ||
                        ( ( j < tw || j > SOUTH_EDGE - bw ) && i > SEEX - 3 && i < SEEX + 2 ) ) {
                        ter_set( point_bub_ms( i, j ), ter_t_sewage );
                    }
                    if( ( i == 0 && is_ot_match( "lab", dat.east(), ot_match_type::contains ) ) || i == EAST_EDGE ) {
                        if( ter( point_bub_ms( i, j ) ) == ter_t_sewage ) {
                            ter_set( point_bub_ms( i, j ), ter_t_bars );
                        } else if( j == SEEY - 1 || j == SEEY ) {
                            ter_set( point_bub_ms( i, j ), ter_t_door_metal_c );
                        } else {
                            ter_set( point_bub_ms( i, j ), ter_t_concrete_wall );
                        }
                    } else if( ( j == 0 && is_ot_match( "lab", dat.north(), ot_match_type::contains ) ) ||
                               j == SOUTH_EDGE ) {
                        if( ter( point_bub_ms( i, j ) ) == ter_t_sewage ) {
                            ter_set( point_bub_ms( i, j ), ter_t_bars );
                        } else if( i == SEEX - 1 || i == SEEX ) {
                            ter_set( point_bub_ms( i, j ), ter_t_door_metal_c );
                        } else {
                            ter_set( point_bub_ms( i, j ), ter_t_concrete_wall );
                        }
                    }
                }
            }
        } else { // We're below ground, and no sewers
            // Set up the boundaries of walls (connect to adjacent lab squares)
            tw = ( is_ot_match( "lab", dat.north(), ot_match_type::contains ) &&
                   !is_ot_match( "lab_subway", dat.north(), ot_match_type::contains ) ) ? 0 : 2;
            rw = ( is_ot_match( "lab", dat.east(), ot_match_type::contains ) &&
                   !is_ot_match( "lab_subway", dat.east(), ot_match_type::contains ) ) ? 1 : 2;
            bw = ( is_ot_match( "lab", dat.south(), ot_match_type::contains ) &&
                   !is_ot_match( "lab_subway", dat.south(), ot_match_type::contains ) ) ? 1 : 2;
            lw = ( is_ot_match( "lab", dat.west(), ot_match_type::contains ) &&
                   !is_ot_match( "lab_subway", dat.west(), ot_match_type::contains ) ) ? 0 : 2;

            int boarders = 0;
            if( tw == 0 ) {
                boarders++;
            }
            if( rw == 1 ) {
                boarders++;
            }
            if( bw == 1 ) {
                boarders++;
            }
            if( lw == 0 ) {
                boarders++;
            }

            const auto maybe_insert_stairs = [this]( const oter_id & terrain,  const ter_id & t_stair_type ) {
                if( is_ot_match( "stairs", terrain, ot_match_type::contains ) ) {
                    const auto predicate = [this]( const tripoint_bub_ms & p ) {
                        return ter( p ) == ter_t_thconc_floor && furn( p ) == furn_str_id::NULL_ID() &&
                               tr_at( p ).is_null();
                    };
                    const tripoint_range<tripoint_bub_ms> range =
                        points_in_rectangle( tripoint_bub_ms{ 0, 0, abs_sub.z() },
                                             tripoint_bub_ms{ SEEX * 2 - 2, SEEY * 2 - 2, abs_sub.z() } );

                    if( const auto p = random_point( range, predicate ) ) {
                        ter_set( *p, t_stair_type );
                    }
                }
            };

            //A lab area with only one entrance
            if( boarders == 1 ) {
                // If you remove the usage of "lab_1side" here, remove it from mapgen_factory::get_usages above as well.
                int rot = 0;

                if( tw == 2 ) {
                    rot += 2;
                }
                if( rw == 2 ) {
                    rot += 1;
                }
                if( lw == 2 ) {
                    rot += 3;
                }
                rot %= 4;

                // Rotate the map backwards so the new material can be placed in its 'normal' orientation.
                rotate( 4 - rot );

                if( !oter_mapgen.generate( dat, "lab_1side" ) ) {
                    debugmsg( "Error: Tried to generate 1-sided lab but no lab_1side json exists." );
                }

                // Rotate the map back to its normal orientation, resulting in the new addition being rotated properly.
                rotate( rot );

                maybe_insert_stairs( dat.above(), ter_t_stairs_up );
                maybe_insert_stairs( terrain_type, ter_t_stairs_down );
            } else {
                const int hardcoded_4side_map_weight = 1500; // weight of all hardcoded maps.
                // If you remove the usage of "lab_4side" here, remove it from mapgen_factory::get_usages above as well.
                if( oter_mapgen.generate( dat, "lab_4side", hardcoded_4side_map_weight ) ) {
                    // If the map template hasn't handled borders, handle them in code.
                    // Rotated maps cannot handle borders and have to be caught in code.
                    // We determine if a border isn't handled by checking the east-facing
                    // border space where the door normally is -- it should be a wall or door.
                    tripoint_bub_ms east_border( 23, 11, abs_sub.z() );
                    if( !has_flag_ter( ter_furn_flag::TFLAG_WALL, east_border ) &&
                        !has_flag_ter( ter_furn_flag::TFLAG_DOOR, east_border ) ) {
                        // TODO: create a ter_reset function that does ter_set,
                        // furn_set, and i_clear?
                        ter_str_id lw_type = tower_lab ? ter_t_reinforced_glass : ter_t_concrete_wall;
                        ter_str_id tw_type = tower_lab ? ter_t_reinforced_glass : ter_t_concrete_wall;
                        ter_str_id rw_type = tower_lab && rw == 2 ? ter_t_reinforced_glass :
                                             ter_t_concrete_wall;
                        ter_str_id bw_type = tower_lab && bw == 2 ? ter_t_reinforced_glass :
                                             ter_t_concrete_wall;
                        for( int i = 0; i < SEEX * 2; i++ ) {
                            ter_set( point_bub_ms( 23, i ), rw_type );
                            furn_set( point_bub_ms( 23, i ), furn_str_id::NULL_ID() );
                            i_clear( tripoint_bub_ms( 23, i, dat.zlevel() ) );

                            ter_set( point_bub_ms( i, 23 ), bw_type );
                            furn_set( point_bub_ms( i, 23 ), furn_str_id::NULL_ID() );
                            i_clear( tripoint_bub_ms( i, 23, dat.zlevel() ) );

                            if( lw == 2 ) {
                                ter_set( point_bub_ms( 0, i ), lw_type );
                                furn_set( point_bub_ms( 0, i ), furn_str_id::NULL_ID() );
                                i_clear( tripoint_bub_ms( 0, i, dat.zlevel() ) );
                            }
                            if( tw == 2 ) {
                                ter_set( point_bub_ms( i, 0 ), tw_type );
                                furn_set( point_bub_ms( i, 0 ), furn_str_id::NULL_ID() );
                                i_clear( tripoint_bub_ms( i, 0, dat.zlevel() ) );
                            }
                        }
                        if( rw != 2 ) {
                            ter_set( point_bub_ms( 23, 11 ), ter_t_door_metal_c );
                            ter_set( point_bub_ms( 23, 12 ), ter_t_door_metal_c );
                        }
                        if( bw != 2 ) {
                            ter_set( point_bub_ms( 11, 23 ), ter_t_door_metal_c );
                            ter_set( point_bub_ms( 12, 23 ), ter_t_door_metal_c );
                        }
                    }

                    maybe_insert_stairs( dat.above(), ter_t_stairs_up );
                    maybe_insert_stairs( terrain_type, ter_t_stairs_down );
                } else { // then no json maps for lab_4side were found
                    switch( rng( 1, 3 ) ) {
                        case 1:
                            // Cross shaped
                            for( int i = 0; i < SEEX * 2; i++ ) {
                                for( int j = 0; j < SEEY * 2; j++ ) {
                                    if( ( i < lw || i > EAST_EDGE - rw ) ||
                                        ( ( j < SEEY - 1 || j > SEEY ) &&
                                          ( i == SEEX - 2 || i == SEEX + 1 ) ) ||
                                        ( j < tw || j > SOUTH_EDGE - bw ) ||
                                        ( ( i < SEEX - 1 || i > SEEX ) &&
                                          ( j == SEEY - 2 || j == SEEY + 1 ) ) ) {
                                        ter_set( point_bub_ms( i, j ), ter_t_concrete_wall );
                                    } else {
                                        ter_set( point_bub_ms( i, j ), ter_t_thconc_floor );
                                    }
                                }
                            }
                            if( is_ot_match( "stairs", dat.above(), ot_match_type::contains ) ) {
                                ter_set( point_bub_ms( rng( SEEX - 1, SEEX ), rng( SEEY - 1, SEEY ) ),
                                         ter_t_stairs_up );
                            }
                            // Top left
                            if( one_in( 2 ) ) {
                                ter_set( point_bub_ms( SEEX - 2, static_cast<int>( SEEY / 2 ) ), ter_t_door_glass_frosted_c );
                                science_room( this, point_bub_ms( lw, tw ), point_bub_ms( SEEX - 3, SEEY - 3 ), dat.zlevel(), 1 );
                            } else {
                                ter_set( point_bub_ms( SEEX / 2, SEEY - 2 ), ter_t_door_glass_frosted_c );
                                science_room( this, point_bub_ms( lw, tw ), point_bub_ms( SEEX - 3, SEEY - 3 ), dat.zlevel(), 2 );
                            }
                            // Top right
                            if( one_in( 2 ) ) {
                                ter_set( point_bub_ms( SEEX + 1, static_cast<int>( SEEY / 2 ) ), ter_t_door_glass_frosted_c );
                                science_room( this, point_bub_ms( SEEX + 2, tw ), point_bub_ms( EAST_EDGE - rw, SEEY - 3 ),
                                              dat.zlevel(), 3 );
                            } else {
                                ter_set( point_bub_ms( SEEX + static_cast<int>( SEEX / 2 ), SEEY - 2 ),
                                         ter_t_door_glass_frosted_c );
                                science_room( this, point_bub_ms( SEEX + 2, tw ), point_bub_ms( EAST_EDGE - rw, SEEY - 3 ),
                                              dat.zlevel(), 2 );
                            }
                            // Bottom left
                            if( one_in( 2 ) ) {
                                ter_set( point_bub_ms( SEEX / 2, SEEY + 1 ), ter_t_door_glass_frosted_c );
                                science_room( this, point_bub_ms( lw, SEEY + 2 ), point_bub_ms( SEEX - 3, SOUTH_EDGE - bw ),
                                              dat.zlevel(), 0 );
                            } else {
                                ter_set( point_bub_ms( SEEX - 2, SEEY + static_cast<int>( SEEY / 2 ) ),
                                         ter_t_door_glass_frosted_c );
                                science_room( this, point_bub_ms( lw, SEEY + 2 ), point_bub_ms( SEEX - 3, SOUTH_EDGE - bw ),
                                              dat.zlevel(), 1 );
                            }
                            // Bottom right
                            if( one_in( 2 ) ) {
                                ter_set( point_bub_ms( SEEX + static_cast<int>( SEEX / 2 ), SEEY + 1 ),
                                         ter_t_door_glass_frosted_c );
                                science_room( this, point_bub_ms( SEEX + 2, SEEY + 2 ), point_bub_ms( EAST_EDGE - rw,
                                              SOUTH_EDGE - bw ),
                                              dat.zlevel(), 0 );
                            } else {
                                ter_set( point_bub_ms( SEEX + 1, SEEY + static_cast<int>( SEEY / 2 ) ),
                                         ter_t_door_glass_frosted_c );
                                science_room( this, point_bub_ms( SEEX + 2, SEEY + 2 ), point_bub_ms( EAST_EDGE - rw,
                                              SOUTH_EDGE - bw ),
                                              dat.zlevel(), 3 );
                            }
                            if( rw == 1 ) {
                                ter_set( point_bub_ms( EAST_EDGE, SEEY - 1 ), ter_t_door_metal_c );
                                ter_set( point_bub_ms( EAST_EDGE, SEEY ), ter_t_door_metal_c );
                            }
                            if( bw == 1 ) {
                                ter_set( point_bub_ms( SEEX - 1, SOUTH_EDGE ), ter_t_door_metal_c );
                                ter_set( point_bub_ms( SEEX, SOUTH_EDGE ), ter_t_door_metal_c );
                            }
                            if( is_ot_match( "stairs", terrain_type, ot_match_type::contains ) ) { // Stairs going down
                                std::vector<point_bub_ms> stair_points;
                                stair_points.reserve( 8 );
                                if( tw != 0 ) {
                                    stair_points.emplace_back( SEEX - 1, 2 );
                                    stair_points.emplace_back( SEEX - 1, 2 );
                                    stair_points.emplace_back( SEEX, 2 );
                                    stair_points.emplace_back( SEEX, 2 );
                                }
                                if( rw != 1 ) {
                                    stair_points.emplace_back( SEEX * 2 - 3, SEEY - 1 );
                                    stair_points.emplace_back( SEEX * 2 - 3, SEEY - 1 );
                                    stair_points.emplace_back( SEEX * 2 - 3, SEEY );
                                    stair_points.emplace_back( SEEX * 2 - 3, SEEY );
                                }
                                if( bw != 1 ) {
                                    stair_points.emplace_back( SEEX - 1, SEEY * 2 - 3 );
                                    stair_points.emplace_back( SEEX - 1, SEEY * 2 - 3 );
                                    stair_points.emplace_back( SEEX, SEEY * 2 - 3 );
                                    stair_points.emplace_back( SEEX, SEEY * 2 - 3 );
                                }
                                if( lw != 0 ) {
                                    stair_points.emplace_back( 2, SEEY - 1 );
                                    stair_points.emplace_back( 2, SEEY - 1 );
                                    stair_points.emplace_back( 2, SEEY );
                                    stair_points.emplace_back( 2, SEEY );
                                }
                                stair_points.emplace_back( static_cast<int>( SEEX / 2 ), SEEY );
                                stair_points.emplace_back( static_cast<int>( SEEX / 2 ), SEEY - 1 );
                                stair_points.emplace_back( static_cast<int>( SEEX / 2 ) + SEEX, SEEY );
                                stair_points.emplace_back( static_cast<int>( SEEX / 2 ) + SEEX, SEEY - 1 );
                                stair_points.emplace_back( SEEX, static_cast<int>( SEEY / 2 ) );
                                stair_points.emplace_back( SEEX + 2, static_cast<int>( SEEY / 2 ) );
                                stair_points.emplace_back( SEEX, static_cast<int>( SEEY / 2 ) + SEEY );
                                stair_points.emplace_back( SEEX + 2, static_cast<int>( SEEY / 2 ) + SEEY );
                                const point_bub_ms p = random_entry( stair_points );
                                ter_set( p, ter_t_stairs_down );
                            }

                            break;

                        case 2:
                            // tic-tac-toe # layout
                            for( int i = 0; i < SEEX * 2; i++ ) {
                                for( int j = 0; j < SEEY * 2; j++ ) {
                                    if( i < lw || i > EAST_EDGE - rw ||
                                        i == SEEX - 4 || i == SEEX + 3 ||
                                        j < tw || j > SOUTH_EDGE - bw ||
                                        j == SEEY - 4 || j == SEEY + 3 ) {
                                        ter_set( point_bub_ms( i, j ), ter_t_concrete_wall );
                                    } else {
                                        ter_set( point_bub_ms( i, j ), ter_t_thconc_floor );
                                    }
                                }
                            }
                            if( is_ot_match( "stairs", dat.above(), ot_match_type::contains ) ) {
                                ter_set( point_bub_ms( SEEX - 1, SEEY - 1 ), ter_t_stairs_up );
                                ter_set( point_bub_ms( SEEX, SEEY - 1 ), ter_t_stairs_up );
                                ter_set( point_bub_ms( SEEX - 1, SEEY ), ter_t_stairs_up );
                                ter_set( point_bub_ms( SEEX, SEEY ), ter_t_stairs_up );
                            }
                            ter_set( point_bub_ms( SEEX - rng( 0, 1 ), SEEY - 4 ), ter_t_door_glass_frosted_c );
                            ter_set( point_bub_ms( SEEX - rng( 0, 1 ), SEEY + 3 ), ter_t_door_glass_frosted_c );
                            ter_set( point_bub_ms( SEEX - 4, SEEY + rng( 0, 1 ) ), ter_t_door_glass_frosted_c );
                            ter_set( point_bub_ms( SEEX + 3, SEEY + rng( 0, 1 ) ), ter_t_door_glass_frosted_c );
                            ter_set( point_bub_ms( SEEX - 4, static_cast<int>( SEEY / 2 ) ), ter_t_door_glass_frosted_c );
                            ter_set( point_bub_ms( SEEX + 3, static_cast<int>( SEEY / 2 ) ), ter_t_door_glass_frosted_c );
                            ter_set( point_bub_ms( SEEX / 2, SEEY - 4 ), ter_t_door_glass_frosted_c );
                            ter_set( point_bub_ms( SEEX / 2, SEEY + 3 ), ter_t_door_glass_frosted_c );
                            ter_set( point_bub_ms( SEEX + static_cast<int>( SEEX / 2 ), SEEY - 4 ),
                                     ter_t_door_glass_frosted_c );
                            ter_set( point_bub_ms( SEEX + static_cast<int>( SEEX / 2 ), SEEY + 3 ),
                                     ter_t_door_glass_frosted_c );
                            ter_set( point_bub_ms( SEEX - 4, SEEY + static_cast<int>( SEEY / 2 ) ),
                                     ter_t_door_glass_frosted_c );
                            ter_set( point_bub_ms( SEEX + 3, SEEY + static_cast<int>( SEEY / 2 ) ),
                                     ter_t_door_glass_frosted_c );
                            science_room( this, point_bub_ms( lw, tw ), point_bub_ms( SEEX - 5, SEEY - 5 ), dat.zlevel(),
                                          rng( 1, 2 ) );
                            science_room( this, point_bub_ms( SEEX - 3, tw ), point_bub_ms( SEEX + 2, SEEY - 5 ), dat.zlevel(),
                                          2 );
                            science_room( this, point_bub_ms( SEEX + 4, tw ), point_bub_ms( EAST_EDGE - rw, SEEY - 5 ),
                                          dat.zlevel(), rng( 2, 3 ) );
                            science_room( this, point_bub_ms( lw, SEEY - 3 ), point_bub_ms( SEEX - 5, SEEY + 2 ), dat.zlevel(),
                                          1 );
                            science_room( this, point_bub_ms( SEEX + 4, SEEY - 3 ), point_bub_ms( EAST_EDGE - rw, SEEY + 2 ),
                                          dat.zlevel(), 3 );
                            science_room( this, point_bub_ms( lw, SEEY + 4 ), point_bub_ms( SEEX - 5, SOUTH_EDGE - bw ),
                                          dat.zlevel(), rng( 0, 1 ) );
                            science_room( this, point_bub_ms( SEEX - 3, SEEY + 4 ), point_bub_ms( SEEX + 2, SOUTH_EDGE - bw ),
                                          dat.zlevel(), 0 );
                            science_room( this, point_bub_ms( SEEX + 4, SEEX + 4 ), point_bub_ms( EAST_EDGE - rw,
                                          SOUTH_EDGE - bw ),
                                          dat.zlevel(), 3 * rng( 0, 1 ) );
                            if( rw == 1 ) {
                                ter_set( point_bub_ms( EAST_EDGE, SEEY - 1 ), ter_t_door_metal_c );
                                ter_set( point_bub_ms( EAST_EDGE, SEEY ), ter_t_door_metal_c );
                            }
                            if( bw == 1 ) {
                                ter_set( point_bub_ms( SEEX - 1, SOUTH_EDGE ), ter_t_door_metal_c );
                                ter_set( point_bub_ms( SEEX, SOUTH_EDGE ), ter_t_door_metal_c );
                            }
                            if( is_ot_match( "stairs", terrain_type, ot_match_type::contains ) ) {
                                ter_set( point_bub_ms( SEEX - 3 + 5 * rng( 0, 1 ), SEEY - 3 + 5 * rng( 0, 1 ) ),
                                         ter_t_stairs_down );
                            }
                            break;

                        case 3:
                            // Big room
                            for( int i = 0; i < SEEX * 2; i++ ) {
                                for( int j = 0; j < SEEY * 2; j++ ) {
                                    if( i < lw || i >= EAST_EDGE - rw ||
                                        j < tw || j >= SOUTH_EDGE - bw ) {
                                        ter_set( point_bub_ms( i, j ), ter_t_concrete_wall );
                                    } else {
                                        ter_set( point_bub_ms( i, j ), ter_t_thconc_floor );
                                    }
                                }
                            }
                            science_room( this, point_bub_ms( lw, tw ), point_bub_ms( EAST_EDGE - rw, SOUTH_EDGE - bw ),
                                          dat.zlevel(), rng( 0, 3 ) );

                            if( rw == 1 ) {
                                ter_set( point_bub_ms( EAST_EDGE, SEEY - 1 ), ter_t_door_metal_c );
                                ter_set( point_bub_ms( EAST_EDGE, SEEY ), ter_t_door_metal_c );
                            }
                            if( bw == 1 ) {
                                ter_set( point_bub_ms( SEEX - 1, SOUTH_EDGE ), ter_t_door_metal_c );
                                ter_set( point_bub_ms( SEEX, SOUTH_EDGE ), ter_t_door_metal_c );
                            }
                            maybe_insert_stairs( dat.above(), ter_t_stairs_up );
                            maybe_insert_stairs( terrain_type, ter_t_stairs_down );
                            break;
                    }
                } // endif use_hardcoded_4side_map
            }  // end 1 vs 4 sides
        } // end aboveground vs belowground

        // Ants will totally wreck up the place
        if( is_ot_match( "ants", terrain_type, ot_match_type::contains ) ) {
            for( int i = 0; i < SEEX * 2; i++ ) {
                for( int j = 0; j < SEEY * 2; j++ ) {
                    // Carve out a diamond area that covers 2 spaces on each edge.
                    if( i + j > 10 && i + j < 36 && std::abs( i - j ) < 13 ) {
                        // Doors and walls get sometimes destroyed:
                        // 100% at the edge, usually in a central cross, occasionally elsewhere.
                        if( ( has_flag_ter( ter_furn_flag::TFLAG_DOOR, point_bub_ms( i, j ) ) ||
                              has_flag_ter( ter_furn_flag::TFLAG_WALL, point_bub_ms( i, j ) ) ) ) {
                            if( ( i == 0 || j == 0 || i == 23 || j == 23 ) ||
                                ( !one_in( 3 ) && ( i == 11 || i == 12 || j == 11 || j == 12 ) ) ||
                                one_in( 4 ) ) {
                                // bash and usually remove the rubble.
                                make_rubble( tripoint_bub_ms( i, j, abs_sub.z() ) );
                                ter_set( point_bub_ms( i, j ), ter_t_rock_floor );
                                if( !one_in( 3 ) ) {
                                    furn_set( point_bub_ms( i, j ), furn_str_id::NULL_ID() );
                                }
                            }
                            // and then randomly destroy 5% of the remaining nonstairs.
                        } else if( one_in( 20 ) &&
                                   !has_flag_ter( ter_furn_flag::TFLAG_GOES_DOWN, p2 ) &&
                                   !has_flag_ter( ter_furn_flag::TFLAG_GOES_UP, p2 ) ) {
                            destroy( tripoint_bub_ms( i, j, abs_sub.z() ) );
                            // bashed squares can create dirt & floors, but we want rock floors.
                            const ter_id &floor_to_check_ter = ter( point_bub_ms( i, j ) );
                            if( floor_to_check_ter == ter_t_dirt || floor_to_check_ter == ter_t_floor ) {
                                ter_set( point_bub_ms( i, j ), ter_t_rock_floor );
                            }
                        }
                    }
                }
            }
        }

        int light_odds = 0;
        // central labs are always fully lit, other labs have half chance of some lights.
        if( central_lab ) {
            light_odds = 1;
        } else if( one_in( 2 ) ) {
            // Create a spread of densities, from all possible lights on, to 1/3, ...
            // to ~1 per segment.
            light_odds = std::pow( rng( 1, 12 ), 1.6 );
        }
        if( light_odds > 0 ) {
            for( int i = 0; i < SEEX * 2; i++ ) {
                for( int j = 0; j < SEEY * 2; j++ ) {
                    if( !( ( i * j ) % 2 || ( i + j ) % 4 ) && one_in( light_odds ) ) {
                        const ter_id &nearby_ter = ter( point_bub_ms( i, j ) );
                        if( nearby_ter == ter_t_thconc_floor || nearby_ter == ter_t_strconc_floor ) {
                            ter_set( point_bub_ms( i, j ), ter_t_thconc_floor_olight );
                        }
                    }
                }
            }
        }

        if( tower_lab ) {
            place_spawns( GROUP_LAB, 1, point_bub_ms::zero, point_bub_ms( EAST_EDGE, SOUTH_EDGE ),
                          abs_sub.z(), 0.02f );
        }

        // Lab special effects.
        if( one_in( 10 ) ) {
            switch( rng( 1, 7 ) ) {
                // full flooding/sewage
                case 1: {
                    if( is_ot_match( "stairs", terrain_type, ot_match_type::contains ) ||
                        is_ot_match( "ice", terrain_type, ot_match_type::contains ) ) {
                        // don't flood if stairs because the floor below will not be flooded.
                        // don't flood if ice lab because there's no mechanic for freezing
                        // liquid floors.
                        break;
                    }
                    const ter_id &fluid_type = one_in( 3 ) ? ter_t_sewage : ter_t_water_sh;
                    for( int i = 0; i < EAST_EDGE; i++ ) {
                        for( int j = 0; j < SOUTH_EDGE; j++ ) {
                            // We spare some terrain to make it look better visually.
                            const ter_id &nearby_ter = ter( point_bub_ms( i, j ) );
                            if( !one_in( 10 ) && ( nearby_ter == ter_t_thconc_floor ||
                                                   nearby_ter == ter_t_strconc_floor ||
                                                   nearby_ter == ter_t_thconc_floor_olight ) ) {
                                ter_set( point_bub_ms( i, j ), fluid_type );
                            } else if( has_flag_ter( ter_furn_flag::TFLAG_DOOR, point_bub_ms( i, j ) ) && !one_in( 3 ) ) {
                                // We want the actual debris, but not the rubble marker or dirt.
                                make_rubble( tripoint_bub_ms( i, j, abs_sub.z() ) );
                                ter_set( point_bub_ms( i, j ), fluid_type );
                                furn_set( point_bub_ms( i, j ), furn_str_id::NULL_ID() );
                            }
                        }
                    }
                    break;
                }
                // minor flooding/sewage
                case 2: {
                    if( is_ot_match( "stairs", terrain_type, ot_match_type::contains ) ||
                        is_ot_match( "ice", terrain_type, ot_match_type::contains ) ) {
                        // don't flood if stairs because the floor below will not be flooded.
                        // don't flood if ice lab because there's no mechanic for freezing
                        // liquid floors.
                        break;
                    }
                    const ter_id &fluid_type = one_in( 3 ) ? ter_t_sewage : ter_t_water_sh;
                    for( int i = 0; i < 2; ++i ) {
                        draw_rough_circle( [this, fluid_type]( const point_bub_ms & p ) {
                            const ter_id &maybe_flood_ter = ter( p );
                            if( maybe_flood_ter == ter_t_thconc_floor || maybe_flood_ter == ter_t_strconc_floor ||
                                maybe_flood_ter == ter_t_thconc_floor_olight ) {
                                ter_set( p, fluid_type );
                            } else if( has_flag_ter( ter_furn_flag::TFLAG_DOOR, p ) ) {
                                // We want the actual debris, but not the rubble marker or dirt.
                                make_rubble( { p, abs_sub.z() } );
                                ter_set( p, fluid_type );
                                furn_set( p, furn_str_id::NULL_ID() );
                            }
                        }, point_bub_ms( rng( 1, SEEX * 2 - 2 ), rng( 1, SEEY * 2 - 2 ) ), rng( 3, 6 ) );
                    }
                    break;
                }
                // toxic gas leaks and smoke-filled rooms.
                case 3:
                case 4: {
                    bool is_toxic = one_in( 3 );
                    for( int i = 0; i < SEEX * 2; i++ ) {
                        for( int j = 0; j < SEEY * 2; j++ ) {
                            const ter_id &nearby_ter = ter( point_bub_ms( i, j ) );
                            if( one_in( 200 ) && ( nearby_ter == ter_t_thconc_floor ||
                                                   nearby_ter == ter_t_strconc_floor ) ) {
                                if( is_toxic ) {
                                    add_field( tripoint_bub_ms{i, j, abs_sub.z()}, fd_gas_vent, 1 );
                                } else {
                                    add_field( tripoint_bub_ms{i, j, abs_sub.z()}, fd_smoke_vent, 2 );
                                }
                            }
                        }
                    }
                    break;
                }
                // portal with an artifact effect.
                case 5: {
                    tripoint_range<tripoint_bub_ms> options =
                    points_in_rectangle( tripoint_bub_ms{ 6, 6, abs_sub.z() },
                    { SEEX * 2 - 7, SEEY * 2 - 7, abs_sub.z() } );
                    std::optional<tripoint_bub_ms> center = random_point(
                    options, [&]( const tripoint_bub_ms & p ) {
                        return tr_at( p ).is_null();
                    } );
                    if( !center ) {
                        break;
                    }
                    std::vector<artifact_natural_property> valid_props = {
                        ARTPROP_BREATHING,
                        ARTPROP_CRACKLING,
                        ARTPROP_WARM,
                        ARTPROP_SCALED,
                        ARTPROP_WHISPERING,
                        ARTPROP_GLOWING
                    };
                    draw_rough_circle( [this]( const point_bub_ms & p ) {
                        if( has_flag_ter( ter_furn_flag::TFLAG_GOES_DOWN, p ) ||
                            has_flag_ter( ter_furn_flag::TFLAG_GOES_UP, p ) ||
                            has_flag_ter( ter_furn_flag::TFLAG_CONSOLE, p ) ) {
                            return; // spare stairs and consoles.
                        }
                        make_rubble( {p, abs_sub.z() } );
                        ter_set( p, ter_t_thconc_floor );
                    }, center->xy(), 4 );
                    furn_set( center->xy(), furn_str_id::NULL_ID() );
                    if( !is_open_air( *center ) ) {
                        trap_set( *center, tr_portal );
                        create_anomaly( *center, random_entry( valid_props ), false );
                    }
                    break;
                }
                // portal with fungal invasion
                case 7: {
                    for( int i = 0; i < EAST_EDGE; i++ ) {
                        for( int j = 0; j < SOUTH_EDGE; j++ ) {
                            // Create a mostly spread fungal area throughout entire lab.
                            if( !one_in( 5 ) && has_flag( ter_furn_flag::TFLAG_FLAT, point_bub_ms( i, j ) ) ) {
                                ter_set( point_bub_ms( i, j ), ter_t_fungus_floor_in );
                                if( has_flag_furn( ter_furn_flag::TFLAG_ORGANIC, tripoint_bub_ms( i, j, abs_sub.z() ) ) ) {
                                    furn_set( point_bub_ms( i, j ), furn_f_fungal_clump );
                                }
                            } else if( has_flag_ter( ter_furn_flag::TFLAG_DOOR, point_bub_ms( i, j ) ) && !one_in( 5 ) ) {
                                ter_set( point_bub_ms( i, j ), ter_t_fungus_floor_in );
                            } else if( has_flag_ter( ter_furn_flag::TFLAG_WALL, point_bub_ms( i, j ) ) && one_in( 3 ) ) {
                                ter_set( point_bub_ms( i, j ), ter_t_fungus_wall );
                            }
                        }
                    }
                    tripoint_bub_ms center( rng( 6, SEEX * 2 - 7 ), rng( 6, SEEY * 2 - 7 ), abs_sub.z() );

                    // Make a portal surrounded by more dense fungal stuff and a fungaloid.
                    draw_rough_circle( [this]( const point_bub_ms & p ) {
                        if( has_flag_ter( ter_furn_flag::TFLAG_GOES_DOWN, p ) ||
                            has_flag_ter( ter_furn_flag::TFLAG_GOES_UP, p ) ||
                            has_flag_ter( ter_furn_flag::TFLAG_CONSOLE, p ) ) {
                            return; // spare stairs and consoles.
                        }
                        if( has_flag_ter( ter_furn_flag::TFLAG_WALL, p ) ) {
                            ter_set( p, ter_t_fungus_wall );
                        } else {
                            ter_set( p, ter_t_fungus_floor_in );
                            if( one_in( 3 ) ) {
                                furn_set( p, furn_f_flower_fungal );
                            } else if( one_in( 10 ) ) {
                                ter_set( p, ter_t_marloss );
                            }
                        }
                    }, center.xy(), 3 );
                    ter_set( center.xy(), ter_t_fungus_floor_in );
                    furn_set( center.xy(), furn_str_id::NULL_ID() );
                    trap_set( center, tr_portal );
                    place_spawns( GROUP_FUNGI_FUNGALOID, 1, center.xy() + point( -2, -2 ),
                                  center.xy() + point( 2, 2 ), center.z(), 1, true );

                    break;
                }
            }
        }
    } else if( terrain_type == oter_lab_finale || terrain_type == oter_ice_lab_finale ||
               terrain_type == oter_central_lab_finale || terrain_type == oter_tower_lab_finale ) {

        ice_lab = is_ot_match( "ice_lab", terrain_type, ot_match_type::prefix );
        central_lab = is_ot_match( "central_lab", terrain_type, ot_match_type::prefix );
        tower_lab = is_ot_match( "tower_lab", terrain_type, ot_match_type::prefix );

        if( ice_lab ) {
            units::temperature_delta temperature_d = units::from_fahrenheit_delta( -20 + 30 * dat.zlevel() );
            set_temperature_mod( p2, temperature_d );
            set_temperature_mod( p2 + point( SEEX, 0 ), temperature_d );
            set_temperature_mod( p2 + point( 0, SEEY ), temperature_d );
            set_temperature_mod( p2 + point( SEEX, SEEY ), temperature_d );
        }

        tw = ( is_ot_match( "lab", dat.north(), ot_match_type::contains ) &&
               !is_ot_match( "lab_subway", dat.north(), ot_match_type::contains ) ) ? 0 : 2;
        rw = ( is_ot_match( "lab", dat.east(), ot_match_type::contains ) &&
               !is_ot_match( "lab_subway", dat.east(), ot_match_type::contains ) ) ? 1 : 2;
        bw = ( is_ot_match( "lab", dat.south(), ot_match_type::contains ) &&
               !is_ot_match( "lab_subway", dat.south(), ot_match_type::contains ) ) ? 1 : 2;
        lw = ( is_ot_match( "lab", dat.west(), ot_match_type::contains ) &&
               !is_ot_match( "lab_subway", dat.west(), ot_match_type::contains ) ) ? 0 : 2;

        const int hardcoded_finale_map_weight = 500; // weight of all hardcoded maps.
        // If you remove the usage of "lab_finale_1level" here, remove it from mapgen_factory::get_usages above as well.
        if( oter_mapgen.generate( dat, "lab_finale_1level", hardcoded_finale_map_weight ) ) {
            // If the map template hasn't handled borders, handle them in code.
            // Rotated maps cannot handle borders and have to be caught in code.
            // We determine if a border isn't handled by checking the east-facing
            // border space where the door normally is -- it should be a wall or door.
            tripoint_bub_ms east_border( 23, 11, abs_sub.z() );
            if( !has_flag_ter( ter_furn_flag::TFLAG_WALL, east_border ) &&
                !has_flag_ter( ter_furn_flag::TFLAG_DOOR, east_border ) ) {
                // TODO: create a ter_reset function that does ter_set, furn_set, and i_clear?
                ter_str_id lw_type = tower_lab ? ter_t_reinforced_glass : ter_t_concrete_wall;
                ter_str_id tw_type = tower_lab ? ter_t_reinforced_glass : ter_t_concrete_wall;
                ter_str_id rw_type = tower_lab && rw == 2 ? ter_t_reinforced_glass : ter_t_concrete_wall;
                ter_str_id bw_type = tower_lab && bw == 2 ? ter_t_reinforced_glass : ter_t_concrete_wall;
                for( int i = 0; i < SEEX * 2; i++ ) {
                    ter_set( point_bub_ms( 23, i ), rw_type );
                    furn_set( point_bub_ms( 23, i ), furn_str_id::NULL_ID() );
                    i_clear( tripoint_bub_ms( 23, i, dat.zlevel() ) );

                    ter_set( point_bub_ms( i, 23 ), bw_type );
                    furn_set( point_bub_ms( i, 23 ), furn_str_id::NULL_ID() );
                    i_clear( tripoint_bub_ms( i, 23, dat.zlevel() ) );

                    if( lw == 2 ) {
                        ter_set( point_bub_ms( 0, i ), lw_type );
                        furn_set( point_bub_ms( 0, i ), furn_str_id::NULL_ID() );
                        i_clear( tripoint_bub_ms( 0, i, dat.zlevel() ) );
                    }
                    if( tw == 2 ) {
                        ter_set( point_bub_ms( i, 0 ), tw_type );
                        furn_set( point_bub_ms( i, 0 ), furn_str_id::NULL_ID() );
                        i_clear( tripoint_bub_ms( i, 0, dat.zlevel() ) );
                    }
                }
                if( rw != 2 ) {
                    ter_set( point_bub_ms( 23, 11 ), ter_t_door_metal_c );
                    ter_set( point_bub_ms( 23, 12 ), ter_t_door_metal_c );
                }
                if( bw != 2 ) {
                    ter_set( point_bub_ms( 11, 23 ), ter_t_door_metal_c );
                    ter_set( point_bub_ms( 12, 23 ), ter_t_door_metal_c );
                }
            }
        } else { // then no json maps for lab_finale_1level were found
            // Start by setting up a large, empty room.
            for( int i = 0; i < SEEX * 2; i++ ) {
                for( int j = 0; j < SEEY * 2; j++ ) {
                    if( i < lw || i > EAST_EDGE - rw || j < tw || j > SOUTH_EDGE - bw ) {
                        ter_set( point_bub_ms( i, j ), ter_t_concrete_wall );
                    } else {
                        ter_set( point_bub_ms( i, j ), ter_t_thconc_floor );
                    }
                }
            }
            if( rw == 1 ) {
                ter_set( point_bub_ms( EAST_EDGE, SEEY - 1 ), ter_t_door_metal_c );
                ter_set( point_bub_ms( EAST_EDGE, SEEY ), ter_t_door_metal_c );
            }
            if( bw == 1 ) {
                ter_set( point_bub_ms( SEEX - 1, SOUTH_EDGE ), ter_t_door_metal_c );
                ter_set( point_bub_ms( SEEX, SOUTH_EDGE ), ter_t_door_metal_c );
            }

            int loot_variant; //only used for weapons testing variant.
            computer *tmpcomp = nullptr;
            switch( rng( 1, 5 ) ) {
                // Weapons testing - twice as common because it has 4 variants.
                case 1:
                case 2:
                    loot_variant = rng( 1, 100 ); //The variants have a 67/22/7/4 split.
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point_bub_ms( 6, 6 ), point_bub_ms( 6, 6 ), abs_sub.z(), 1,
                                  true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point_bub_ms( SEEX * 2 - 7, 6 ),
                                  point_bub_ms( SEEX * 2 - 7, 6 ), abs_sub.z(), 1, true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point_bub_ms( 6, SEEY * 2 - 7 ),
                                  point_bub_ms( 6, SEEY * 2 - 7 ), abs_sub.z(), 1, true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point_bub_ms( SEEX * 2 - 7, SEEY * 2 - 7 ),
                                  point_bub_ms( SEEX * 2 - 7, SEEY * 2 - 7 ), abs_sub.z(), 1, true );
                    spawn_item( point_bub_ms( SEEX - 4, SEEY - 2 ), itype_id_science );
                    if( loot_variant <= 96 ) {
                        mtrap_set( this, tripoint_bub_ms( SEEX - 3, SEEY - 3, dat.zlevel() ), tr_dissector );
                        mtrap_set( this, tripoint_bub_ms( SEEX + 2, SEEY - 3, dat.zlevel() ), tr_dissector );
                        mtrap_set( this, tripoint_bub_ms( SEEX - 3, SEEY + 2, dat.zlevel() ), tr_dissector );
                        mtrap_set( this, tripoint_bub_ms( SEEX + 2, SEEY + 2, dat.zlevel() ), tr_dissector );
                        line( this, ter_t_reinforced_glass, point_bub_ms( SEEX + 1, SEEY + 1 ), point_bub_ms( SEEX - 2,
                                SEEY + 1 ),
                              dat.zlevel() );
                        line( this, ter_t_reinforced_glass, point_bub_ms( SEEX - 2, SEEY ), point_bub_ms( SEEX - 2,
                                SEEY - 2 ),
                              dat.zlevel() );
                        line( this, ter_t_reinforced_glass, point_bub_ms( SEEX - 1, SEEY - 2 ), point_bub_ms( SEEX + 1,
                                SEEY - 2 ),
                              dat.zlevel() );
                        ter_set( point_bub_ms( SEEX + 1, SEEY - 1 ), ter_t_reinforced_glass );
                        ter_set( point_bub_ms( SEEX + 1, SEEY ), ter_t_reinforced_door_glass_c );
                        furn_set( point_bub_ms( SEEX - 1, SEEY - 1 ), furn_f_table );
                        furn_set( point_bub_ms( SEEX, SEEY - 1 ), furn_f_table );
                        furn_set( point_bub_ms( SEEX - 1, SEEY ), furn_f_table );
                        furn_set( point_bub_ms( SEEX, SEEY ), furn_f_table );
                        if( loot_variant <= 67 ) {
                            spawn_item( point_bub_ms( SEEX, SEEY - 1 ), itype_UPS_off );
                            spawn_item( point_bub_ms( SEEX, SEEY - 1 ), itype_heavy_battery_cell );
                            spawn_item( point_bub_ms( SEEX - 1, SEEY ), itype_v29 );
                            spawn_item( point_bub_ms( SEEX - 1, SEEY ), itype_laser_rifle, dice( 1, 0 ) );
                            spawn_item( point_bub_ms( SEEX, SEEY ), itype_plasma_gun );
                            spawn_item( point_bub_ms( SEEX, SEEY ), itype_plasma );
                            spawn_item( point_bub_ms( SEEX - 1, SEEY ), itype_recipe_atomic_battery );
                            spawn_item( point_bub_ms( SEEX + 1, SEEY ), itype_plut_cell, rng( 8, 20 ) );
                        } else if( loot_variant < 89 ) {
                            spawn_item( point_bub_ms( SEEX, SEEY ), itype_recipe_atomic_battery );
                            spawn_item( point_bub_ms( SEEX + 1, SEEY ), itype_plut_cell, rng( 8, 20 ) );
                        }  else { // loot_variant between 90 and 96.
                            spawn_item( point_bub_ms( SEEX - 1, SEEY - 1 ), itype_rm13_armor );
                            spawn_item( point_bub_ms( SEEX, SEEY - 1 ), itype_plut_cell );
                            spawn_item( point_bub_ms( SEEX - 1, SEEY ), itype_plut_cell );
                            spawn_item( point_bub_ms( SEEX, SEEY ), itype_recipe_caseless );
                        }
                    } else { // 4% of the lab ends will be this weapons testing end.
                        mtrap_set( this, tripoint_bub_ms( SEEX - 4, SEEY - 3, dat.zlevel() ), tr_dissector );
                        mtrap_set( this, tripoint_bub_ms( SEEX + 3, SEEY - 3, dat.zlevel() ), tr_dissector );
                        mtrap_set( this, tripoint_bub_ms( SEEX - 4, SEEY + 2, dat.zlevel() ), tr_dissector );
                        mtrap_set( this, tripoint_bub_ms( SEEX + 3, SEEY + 2, dat.zlevel() ), tr_dissector );

                        furn_set( point_bub_ms( SEEX - 2, SEEY - 1 ), furn_f_rack );
                        furn_set( point_bub_ms( SEEX - 1, SEEY - 1 ), furn_f_rack );
                        furn_set( point_bub_ms( SEEX, SEEY - 1 ), furn_f_rack );
                        furn_set( point_bub_ms( SEEX + 1, SEEY - 1 ), furn_f_rack );
                        furn_set( point_bub_ms( SEEX - 2, SEEY ), furn_f_rack );
                        furn_set( point_bub_ms( SEEX - 1, SEEY ), furn_f_rack );
                        furn_set( point_bub_ms( SEEX, SEEY ), furn_f_rack );
                        furn_set( point_bub_ms( SEEX + 1, SEEY ), furn_f_rack );
                        line( this, ter_t_reinforced_door_glass_c, point_bub_ms( SEEX - 2, SEEY - 2 ),
                              point_bub_ms( SEEX + 1, SEEY - 2 ), dat.zlevel() );
                        line( this, ter_t_reinforced_door_glass_c, point_bub_ms( SEEX - 2, SEEY + 1 ),
                              point_bub_ms( SEEX + 1, SEEY + 1 ), dat.zlevel() );
                        line( this, ter_t_reinforced_glass, point_bub_ms( SEEX - 3, SEEY - 2 ), point_bub_ms( SEEX - 3,
                                SEEY + 1 ),
                              dat.zlevel() );
                        line( this, ter_t_reinforced_glass, point_bub_ms( SEEX + 2, SEEY - 2 ), point_bub_ms( SEEX + 2,
                                SEEY + 1 ),
                              dat.zlevel() );
                        place_items( Item_spawn_data_ammo_rare, 96,
                                     point_bub_ms( SEEX - 2, SEEY - 1 ),
                                     point_bub_ms( SEEX + 1, SEEY - 1 ), abs_sub.z(), false,
                                     calendar::start_of_cataclysm );
                        place_items( Item_spawn_data_guns_rare, 96, point_bub_ms( SEEX - 2, SEEY ),
                                     point_bub_ms( SEEX + 1, SEEY ), abs_sub.z(), false,
                                     calendar::start_of_cataclysm );
                        spawn_item( point_bub_ms( SEEX + 1, SEEY ), itype_plut_cell, rng( 1, 10 ) );
                    }
                    break;
                // Netherworld access
                case 3: {
                    bool monsters_end = false;
                    if( !one_in( 4 ) ) { // Trapped netherworld monsters
                        monsters_end = true;
                        tw = rng( SEEY + 3, SEEY + 5 );
                        bw = tw + 4;
                        lw = rng( SEEX - 6, SEEX - 2 );
                        rw = lw + 6;
                        for( int i = lw; i <= rw; i++ ) {
                            for( int j = tw; j <= bw; j++ ) {
                                if( j == tw || j == bw ) {
                                    if( ( i - lw ) % 2 == 0 ) {
                                        ter_set( point_bub_ms( i, j ), ter_t_concrete_wall );
                                    } else {
                                        ter_set( point_bub_ms( i, j ), ter_t_reinforced_glass );
                                    }
                                } else if( ( i - lw ) % 2 == 0 || j == tw + 2 ) {
                                    ter_set( point_bub_ms( i, j ), ter_t_concrete_wall );
                                } else { // Empty space holds monsters!
                                    place_spawns( GROUP_NETHER, 1, point_bub_ms( i, j ), point_bub_ms( i, j ), abs_sub.z(), 1, true );
                                }
                            }
                        }
                    }

                    spawn_item( point_bub_ms( SEEX - 1, 8 ), itype_id_science );
                    tmpcomp = add_computer( { SEEX,  8, abs_sub.z() },
                                            _( "Sub-prime contact console" ), 7 );
                    if( monsters_end ) { //only add these options when there are monsters.
                        tmpcomp->add_option( _( "Terminate Specimens" ), COMPACT_TERMINATE, 2 );
                        tmpcomp->add_option( _( "Release Specimens" ), COMPACT_RELEASE, 3 );
                    }
                    tmpcomp->add_option( _( "Toggle Portal" ), COMPACT_PORTAL, 8 );
                    tmpcomp->add_option( _( "Activate Resonance Cascade" ), COMPACT_CASCADE, 10 );
                    tmpcomp->add_failure( COMPFAIL_MANHACKS );
                    tmpcomp->add_failure( COMPFAIL_SECUBOTS );
                    tmpcomp->set_access_denied_msg(
                        _( "ERROR!  Access denied!  Unauthorized access will be met with lethal force!" ) );
                    ter_set( point_bub_ms( SEEX - 2, 4 ), ter_t_radio_tower );
                    ter_set( point_bub_ms( SEEX + 1, 4 ), ter_t_radio_tower );
                    ter_set( point_bub_ms( SEEX - 2, 7 ), ter_t_radio_tower );
                    ter_set( point_bub_ms( SEEX + 1, 7 ), ter_t_radio_tower );
                }
                break;

                // Bionics
                case 4: {
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point_bub_ms( 6, 6 ), point_bub_ms( 6, 6 ), abs_sub.z(), 1,
                                  true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point_bub_ms( SEEX * 2 - 7, 6 ),
                                  point_bub_ms( SEEX * 2 - 7, 6 ), abs_sub.z(), 1, true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point_bub_ms( 6, SEEY * 2 - 7 ),
                                  point_bub_ms( 6, SEEY * 2 - 7 ), abs_sub.z(), 1, true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point_bub_ms( SEEX * 2 - 7, SEEY * 2 - 7 ),
                                  point_bub_ms( SEEX * 2 - 7, SEEY * 2 - 7 ), abs_sub.z(), 1, true );
                    mtrap_set( this, tripoint_bub_ms( SEEX - 2, SEEY - 2, dat.zlevel() ), tr_dissector );
                    mtrap_set( this, tripoint_bub_ms( SEEX + 1, SEEY - 2, dat.zlevel() ), tr_dissector );
                    mtrap_set( this, tripoint_bub_ms( SEEX - 2, SEEY + 1, dat.zlevel() ), tr_dissector );
                    mtrap_set( this, tripoint_bub_ms( SEEX + 1, SEEY + 1, dat.zlevel() ), tr_dissector );
                    square_furn( this, furn_f_counter, point_bub_ms( SEEX - 1, SEEY - 1 ), point_bub_ms( SEEX, SEEY ) );
                    int item_count = 0;
                    while( item_count < 5 ) {
                        item_count +=
                            place_items(
                                Item_spawn_data_bionics, 75, point_bub_ms( SEEX - 1, SEEY - 1 ),
                                point_bub_ms( SEEX, SEEY ), abs_sub.z(), false, calendar::start_of_cataclysm ).size();
                    }
                    line( this, ter_t_reinforced_glass, point_bub_ms( SEEX - 2, SEEY - 2 ), point_bub_ms( SEEX + 1,
                            SEEY - 2 ),
                          dat.zlevel() );
                    line( this, ter_t_reinforced_glass, point_bub_ms( SEEX - 2, SEEY + 1 ), point_bub_ms( SEEX + 1,
                            SEEY + 1 ),
                          dat.zlevel() );
                    line( this, ter_t_reinforced_glass, point_bub_ms( SEEX - 2, SEEY - 1 ), point_bub_ms( SEEX - 2,
                            SEEY ),
                          dat.zlevel() );
                    line( this, ter_t_reinforced_glass, point_bub_ms( SEEX + 1, SEEY - 1 ), point_bub_ms( SEEX + 1,
                            SEEY ),
                          dat.zlevel() );
                    spawn_item( point_bub_ms( SEEX - 4, SEEY - 3 ), itype_id_science );
                    furn_set( point_bub_ms( SEEX - 3, SEEY - 3 ), furn_f_console );
                    tmpcomp = add_computer( { SEEX - 3,  SEEY - 3, abs_sub.z() },
                                            _( "Bionic access" ), 3 );
                    tmpcomp->add_option( _( "Manifest" ), COMPACT_LIST_BIONICS, 0 );
                    tmpcomp->add_option( _( "Open Chambers" ), COMPACT_RELEASE, 5 );
                    tmpcomp->add_failure( COMPFAIL_MANHACKS );
                    tmpcomp->add_failure( COMPFAIL_SECUBOTS );
                    tmpcomp->set_access_denied_msg(
                        _( "ERROR!  Access denied!  Unauthorized access will be met with lethal force!" ) );
                }
                break;

                // CVD Forge
                case 5:
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point_bub_ms( 6, 6 ), point_bub_ms( 6, 6 ), dat.zlevel(), 1,
                                  true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point_bub_ms( SEEX * 2 - 7, 6 ),
                                  point_bub_ms( SEEX * 2 - 7, 6 ), dat.zlevel(), 1, true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point_bub_ms( 6, SEEY * 2 - 7 ),
                                  point_bub_ms( 6, SEEY * 2 - 7 ), dat.zlevel(), 1, true );
                    place_spawns( GROUP_ROBOT_SECUBOT, 1, point_bub_ms( SEEX * 2 - 7, SEEY * 2 - 7 ),
                                  point_bub_ms( SEEX * 2 - 7, SEEY * 2 - 7 ), dat.zlevel(), 1, true );
                    line( this, ter_t_cvdbody, point_bub_ms( SEEX - 2, SEEY - 2 ), point_bub_ms( SEEX - 2, SEEY + 1 ),
                          dat.zlevel() );
                    line( this, ter_t_cvdbody, point_bub_ms( SEEX - 1, SEEY - 2 ), point_bub_ms( SEEX - 1, SEEY + 1 ),
                          dat.zlevel() );
                    line( this, ter_t_cvdbody, point_bub_ms( SEEX, SEEY - 1 ), point_bub_ms( SEEX, SEEY + 1 ),
                          dat.zlevel() );
                    line( this, ter_t_cvdbody, point_bub_ms( SEEX + 1, SEEY - 2 ), point_bub_ms( SEEX + 1, SEEY + 1 ),
                          dat.zlevel() );
                    ter_set( point_bub_ms( SEEX, SEEY - 2 ), ter_t_cvdmachine );
                    spawn_item( point_bub_ms( SEEX, SEEY - 3 ), itype_id_science );
                    break;
            }
        } // end use_hardcoded_lab_finale

        // Handle stairs in the unlikely case they are needed.

        const auto maybe_insert_stairs = [this]( const oter_id & terrain,
        const ter_str_id & t_stair_type ) {
            if( is_ot_match( "stairs", terrain, ot_match_type::contains ) ) {
                const auto predicate = [this]( const tripoint_bub_ms & p ) {
                    return ter( p ) == ter_t_thconc_floor && furn( p ) == furn_str_id::NULL_ID() &&
                           tr_at( p ).is_null();
                };
                const tripoint_range<tripoint_bub_ms> range = points_in_rectangle( tripoint_bub_ms{ 0, 0, abs_sub.z() },
                { SEEX * 2 - 2, SEEY * 2 - 2, abs_sub.z() } );
                if( const auto p = random_point( range, predicate ) ) {
                    ter_set( *p, t_stair_type );
                }
            }
        };
        maybe_insert_stairs( dat.above(), ter_t_stairs_up );
        maybe_insert_stairs( terrain_type, ter_t_stairs_down );

        int light_odds = 0;
        // central labs are always fully lit, other labs have half chance of some lights.
        if( central_lab ) {
            light_odds = 1;
        } else if( one_in( 2 ) ) {
            light_odds = std::pow( rng( 1, 12 ), 1.6 );
        }
        if( light_odds > 0 ) {
            for( int i = 0; i < SEEX * 2; i++ ) {
                for( int j = 0; j < SEEY * 2; j++ ) {
                    if( !( ( i * j ) % 2 || ( i + j ) % 4 ) && one_in( light_odds ) ) {
                        const ter_id &nearby_ter = ter( point_bub_ms( i, j ) );
                        if( nearby_ter == ter_t_thconc_floor || nearby_ter == ter_t_strconc_floor ) {
                            ter_set( point_bub_ms( i, j ), ter_t_thconc_floor_olight );
                        }
                    }
                }
            }
        }
    }
}

void map::place_spawns( const mongroup_id &group, const int chance,
                        const point_bub_ms &p1, const point_bub_ms &p2, const int z_level, const float density,
                        const bool individual, const bool friendly, const std::optional<std::string> &name,
                        const int mission_id )
{
    if( !group.is_valid() ) {
        tripoint_abs_omt omt = project_to<coords::omt>( get_abs_sub() );
        omt.z() = z_level;
        const oter_id &oid = overmap_buffer.ter( omt );
        debugmsg( "place_spawns: invalid mongroup '%s', om_terrain = '%s' (%s)", group.c_str(),
                  oid.id().c_str(), oid->get_mapgen_id().c_str() );
        return;
    }

    // Set chance to be 1 or less to guarantee spawn, else set higher than 1
    if( !one_in( chance ) ) {
        return;
    }

    float spawn_density = 1.0f;
    if( MonsterGroupManager::is_animal( group ) ) {
        spawn_density = get_option< float >( "SPAWN_ANIMAL_DENSITY" );
    } else {
        spawn_density = get_option< float >( "SPAWN_DENSITY" );
    }

    float multiplier = density * spawn_density;
    // Only spawn 1 creature if individual flag set, else scale by density
    float thenum = individual ? 1 : ( multiplier * rng_float( 10.0f, 50.0f ) );
    int num = roll_remainder( thenum );

    // GetResultFromGroup decrements num
    while( num > 0 ) {
        int tries = 10;
        point_bub_ms p;

        // Pick a spot for the spawn
        do {
            p = { rng( p1.x(), p2.x() ), rng( p1.y(), p2.y() ) };
            tries--;
        } while( impassable( p ) && tries > 0 );

        // Pick a monster type
        std::vector<MonsterGroupResult> spawn_details =
            MonsterGroupManager::GetResultFromGroup( group, &num );
        for( const MonsterGroupResult &mgr : spawn_details ) {
            add_spawn( mgr.id, mgr.pack_size, { p, abs_sub.z() },
                       friendly, -1, mission_id, name, mgr.data );
        }
    }
}

void map::place_gas_pump( const tripoint_bub_ms &p, int charges, const itype_id &fuel_type )
{
    item fuel( fuel_type, calendar::start_of_cataclysm );
    fuel.charges = charges;
    add_item( p, fuel );
    ter_set( p, ter_id( fuel.fuel_pump_terrain() ) );
}

void map::place_gas_pump( const tripoint_bub_ms &p, int charges )
{
    place_gas_pump( p, charges, one_in( 4 ) ? itype_diesel : itype_gasoline );
}

void map::place_toilet( const tripoint_bub_ms &p, int charges )
{
    item water( itype_water, calendar::start_of_cataclysm );
    water.charges = charges;
    add_item( p, water );
    furn_set( p, furn_f_toilet );
}

void map::place_vending( const tripoint_bub_ms &p, const item_group_id &type, bool reinforced,
                         bool lootable, bool powered, bool networked )
{
    if( !powered ) {
        if( reinforced ) {
            furn_set( p, furn_f_vending_reinforced_off );
        } else {
            furn_set( p, furn_f_vending_c_off );
        }
    } else {
        if( reinforced ) {
            if( networked ) {
                furn_set( p, furn_f_vending_reinforced_networked );
            } else {
                furn_set( p, furn_f_vending_reinforced );
            }
        } else {
            if( networked ) {
                furn_set( p, furn_f_vending_c_networked );
            } else {
                furn_set( p, furn_f_vending_c );
            }

        }
    }
    // The chance to find a non-ransacked vending machine reduces greatly with every day after the Cataclysm,
    // unless it's hidden somewhere far away from everyone's eyes (e.g. deep in the lab)
    if( lootable &&
        !one_in( std::max( to_days<int>( calendar::turn - calendar::start_of_cataclysm ), 0 ) + 4 ) ) {
        bash( p, 9999 );
        for( const tripoint_bub_ms &loc : points_in_radius( p, 1 ) ) {
            if( one_in( 4 ) ) {
                spawn_item( loc, itype_glass_shard, rng( 1, 25 ) );
            }
        }
    } else {
        place_items( type, 100, p, p, false, calendar::start_of_cataclysm );
    }
}

character_id map::place_npc( const point_bub_ms &p, const string_id<npc_template> &type )
{
    shared_ptr_fast<npc> temp = make_shared_fast<npc>();
    temp->normalize();
    temp->load_npc_template( type );
    temp->spawn_at_precise( get_abs( { p, abs_sub.z() } ) );
    temp->toggle_trait( trait_NPC_STATIC_NPC );
    overmap_buffer.insert_npc( temp );
    return temp->getID();
}

void map::apply_faction_ownership( const point_bub_ms &p1, const point_bub_ms &p2,
                                   const faction_id &id )
{
    for( const tripoint_bub_ms &p : points_in_rectangle( tripoint_bub_ms( p1, abs_sub.z() ),
            tripoint_bub_ms( p2,
                             abs_sub.z() ) ) ) {
        map_stack items = i_at( p.xy() );
        for( item &elem : items ) {
            elem.set_owner( id );
        }
        vehicle *source_veh = veh_pointer_or_null( veh_at( p ) );
        if( source_veh ) {
            if( !source_veh->has_owner() ) {
                source_veh->set_owner( id );
            }
        }
    }
}

// A chance of 100 indicates that items should always spawn,
// the item group should be responsible for determining the amount of items.
std::vector<item *> map::place_items(
    const item_group_id &group_id, const int chance, const tripoint_bub_ms &p1,
    const tripoint_bub_ms &p2, const bool ongrass, const time_point &turn, const int magazine,
    const int ammo, const std::string &faction )
{
    std::vector<item *> res;

    if( chance > 100 || chance <= 0 ) {
        debugmsg( "map::place_items() called with an invalid chance (%d)", chance );
        return res;
    }
    if( !item_group::group_is_defined( group_id ) ) {
        const tripoint_abs_omt omt = project_to<coords::omt>( get_abs_sub() );
        const oter_id &oid = overmap_buffer.ter( omt );
        debugmsg( "place_items: invalid item group '%s', om_terrain = '%s' (%s)",
                  group_id.c_str(), oid.id().c_str(), oid->get_mapgen_id().c_str() );
        return res;
    }

    // spawn rates < 1 are handled in item_group
    const float spawn_rate = std::max( get_option<float>( "ITEM_SPAWNRATE" ), 1.0f );
    const int spawn_count = roll_remainder( chance * spawn_rate / 100.0f );
    for( int i = 0; i < spawn_count; i++ ) {
        // Might contain one item or several that belong together like guns & their ammo
        int tries = 0;
        auto is_valid_terrain = [this, ongrass]( const tripoint_bub_ms & p ) {
            const ter_t &terrain = ter( p ).obj();
            return terrain.movecost == 0 &&
                   !terrain.has_flag( ter_furn_flag::TFLAG_PLACE_ITEM ) &&
                   !ongrass &&
                   !terrain.has_flag( ter_furn_flag::TFLAG_FLAT );
        };

        tripoint_bub_ms p;
        do {
            p.x() = rng( p1.x(), p2.x() );
            p.y() = rng( p1.y(), p2.y() );
            p.z() = rng( p1.z(), p2.z() );
            tries++;
        } while( is_valid_terrain( p ) && tries < 20 );
        if( tries < 20 ) {
            auto add_res_itm = [this, &p, &res]( const item & itm ) {
                item &it = add_item_or_charges( p, itm );
                if( !it.is_null() ) {
                    res.push_back( &it );
                }
            };

            for( const item &itm : item_group::items_from( group_id, turn, spawn_flags::use_spawn_rate ) ) {
                const float item_cat_spawn_rate = std::max( 0.0f, item_category_spawn_rate( itm ) );
                if( item_cat_spawn_rate == 0.0f ) {
                    continue;
                } else if( item_cat_spawn_rate < 1.0f ) {
                    // for items with category spawn rate less than 1, roll a dice to see if they should spawn
                    if( rng_float( 0.0f, 1.0f ) <= item_cat_spawn_rate ) {
                        add_res_itm( itm );
                    }
                } else {
                    // for items with category spawn rate more or equal to 1, spawn item at least one time...
                    add_res_itm( itm );

                    if( item_cat_spawn_rate > 1.0f ) {
                        // ...then create a list with items from the same item group and remove all items with differing category from that list
                        // so if the original item was e.g. from 'guns' category, the list will contain only items from the 'guns' category...
                        Item_list extra_spawn = item_group::items_from( group_id, turn, spawn_flags::use_spawn_rate );
                        extra_spawn.erase( std::remove_if( extra_spawn.begin(), extra_spawn.end(), [&itm]( item & it ) {
                            return it.get_category_of_contents() != itm.get_category_of_contents();
                        } ), extra_spawn.end() );

                        // ...then add a chance to spawn additional items from the list, amount is based on the item category spawn rate
                        for( int i = 0; i < item_cat_spawn_rate; i++ ) {
                            for( const item &it : extra_spawn ) {
                                if( rng( 1, 100 ) <= chance ) {
                                    add_res_itm( it );
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    for( item *e : res ) {
        if( e->is_tool() || e->is_gun() || e->is_magazine() ) {
            if( rng( 0, 99 ) < magazine && e->magazine_default() && !e->magazine_integral() &&
                !e->magazine_current() ) {
                e->put_in( item( e->magazine_default(), e->birthday() ), pocket_type::MAGAZINE_WELL );
            }
            if( rng( 0, 99 ) < ammo && e->ammo_default() && e->ammo_remaining( ) == 0 ) {
                e->ammo_set( e->ammo_default() );
            }
        }

        e->randomize_rot();
        e->set_owner( faction_id( faction ) );
    }
    return res;
}

std::vector<item *> map::put_items_from_loc( const item_group_id &group_id,
        const tripoint_bub_ms &p,
        const time_point &turn )
{
    const std::vector<item> items =
        item_group::items_from( group_id, turn, spawn_flags::use_spawn_rate );
    return spawn_items( p, items );
}

void map::add_spawn( const MonsterGroupResult &spawn_details, const tripoint_bub_ms &p )
{
    add_spawn( spawn_details.id, spawn_details.pack_size, p, false, -1, -1, std::nullopt,
               spawn_details.data );
}

void map::add_spawn( const mtype_id &type, int count, const tripoint_bub_ms &p, bool friendly,
                     int faction_id, int mission_id, const std::optional<std::string> &name )
{
    add_spawn( type, count, p, friendly, faction_id, mission_id, name, spawn_data() );
}

void map::add_spawn(
    const mtype_id &type, int count, const tripoint_bub_ms &p, bool friendly, int faction_id,
    int mission_id, const std::optional<std::string> &name, const spawn_data &data )
{
    if( p.x() < 0 || p.x() >= SEEX * my_MAPSIZE || p.y() < 0 || p.y() >= SEEY * my_MAPSIZE ) {
        debugmsg( "Out of bounds add_spawn(%s, %d, %d, %d)", type.c_str(), count, p.x(), p.y() );
        return;
    }
    point_sm_ms offset;
    submap *place_on_submap = get_submap_at( p, offset );
    if( place_on_submap == nullptr ) {
        debugmsg( "Tried to add spawn at (%d,%d) but the submap is not loaded", offset.x(), offset.y() );
        return;
    }

    if( !place_on_submap ) {
        debugmsg( "centadodecamonant doesn't exist in grid; within add_spawn(%s, %d, %d, %d, %d)",
                  type.c_str(), count, p.x(), p.y(), p.z() );
        return;
    }
    if( MonsterGroupManager::monster_is_blacklisted( type ) ) {
        return;
    }
    place_on_submap->spawns.emplace_back( type, count, offset, faction_id, mission_id, friendly, name,
                                          data );
}

vehicle *map::add_vehicle( const vproto_id &type, const tripoint_bub_ms &p, const units::angle &dir,
                           const int veh_fuel, const int veh_status, const bool merge_wrecks,
                           const bool force_status/* = false*/ )
{
    if( !type.is_valid() ) {
        debugmsg( "Nonexistent vehicle type: \"%s\"", type.c_str() );
        return nullptr;
    }
    if( !inbounds( p ) ) {
        dbg( D_WARNING ) << string_format( "Out of bounds add_vehicle t=%s d=%d p=%d,%d,%d",
                                           type.str(), to_degrees( dir ), p.x(), p.y(), p.z() );
        return nullptr;
    }

    std::unique_ptr<vehicle> veh = std::make_unique<vehicle>( type );
    tripoint_bub_ms p_ms( p );
    tripoint_bub_sm quotient;
    point_sm_ms remainder;
    std::tie( quotient, remainder ) = coords::project_remain<coords::sm>( p_ms );
    veh->sm_pos = abs_sub.xy() + rebase_rel( quotient );
    veh->pos = remainder;
    veh->init_state( *this, veh_fuel, veh_status, force_status );
    veh->place_spawn_items( *this );
    veh->face.init( dir );
    veh->turn_dir = dir;
    // for backwards compatibility, we always spawn with a pivot point of (0,0) so
    // that the mount at (0,0) is located at the spawn position.
    veh->precalc_mounts( 0, dir, point_rel_ms::zero );

    std::unique_ptr<vehicle> placed_vehicle_up =
        add_vehicle_to_map( std::move( veh ), merge_wrecks );
    vehicle *placed_vehicle = placed_vehicle_up.get();

    if( placed_vehicle != nullptr ) {
        submap *place_on_submap = get_submap_at_grid( placed_vehicle->sm_pos - abs_sub.xy() );
        if( place_on_submap == nullptr ) {
            debugmsg( "Tried to add vehicle at %s but the submap is not loaded",
                      placed_vehicle->sm_pos.to_string() );
            return placed_vehicle;
        }
        place_on_submap->ensure_nonuniform();
        place_on_submap->vehicles.push_back( std::move( placed_vehicle_up ) );
        invalidate_max_populated_zlev( p.z() );

        level_cache &ch = get_cache( placed_vehicle->sm_pos.z() );
        ch.vehicle_list.insert( placed_vehicle );
        add_vehicle_to_cache( placed_vehicle );

        rebuild_vehicle_level_caches();
        set_pathfinding_cache_dirty( p.z() );
        placed_vehicle->place_zones( *this );
    }
    return placed_vehicle;
}

/**
 * Takes a vehicle already created with new and attempts to place it on the map,
 * checking for collisions. If the vehicle can't be placed, returns NULL,
 * otherwise returns a pointer to the placed vehicle, which may not necessarily
 * be the one passed in (if wreckage is created by fusing cars).
 * @param veh The vehicle to place on the map.
 * @param merge_wrecks Whether crashed vehicles intermix parts
 * @return The vehicle that was finally placed.
 */
std::unique_ptr<vehicle> map::add_vehicle_to_map(
    std::unique_ptr<vehicle> veh_to_add, const bool merge_wrecks )
{
    //We only want to check once per square, so loop over all structural parts
    std::vector<int> frame_indices = veh_to_add->all_parts_at_location( "structure" );

    //Check for boat type vehicles that should be placeable in deep water
    const bool can_float = size( veh_to_add->get_avail_parts( "FLOATS" ) ) > 2;

    //When hitting a wall, only smash the vehicle once (but walls many times)
    bool needs_smashing = false;

    for( std::vector<int>::const_iterator part = frame_indices.begin();
         part != frame_indices.end(); part++ ) {
        const tripoint_bub_ms p = veh_to_add->bub_part_pos( *this, *part );

        if( veh_to_add->part( *part ).is_fake ) {
            continue;
        }
        //Don't spawn anything in water
        if( has_flag_ter( ter_furn_flag::TFLAG_DEEP_WATER, p ) && !can_float ) {
            return nullptr;
        }

        // Don't spawn shopping carts on top of another vehicle or other obstacle.
        if( veh_to_add->type == vehicle_prototype_shopping_cart ) {
            if( veh_at( p ) || impassable( p ) ) {
                return nullptr;
            }
        }

        //For other vehicles, simulate collisions with (non-shopping cart) stuff
        vehicle *const first_veh = veh_pointer_or_null( veh_at( p ) );
        if( first_veh != nullptr && first_veh->type != vehicle_prototype_shopping_cart ) {
            if( !merge_wrecks ) {
                return nullptr;
            }

            // Hard wreck-merging limit: 200 tiles
            // Merging is slow for big vehicles which lags the mapgen
            if( frame_indices.size() + first_veh->all_parts_at_location( "structure" ).size() > 200 ) {
                return nullptr;
            }

            /**
             * attempt to pull parts from `veh_to_add` (the prospective new vehicle)
             * in order to place them into `first_veh` (the vehicle that is overlapped)
             *
             * the intent here is to get something similar to the old wreckage behavior
             * (combining two vehicles) without completely garbling all of the vehicle parts
             * for tile rendering purposes.
             *
             * the overlap span is still a mess, though.
             */
            std::unique_ptr<RemovePartHandler> handler_ptr;
            bool did_merge = false;
            for( const tripoint_abs_ms &map_pos : first_veh->get_points( true ) ) {
                std::vector<vehicle_part *> parts_to_move = veh_to_add->get_parts_at( map_pos, "",
                        part_status_flag::any );
                if( !parts_to_move.empty() ) {
                    // Store target_point by value because first_veh->parts may reallocate
                    // to a different address after install_part()
                    std::vector<vehicle_part *> first_veh_parts = first_veh->get_parts_at( map_pos, "",
                            part_status_flag:: any );
                    // This happens if this location is occupied by a fake part.
                    if( first_veh_parts.empty() || first_veh_parts.front()->is_fake ) {
                        continue;
                    }
                    if( !inbounds( map_pos ) ) {
                        debugmsg( "Existing vehicle %s (%s; origin %s; rot %g) and "
                                  "new vehicle %s (%s; origin %s; rot %g) "
                                  "out of map bounds at %s",
                                  first_veh->name, first_veh->type.str(),
                                  first_veh->pos_abs().to_string(),
                                  to_degrees( first_veh->turn_dir ),
                                  veh_to_add->name, veh_to_add->type.str(),
                                  veh_to_add->pos_abs().to_string(),
                                  to_degrees( veh_to_add->turn_dir ),
                                  get_bub( map_pos ).to_string() );
                    }
                    did_merge = true;
                    const point_rel_ms target_point = first_veh_parts.front()->mount;
                    const point_rel_ms source_point = parts_to_move.front()->mount;
                    for( const vehicle_part *vp : parts_to_move ) {
                        const vpart_info &vpi = vp->info();
                        // TODO: change mount points to be tripoint
                        const ret_val<void> valid_mount = first_veh->can_mount( target_point, vpi );
                        if( valid_mount.success() ) {
                            // make a copy so we don't interfere with veh_to_add->remove_part below
                            first_veh->install_part( *this, target_point, vehicle_part( *vp ) );
                        }
                        // ignore parts that would make invalid vehicle configuration
                    }

                    if( !handler_ptr ) {
                        // This is a heuristic: we just assume the default handler is good enough when called
                        // on the main game map. And assume that we run from some mapgen code if called on
                        // another instance.
                        // TODO: Update logic to be able to work outside of mapgen and in the reality bubble.
                        if( !g || &reality_bubble() != this ) {
                            handler_ptr = std::make_unique<MapgenRemovePartHandler>( *this );
                        }
                    }
                    // this could probably be done in a single loop with installing parts above
                    std::vector<int> parts_in_square = veh_to_add->parts_at_relative( source_point, true );
                    std::set<int> parts_to_check;
                    for( int index = parts_in_square.size() - 1; index >= 0; index-- ) {
                        vehicle_part &vp = veh_to_add->part( parts_in_square[index] );
                        if( handler_ptr ) {
                            veh_to_add->remove_part( vp, *handler_ptr );
                        } else {
                            veh_to_add->remove_part( vp );
                        }
                        parts_to_check.insert( parts_in_square[index] );
                    }
                    veh_to_add->find_and_split_vehicles( *this, parts_to_check );
                }
            }

            if( did_merge ) {
                // TODO: more targeted damage around the impact site
                first_veh->smash( *this );
                // TODO: entangle the old vehicle and the new vehicle somehow, perhaps with tow cables
                // or something like them, to make them harder to separate
                std::unique_ptr<vehicle> new_veh = add_vehicle_to_map( std::move( veh_to_add ), true );
                if( new_veh != nullptr ) {
                    new_veh->smash( *this );
                    return new_veh;
                }
                return nullptr;
            }
        } else if( impassable( p ) ) {
            if( !merge_wrecks ) {
                return nullptr;
            }

            // There's a wall or other obstacle here; destroy it
            destroy( p, true );

            // Some weird terrain, don't place the vehicle
            if( impassable( p ) ) {
                return nullptr;
            }

            needs_smashing = true;
        }
    }

    if( needs_smashing ) {
        veh_to_add->smash( *this );
    }

    veh_to_add->refresh( );
    return veh_to_add;
}

computer *map::add_computer( const tripoint_bub_ms &p, const std::string &name, int security )
{
    // TODO: Turn this off?
    furn_set( p, furn_f_console );
    point_sm_ms l;
    submap *const place_on_submap = get_submap_at( p, l );
    if( place_on_submap == nullptr ) {
        debugmsg( "Tried to add computer at (%d,%d) but the submap is not loaded", l.x(), l.y() );
        static computer null_computer = computer( name, security, *this, p );
        return &null_computer;
    }
    place_on_submap->set_computer( l, computer( name, security, *this, p ) );
    return place_on_submap->get_computer( l );
}

/**
 * Rotates this map, and all of its contents, by the specified multiple of 90
 * degrees.
 * @param turns How many 90-degree turns to rotate the map.
 */
void map::rotate( int turns )
{
    if( this->my_MAPSIZE != 2 ) {
        debugmsg( "map::rotate called with map too large to be rotated properly.  Only the top left overmap will be rotated." );
    }

    //Handle anything outside the 1-3 range gracefully; rotate(0) is a no-op.
    turns = turns % 4;
    if( turns == 0 ) {
        return;
    }

    const tripoint_abs_sm &abs_sub = get_abs_sub();
    const tripoint_abs_omt abs_omt = project_to<coords::omt>( abs_sub );

    if( abs_sub.x() % 2 != 0 || abs_sub.y() % 2 != 0 ) {
        debugmsg( "map::rotate called with map not aligned with overmap boundary.  Results will be incorrect at best." );
    }

    const std::vector<shared_ptr_fast<npc>> npcs = overmap_buffer.get_npcs_near_omt( abs_omt, 0 );
    for( const shared_ptr_fast<npc> &i : npcs ) {
        npc &np = *i;
        const tripoint_abs_ms sq( np.pos_abs() );

        if( sq.z() != abs_sub.z() && !zlevels ) {
            continue;
        }

        const point_bub_ms old( get_bub( np.pos_abs().xy() ) );

        const point_bub_ms new_pos( old.rotate( turns, {SEEX * 2, SEEY * 2} ) );
        np.spawn_at_precise( get_abs( tripoint_bub_ms( new_pos, sq.z() ) ) );
    }

    clear_vehicle_level_caches();

    // rotate zones
    zone_manager &mgr = zone_manager::get_manager();
    mgr.rotate_zones( *this, turns );

    const int bottom_level = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
    const int top_level = zlevels ? OVERMAP_HEIGHT : abs_sub.z();

    for( int z_level = bottom_level; z_level <= top_level; z_level++ ) {
        clear_vehicle_list( z_level );

        submap *pz = get_submap_at_grid( tripoint_rel_sm{ point_rel_sm::zero, z_level } );
        submap *pse = get_submap_at_grid( tripoint_rel_sm{ point_rel_sm::south_east, z_level } );
        submap *pe = get_submap_at_grid( tripoint_rel_sm{ point_rel_sm::east, z_level } );
        submap *ps = get_submap_at_grid( tripoint_rel_sm{ point_rel_sm::south, z_level } );
        if( pz == nullptr || pse == nullptr || pe == nullptr || ps == nullptr ) {
            debugmsg( "Tried to rotate map at (%d,%d) but the submap is not loaded", point::zero.x,
                      point::zero.y );
            return;
        }

        // Move the submaps around.
        if( turns == 2 ) {
            std::swap( *pz, *pse );
            std::swap( *pe, *ps );
        } else {
            point_rel_sm p;
            submap tmp;

            std::swap( *pse, tmp );

            for( int k = 0; k < 4; ++k ) {
                p = p.rotate( turns, { 2, 2 } );
                point_rel_sm tmpp = point_rel_sm::south_east - p;
                submap *psep = get_submap_at_grid( tripoint_rel_sm( tmpp, z_level ) );
                if( psep == nullptr ) {
                    debugmsg( "Tried to rotate map at (%d,%d) but the submap is not loaded", tmpp.x(), tmpp.y() );
                    continue;
                }
                std::swap( *psep, tmp );
            }
        }

        // Then rotate them and recalculate vehicle positions.
        for( int j = 0; j < 2; ++j ) {
            for( int i = 0; i < 2; ++i ) {
                point_rel_sm p( i, j );
                submap *sm = get_submap_at_grid( tripoint_rel_sm( p, z_level ) );
                if( sm == nullptr ) {
                    debugmsg( "Tried to rotate map at (%d,%d) but the submap is not loaded", p.x(), p.y() );
                    continue;
                }

                sm->rotate( turns );

                for( auto &veh : sm->vehicles ) {
                    veh->sm_pos = { abs_sub.xy() + p, z_level };
                }

                update_vehicle_list( sm, z_level );
            }
        }
        rebuild_vehicle_level_caches();

        std::unordered_map<std::string, tripoint_abs_ms> temp_points = queued_points;
        queued_points.clear();
        for( std::pair<const std::string, tripoint_abs_ms> &queued_point : temp_points ) {
            //This is all just a copy of the section rotating NPCs above
            const point_abs_omt queued_point_omt( project_to<coords::omt>( queued_point.second.xy() ) );

            // Note: We are rotating the entire overmap square (2x2 of submaps)
            if( queued_point_omt != abs_omt.xy() || ( queued_point.second.z() != abs_sub.z() && !zlevels ) ) {
                continue;
            }
            const point_abs_sm queued_point_sm( project_to<coords::sm>( queued_point.second.xy() ) );
            const point_bub_ms queued_point_bub( reality_bubble().get_bub( queued_point.second.xy() ) );
            point_bub_ms old( queued_point_bub.x() % SEEX, queued_point_bub.y() % SEEY );

            if( queued_point_sm.x() % 2 != 0 ) {
                old.x() += SEEX;
            }
            if( queued_point_sm.y() % 2 != 0 ) {
                old.y() += SEEY;
            }
            const point_bub_ms new_pos( old.rotate( turns, {SEEX * 2, SEEY * 2} ) );
            queued_points[queued_point.first] = get_abs( tripoint_bub_ms( new_pos,
                                                queued_point.second.z() ) );
        }
    }
}

/**
 * Mirrors this map, and all of its contents along with all its contents in the
 * directions specified.
 */
void map::mirror( bool mirror_horizontal, bool mirror_vertical )
{
    if( !mirror_horizontal && !mirror_vertical ) {
        return;
    }

    const tripoint_abs_sm &abs_sub = get_abs_sub();

    for( int z_level = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
         z_level <= ( zlevels ? OVERMAP_HEIGHT : abs_sub.z() ); z_level++ ) {
        submap *pz = get_submap_at_grid( tripoint_rel_sm{ point_rel_sm::zero, z_level } );
        submap *pse = get_submap_at_grid( tripoint_rel_sm{ point_rel_sm::south_east, z_level } );
        submap *pe = get_submap_at_grid( tripoint_rel_sm{ point_rel_sm::east, z_level
                                                        } );
        submap *ps = get_submap_at_grid( tripoint_rel_sm{ point_rel_sm::south, z_level } );
        if( pz == nullptr || pse == nullptr || pe == nullptr || ps == nullptr ) {
            debugmsg( "Tried to mirror map at (%d, %d, %d) but the submap is not loaded", point::zero.x,
                      point::zero.y, z_level );
            return;
        }

        // Move the submaps around. Note that the order doesn't matter as the outcome is the same.
        if( mirror_horizontal ) {
            std::swap( *pz, *pe );
            std::swap( *ps, *pse );
        }
        if( mirror_vertical ) {
            std::swap( *pz, *ps );
            std::swap( *pe, *pse );
        }

        // Then mirror them.
        for( int j = 0; j < 2; ++j ) {
            for( int i = 0; i < 2; ++i ) {
                point_rel_sm p( i, j );
                submap *sm = get_submap_at_grid( tripoint_rel_sm{ p, z_level } );
                if( sm == nullptr ) {
                    debugmsg( "Tried to mirror map at (%d, %d, %d) but the submap is not loaded", p.x(), p.y(),
                              z_level );
                    continue;
                }

                if( mirror_horizontal ) {
                    sm->mirror( true );
                }
                if( mirror_vertical ) {
                    sm->mirror( false );
                }
            }
        }
    }
}

// Hideous function, I admit...
bool connects_to( const oter_id &there, int dir )
{
    switch( dir ) {
        // South
        case 2:
            if( there == oter_ants_ns || there == oter_ants_es ||
                there == oter_ants_sw || there == oter_ants_nes || there == oter_ants_nsw ||
                there == oter_ants_esw || there == oter_ants_nesw ) {
                return true;
            }
            return false;
        // West
        case 3:
            if( there == oter_ants_ew || there == oter_ants_sw ||
                there == oter_ants_wn || there == oter_ants_new || there == oter_ants_nsw ||
                there == oter_ants_esw || there == oter_ants_nesw ) {
                return true;
            }
            return false;
        // North
        case 0:
            if( there == oter_ants_ns || there == oter_ants_ne ||
                there == oter_ants_wn || there == oter_ants_nes || there == oter_ants_new ||
                there == oter_ants_nsw || there == oter_ants_nesw ) {
                return true;
            }
            return false;
        // East
        case 1:
            if( there == oter_ants_ew || there == oter_ants_ne ||
                there == oter_ants_es || there == oter_ants_nes || there == oter_ants_new ||
                there == oter_ants_esw || there == oter_ants_nesw ) {
                return true;
            }
            return false;
        default:
            debugmsg( "Connects_to with dir of %d", dir );
            return false;
    }
}

void science_room( map *m, const point_bub_ms &p1, const point_bub_ms &p2, int z, int rotate )
{
    int height = p2.y() - p1.y();
    int width  = p2.x() - p1.x();
    if( rotate % 2 == 1 ) { // Swap width & height if we're a lateral room
        int tmp = height;
        height  = width;
        width   = tmp;
    }
    for( int i = p1.x(); i <= p2.x(); i++ ) {
        for( int j = p1.y(); j <= p2.y(); j++ ) {
            m->ter_set( point_bub_ms( i, j ), ter_t_thconc_floor );
        }
    }
    int area = height * width;
    std::vector<room_type> valid_rooms;
    if( height < 5 && width < 5 ) {
        valid_rooms.push_back( room_closet );
    }
    if( height > 6 && width > 3 ) {
        valid_rooms.push_back( room_lobby );
    }
    if( height > 4 || width > 4 ) {
        valid_rooms.push_back( room_chemistry );
        valid_rooms.push_back( room_goo );
    }
    if( z != 0 && ( height > 7 || width > 7 ) && height > 2 && width > 2 ) {
        valid_rooms.push_back( room_teleport );
    }
    if( height > 7 && width > 7 ) {
        valid_rooms.push_back( room_bionics );
        valid_rooms.push_back( room_cloning );
    }
    if( area >= 9 ) {
        valid_rooms.push_back( room_vivisect );
    }
    if( height > 5 && width > 4 ) {
        valid_rooms.push_back( room_dorm );
    }
    if( width > 8 ) {
        for( int i = 8; i < width; i += rng( 1, 2 ) ) {
            valid_rooms.push_back( room_split );
        }
    }

    tripoint_bub_ms trap( rng( p1.x() + 1, p2.x() - 1 ), rng( p1.y() + 1, p2.y() - 1 ), z );
    switch( random_entry( valid_rooms ) ) {
        case room_closet:
            m->place_items( Item_spawn_data_cleaning, 80, p1, p2, z, false,
                            calendar::start_of_cataclysm );
            break;
        case room_lobby:
            if( rotate % 2 == 0 ) { // Vertical
                int desk = p1.y() + rng( static_cast<int>( height / 2 ) - static_cast<int>( height / 4 ),
                                         static_cast<int>( height / 2 ) + 1 );
                for( int x = p1.x() + static_cast<int>( width / 4 ); x < p2.x() - static_cast<int>( width / 4 );
                     x++ ) {
                    m->furn_set( point_bub_ms( x, desk ), furn_f_counter );
                }
                computer *tmpcomp = m->add_computer( { p2.x() - static_cast<int>( width / 4 ), desk, z},
                                                     _( "Log Console" ), 3 );
                tmpcomp->add_option( _( "View Research Logs" ), COMPACT_RESEARCH, 0 );
                tmpcomp->add_option( _( "Download Map Data" ), COMPACT_MAPS, 0 );
                tmpcomp->add_failure( COMPFAIL_SHUTDOWN );
                tmpcomp->add_failure( COMPFAIL_ALARM );
                tmpcomp->add_failure( COMPFAIL_DAMAGE );
                m->place_spawns( GROUP_LAB_SECURITY, 1,
                                 point_bub_ms( static_cast<int>( ( p1.x() + p2.x() ) / 2 ), desk ),
                                 point_bub_ms( static_cast<int>( ( p1.x() + p2.x() ) / 2 ), desk ), z, 1, true );
            } else {
                int desk = p1.x() + rng( static_cast<int>( height / 2 ) - static_cast<int>( height / 4 ),
                                         static_cast<int>( height / 2 ) + 1 );
                for( int y = p1.y() + static_cast<int>( width / 4 ); y < p2.y() - static_cast<int>( width / 4 );
                     y++ ) {
                    m->furn_set( point_bub_ms( desk, y ), furn_f_counter );
                }
                computer *tmpcomp = m->add_computer( { desk, p2.y() - static_cast<int>( width / 4 ), z},
                                                     _( "Log Console" ), 3 );
                tmpcomp->add_option( _( "View Research Logs" ), COMPACT_RESEARCH, 0 );
                tmpcomp->add_option( _( "Download Map Data" ), COMPACT_MAPS, 0 );
                tmpcomp->add_failure( COMPFAIL_SHUTDOWN );
                tmpcomp->add_failure( COMPFAIL_ALARM );
                tmpcomp->add_failure( COMPFAIL_DAMAGE );
                m->place_spawns( GROUP_LAB_SECURITY, 1,
                                 point_bub_ms( desk, static_cast<int>( ( p1.y() + p2.y() ) / 2 ) ),
                                 point_bub_ms( desk, static_cast<int>( ( p1.y() + p2.y() ) / 2 ) ), z, 1, true );
            }
            break;
        case room_chemistry:
            if( rotate % 2 == 0 ) { // Vertical
                for( int x = p1.x(); x <= p2.x(); x++ ) {
                    if( x % 3 == 0 ) {
                        for( int y = p1.y() + 1; y <= p2.y() - 1; y++ ) {
                            m->furn_set( point_bub_ms( x, y ), furn_f_counter );
                        }
                        if( one_in( 3 ) ) {
                            m->place_items( Item_spawn_data_mut_lab, 35, point_bub_ms( x, p1.y() + 1 ),
                                            point_bub_ms( x, p2.y() - 1 ), z, false,
                                            calendar::start_of_cataclysm );
                        } else {
                            m->place_items( Item_spawn_data_chem_lab, 70, point_bub_ms( x, p1.y() + 1 ),
                                            point_bub_ms( x, p2.y() - 1 ), z, false,
                                            calendar::start_of_cataclysm );
                        }
                    }
                }
            } else {
                for( int y = p1.y(); y <= p2.y(); y++ ) {
                    if( y % 3 == 0 ) {
                        for( int x = p1.x() + 1; x <= p2.x() - 1; x++ ) {
                            m->furn_set( point_bub_ms( x, y ), furn_f_counter );
                        }
                        if( one_in( 3 ) ) {
                            m->place_items( Item_spawn_data_mut_lab, 35, point_bub_ms( p1.x() + 1, y ),
                                            point_bub_ms( p2.x() - 1, y ), z, false,
                                            calendar::start_of_cataclysm );
                        } else {
                            m->place_items( Item_spawn_data_chem_lab, 70, point_bub_ms( p1.x() + 1, y ),
                                            point_bub_ms( p2.x() - 1, y ), z, false,
                                            calendar::start_of_cataclysm );
                        }
                    }
                }
            }
            break;
        case room_teleport:
            m->furn_set( point_bub_ms( ( p1.x() + p2.x() ) / 2, static_cast<int>( ( p1.y() + p2.y() ) / 2 ) ),
                         furn_f_counter );
            m->furn_set( point_bub_ms( static_cast<int>( ( p1.x() + p2.x() ) / 2 ) + 1,
                                       static_cast<int>( ( p1.y() + p2.y() ) / 2 ) ),
                         furn_f_counter );
            m->furn_set( point_bub_ms( ( p1.x() + p2.x() ) / 2,
                                       static_cast<int>( ( p1.y() + p2.y() ) / 2 ) + 1 ),
                         furn_f_counter );
            m->furn_set( point_bub_ms( static_cast<int>( ( p1.x() + p2.x() ) / 2 ) + 1,
                                       static_cast<int>( ( p1.y() + p2.y() ) / 2 ) + 1 ),
                         furn_f_counter );
            mtrap_set( m, trap, tr_telepad );
            m->place_items( Item_spawn_data_teleport, 70, point_bub_ms( ( p1.x() + p2.x() ) / 2,
                            static_cast<int>( ( p1.y() + p2.y() ) / 2 ) ),
                            point_bub_ms( static_cast<int>( ( p1.x() + p2.x() ) / 2 ) + 1,
                                          static_cast<int>( ( p1.y() + p2.y() ) / 2 ) + 1 ),
                            z,
                            false,
                            calendar::start_of_cataclysm );
            break;
        case room_goo:
            do {
                mtrap_set( m, trap, tr_goo );
                trap.x() = rng( p1.x() + 1, p2.x() - 1 );
                trap.y() = rng( p1.y() + 1, p2.y() - 1 );
            } while( !one_in( 5 ) );
            if( rotate == 0 ) {
                mremove_trap( m, tripoint_bub_ms( p1.x(), p2.y(), z ), tr_null );
                m->furn_set( tripoint_bub_ms( p1.x(), p2.y(), z ), furn_f_fridge );
                m->place_items( Item_spawn_data_goo, 60, point_bub_ms( p1.x(), p2.y() ), point_bub_ms( p1.x(),
                                p2.y() ), z,
                                false, calendar::start_of_cataclysm );
            } else if( rotate == 1 ) {
                mremove_trap( m, tripoint_bub_ms( p1, z ), tr_null );
                m->furn_set( tripoint_bub_ms( p1, z ), furn_f_fridge );
                m->place_items( Item_spawn_data_goo, 60, p1, p1, z, false,
                                calendar::start_of_cataclysm );
            } else if( rotate == 2 ) {
                mremove_trap( m, tripoint_bub_ms( p2.x(), p1.y(), z ), tr_null );
                m->furn_set( tripoint_bub_ms( p2.x(), p1.y(), z ), furn_f_fridge );
                m->place_items( Item_spawn_data_goo, 60, point_bub_ms( p2.x(), p1.y() ), point_bub_ms( p2.x(),
                                p1.y() ), z,
                                false, calendar::start_of_cataclysm );
            } else {
                mremove_trap( m, tripoint_bub_ms( p2, z ), tr_null );
                m->furn_set( tripoint_bub_ms( p2, z ), furn_f_fridge );
                m->place_items( Item_spawn_data_goo, 60, p2, p2, z, false,
                                calendar::start_of_cataclysm );
            }
            break;
        case room_cloning:
            for( int x = p1.x() + 1; x <= p2.x() - 1; x++ ) {
                for( int y = p1.y() + 1; y <= p2.y() - 1; y++ ) {
                    if( x % 3 == 0 && y % 3 == 0 ) {
                        m->ter_set( tripoint_bub_ms( x, y, z ), ter_t_vat );
                        m->place_items( Item_spawn_data_cloning_vat, 20, point_bub_ms( x, y ),
                                        point_bub_ms( x, y ), z, false, calendar::start_of_cataclysm );
                    }
                }
            }
            break;
        case room_vivisect:
            if( rotate == 0 ) {
                for( int x = p1.x(); x <= p2.x(); x++ ) {
                    m->furn_set( tripoint_bub_ms( x, p2.y() - 1, z ), furn_f_counter );
                }
                m->place_items( Item_spawn_data_dissection, 80, point_bub_ms( p1.x(), p2.y() - 1 ),
                                p2 + point::north, z, false, calendar::start_of_cataclysm );
            } else if( rotate == 1 ) {
                for( int y = p1.y(); y <= p2.y(); y++ ) {
                    m->furn_set( tripoint_bub_ms( p1.x() + 1, y, z ), furn_f_counter );
                }
                m->place_items( Item_spawn_data_dissection, 80, p1 + point::east,
                                point_bub_ms( p1.x() + 1, p2.y() ), z, false, calendar::start_of_cataclysm );
            } else if( rotate == 2 ) {
                for( int x = p1.x(); x <= p2.x(); x++ ) {
                    m->furn_set( tripoint_bub_ms( x, p1.y() + 1, z ), furn_f_counter );
                }
                m->place_items( Item_spawn_data_dissection, 80, point_bub_ms( p1 + point::south ),
                                point_bub_ms( p2.x(), p1.y() + 1 ), z, false, calendar::start_of_cataclysm );
            } else if( rotate == 3 ) {
                for( int y = p1.y(); y <= p2.y(); y++ ) {
                    m->furn_set( tripoint_bub_ms( p2.x() - 1, y, z ), furn_f_counter );
                }
                m->place_items( Item_spawn_data_dissection, 80, point_bub_ms( p2.x() - 1, p1.y() ),
                                point_bub_ms( p2 + point::west ), z, false, calendar::start_of_cataclysm );
            }
            mtrap_set( m, tripoint_bub_ms( ( p1.x() + p2.x() ) / 2, static_cast<int>( ( p1.y() + p2.y() ) / 2 ),
                                           z ),
                       tr_dissector );
            m->place_spawns( GROUP_LAB_CYBORG, 10,
                             point_bub_ms( static_cast<int>( ( ( p1.x() + p2.x() ) / 2 ) + 1 ),
                                           static_cast<int>( ( ( p1.y() + p2.y() ) / 2 ) + 1 ) ),
                             point_bub_ms( static_cast<int>( ( ( p1.x() + p2.x() ) / 2 ) + 1 ),
                                           static_cast<int>( ( ( p1.y() + p2.y() ) / 2 ) + 1 ) ), z, 1, true );
            break;

        case room_bionics:
            if( rotate % 2 == 0 ) {
                point_bub_ms bio( p1.x() + 2, static_cast<int>( ( p1.y() + p2.y() ) / 2 ) );
                mapf::formatted_set_simple( m, bio + point::north_west,
                                            "---\n"
                                            "|c|\n"
                                            "-=-\n",
                                            mapf::ter_bind( "- | =", ter_t_concrete_wall, ter_t_concrete_wall, ter_t_reinforced_glass_lab ),
                                            mapf::furn_bind( "c", furn_f_counter ) );
                m->place_items( Item_spawn_data_bionics_common, 70, bio,
                                bio, z, false, calendar::start_of_cataclysm );

                m->furn_set( tripoint_bub_ms( bio.x(), bio.y() + 2, z ), furn_f_console );
                computer *tmpcomp = m->add_computer( { bio.x(),  bio.y() + 2, z}, _( "Bionic access" ), 2 );
                tmpcomp->add_option( _( "Manifest" ), COMPACT_LIST_BIONICS, 0 );
                tmpcomp->add_option( _( "Open Chambers" ), COMPACT_RELEASE_BIONICS, 3 );
                tmpcomp->add_failure( COMPFAIL_MANHACKS );
                tmpcomp->add_failure( COMPFAIL_SECUBOTS );
                tmpcomp->set_access_denied_msg(
                    _( "ERROR!  Access denied!  Unauthorized access will be met with lethal force!" ) );

                bio.x() = p2.x() - 2;
                mapf::formatted_set_simple( m, bio + point::north_west,
                                            "-=-\n"
                                            "|c|\n"
                                            "---\n",
                                            mapf::ter_bind( "- | =", ter_t_concrete_wall, ter_t_concrete_wall, ter_t_reinforced_glass_lab ),
                                            mapf::furn_bind( "c", furn_f_counter ) );
                m->place_items( Item_spawn_data_bionics_common, 70, bio,
                                bio, z, false, calendar::start_of_cataclysm );

                m->furn_set( tripoint_bub_ms( bio.x(), bio.y() - 2, z ), furn_f_console );
                computer *tmpcomp2 = m->add_computer( { bio.x(),  bio.y() - 2, z}, _( "Bionic access" ), 2 );
                tmpcomp2->add_option( _( "Manifest" ), COMPACT_LIST_BIONICS, 0 );
                tmpcomp2->add_option( _( "Open Chambers" ), COMPACT_RELEASE_BIONICS, 3 );
                tmpcomp2->add_failure( COMPFAIL_MANHACKS );
                tmpcomp2->add_failure( COMPFAIL_SECUBOTS );
                tmpcomp2->set_access_denied_msg(
                    _( "ERROR!  Access denied!  Unauthorized access will be met with lethal force!" ) );
            } else {
                int bioy = p1.y() + 2;
                int biox = static_cast<int>( ( p1.x() + p2.x() ) / 2 );
                mapf::formatted_set_simple( m, point_bub_ms( biox - 1, bioy - 1 ),
                                            "|-|\n"
                                            "|c=\n"
                                            "|-|\n",
                                            mapf::ter_bind( "- | =", ter_t_concrete_wall, ter_t_concrete_wall, ter_t_reinforced_glass_lab ),
                                            mapf::furn_bind( "c", furn_f_counter ) );
                m->place_items( Item_spawn_data_bionics_common, 70, point_bub_ms( biox, bioy ),
                                point_bub_ms( biox, bioy ), z, false, calendar::start_of_cataclysm );

                m->furn_set( tripoint_bub_ms( biox + 2, bioy, z ), furn_f_console );
                computer *tmpcomp = m->add_computer( { biox + 2,  bioy, z }, _( "Bionic access" ), 2 );
                tmpcomp->add_option( _( "Manifest" ), COMPACT_LIST_BIONICS, 0 );
                tmpcomp->add_option( _( "Open Chambers" ), COMPACT_RELEASE_BIONICS, 3 );
                tmpcomp->add_failure( COMPFAIL_MANHACKS );
                tmpcomp->add_failure( COMPFAIL_SECUBOTS );
                tmpcomp->set_access_denied_msg(
                    _( "ERROR!  Access denied!  Unauthorized access will be met with lethal force!" ) );

                bioy = p2.y() - 2;
                mapf::formatted_set_simple( m, point_bub_ms( biox - 1, bioy - 1 ),
                                            "|-|\n"
                                            "=c|\n"
                                            "|-|\n",
                                            mapf::ter_bind( "- | =", ter_t_concrete_wall, ter_t_concrete_wall, ter_t_reinforced_glass_lab ),
                                            mapf::furn_bind( "c", furn_f_counter ) );
                m->place_items( Item_spawn_data_bionics_common, 70, point_bub_ms( biox, bioy ),
                                point_bub_ms( biox, bioy ), z, false, calendar::turn_zero );

                m->furn_set( tripoint_bub_ms( biox - 2, bioy, z ), furn_f_console );
                computer *tmpcomp2 = m->add_computer( { biox - 2,  bioy, z }, _( "Bionic access" ), 2 );
                tmpcomp2->add_option( _( "Manifest" ), COMPACT_LIST_BIONICS, 0 );
                tmpcomp2->add_option( _( "Open Chambers" ), COMPACT_RELEASE_BIONICS, 3 );
                tmpcomp2->add_failure( COMPFAIL_MANHACKS );
                tmpcomp2->add_failure( COMPFAIL_SECUBOTS );
                tmpcomp2->set_access_denied_msg(
                    _( "ERROR!  Access denied!  Unauthorized access will be met with lethal force!" ) );
            }
            break;
        case room_dorm:
            if( rotate % 2 == 0 ) {
                for( int y = p1.y() + 1; y <= p2.y() - 1; y += 3 ) {
                    m->furn_set( tripoint_bub_ms( p1.x(), y, z ), furn_f_bed );
                    m->place_items( Item_spawn_data_bed, 60, point_bub_ms( p1.x(), y ), point_bub_ms( p1.x(), y ), z,
                                    false, calendar::start_of_cataclysm );
                    m->furn_set( tripoint_bub_ms( p1.x() + 1, y, z ), furn_f_bed );
                    m->place_items( Item_spawn_data_bed, 60, point_bub_ms( p1.x() + 1, y ),
                                    point_bub_ms( p1.x() + 1, y ), z, false, calendar::start_of_cataclysm );
                    m->furn_set( tripoint_bub_ms( p2.x(), y, z ), furn_f_bed );
                    m->place_items( Item_spawn_data_bed, 60, point_bub_ms( p2.x(), y ), point_bub_ms( p2.x(), y ), z,
                                    false, calendar::start_of_cataclysm );
                    m->furn_set( tripoint_bub_ms( p2.x() - 1, y, z ), furn_f_bed );
                    m->place_items( Item_spawn_data_bed, 60, point_bub_ms( p2.x() - 1, y ),
                                    point_bub_ms( p2.x() - 1, y ), z, false, calendar::start_of_cataclysm );
                    m->furn_set( tripoint_bub_ms( p1.x(), y + 1, z ), furn_f_dresser );
                    m->furn_set( tripoint_bub_ms( p2.x(), y + 1, z ), furn_f_dresser );
                    m->place_items( Item_spawn_data_dresser, 70, point_bub_ms( p1.x(), y + 1 ),
                                    point_bub_ms( p1.x(), y + 1 ), z, false, calendar::start_of_cataclysm );
                    m->place_items( Item_spawn_data_dresser, 70, point_bub_ms( p2.x(), y + 1 ),
                                    point_bub_ms( p2.x(), y + 1 ), z, false, calendar::start_of_cataclysm );
                }
            } else if( rotate % 2 == 1 ) {
                for( int x = p1.x() + 1; x <= p2.x() - 1; x += 3 ) {
                    m->furn_set( tripoint_bub_ms( x, p1.y(), z ), furn_f_bed );
                    m->place_items( Item_spawn_data_bed, 60, point_bub_ms( x, p1.y() ), point_bub_ms( x, p1.y() ), z,
                                    false, calendar::start_of_cataclysm );
                    m->furn_set( tripoint_bub_ms( x, p1.y() + 1, z ), furn_f_bed );
                    m->place_items( Item_spawn_data_bed, 60, point_bub_ms( x, p1.y() + 1 ),
                                    point_bub_ms( x, p1.y() + 1 ), z, false, calendar::start_of_cataclysm );
                    m->furn_set( tripoint_bub_ms( x, p2.y(), z ), furn_f_bed );
                    m->place_items( Item_spawn_data_bed, 60, point_bub_ms( x, p2.y() ), point_bub_ms( x, p2.y() ), z,
                                    false, calendar::start_of_cataclysm );
                    m->furn_set( tripoint_bub_ms( x, p2.y() - 1, z ), furn_f_bed );
                    m->place_items( Item_spawn_data_bed, 60, point_bub_ms( x, p2.y() - 1 ),
                                    point_bub_ms( x, p2.y() - 1 ), z, false, calendar::start_of_cataclysm );
                    m->furn_set( tripoint_bub_ms( x + 1, p1.y(), z ), furn_f_dresser );
                    m->furn_set( tripoint_bub_ms( x + 1, p2.y(), z ), furn_f_dresser );
                    m->place_items( Item_spawn_data_dresser, 70, point_bub_ms( x + 1, p1.y() ),
                                    point_bub_ms( x + 1, p1.y() ), z, false, calendar::start_of_cataclysm );
                    m->place_items( Item_spawn_data_dresser, 70, point_bub_ms( x + 1, p2.y() ),
                                    point_bub_ms( x + 1, p2.y() ), z, false, calendar::start_of_cataclysm );
                }
            }
            m->place_items( Item_spawn_data_lab_dorm, 84, p1, p2, z, false,
                            calendar::start_of_cataclysm );
            break;
        case room_split:
            if( rotate % 2 == 0 ) {
                int w1 = static_cast<int>( ( p1.x() + p2.x() ) / 2 ) - 2;
                int w2 = static_cast<int>( ( p1.x() + p2.x() ) / 2 ) + 2;
                for( int y = p1.y(); y <= p2.y(); y++ ) {
                    m->ter_set( tripoint_bub_ms( w1, y, z ), ter_t_concrete_wall );
                    m->ter_set( tripoint_bub_ms( w2, y, z ), ter_t_concrete_wall );
                }
                m->ter_set( tripoint_bub_ms( w1, static_cast<int>( ( p1.y() + p2.y() ) / 2 ), z ),
                            ter_t_door_glass_frosted_c );
                m->ter_set( tripoint_bub_ms( w2, static_cast<int>( ( p1.y() + p2.y() ) / 2 ), z ),
                            ter_t_door_glass_frosted_c );
                science_room( m, p1, point_bub_ms( w1 - 1, p2.y() ), z, 1 );
                science_room( m, point_bub_ms( w2 + 1, p1.y() ), p2, z, 3 );
            } else {
                int w1 = static_cast<int>( ( p1.y() + p2.y() ) / 2 ) - 2;
                int w2 = static_cast<int>( ( p1.y() + p2.y() ) / 2 ) + 2;
                for( int x = p1.x(); x <= p2.x(); x++ ) {
                    m->ter_set( tripoint_bub_ms( x, w1, z ), ter_t_concrete_wall );
                    m->ter_set( tripoint_bub_ms( x, w2, z ), ter_t_concrete_wall );
                }
                m->ter_set( tripoint_bub_ms( ( p1.x() + p2.x() ) / 2, w1, z ), ter_t_door_glass_frosted_c );
                m->ter_set( tripoint_bub_ms( ( p1.x() + p2.x() ) / 2, w2, z ), ter_t_door_glass_frosted_c );
                science_room( m, p1, point_bub_ms( p2.x(), w1 - 1 ), z, 2 );
                science_room( m, point_bub_ms( p1.x(), w2 + 1 ), p2, z, 0 );
            }
            break;
        default:
            break;
    }
}

void map::create_anomaly( const tripoint_bub_ms &cp, artifact_natural_property prop,
                          bool create_rubble )
{
    // TODO: Z
    const int z = cp.z();
    point_bub_ms c( cp.xy() );
    if( create_rubble ) {
        rough_circle( this, ter_t_dirt, c, 11 );
        rough_circle_furn( this, furn_f_rubble, c, 5 );
        furn_set( c, furn_str_id::NULL_ID() );
    }
    switch( prop ) {
        case ARTPROP_WRIGGLING:
        case ARTPROP_MOVING:
            for( int i = c.x() - 5; i <= c.x() + 5; i++ ) {
                for( int j = c.y() - 5; j <= c.y() + 5; j++ ) {
                    if( furn( point_bub_ms( i, j ) ) == furn_f_rubble ) {
                        add_field( tripoint_bub_ms{ i, j, z }, fd_push_items, 1 );
                        if( one_in( 3 ) ) {
                            spawn_item( point_bub_ms( i, j ), itype_rock );
                        }
                    }
                }
            }
            break;

        case ARTPROP_GLOWING:
        case ARTPROP_GLITTERING:
            for( int i = c.x() - 5; i <= c.x() + 5; i++ ) {
                for( int j = c.y() - 5; j <= c.y() + 5; j++ ) {
                    if( furn( point_bub_ms( i, j ) ) == furn_f_rubble && one_in( 2 ) ) {
                        mtrap_set( this, tripoint_bub_ms( i, j, z ), tr_glow );
                    }
                }
            }
            break;

        case ARTPROP_HUMMING:
        case ARTPROP_RATTLING:
            for( int i = c.x() - 5; i <= c.x() + 5; i++ ) {
                for( int j = c.y() - 5; j <= c.y() + 5; j++ ) {
                    if( furn( point_bub_ms( i, j ) ) == furn_f_rubble && one_in( 2 ) ) {
                        mtrap_set( this, tripoint_bub_ms( i, j, z ), tr_hum );
                    }
                }
            }
            break;

        case ARTPROP_WHISPERING:
        case ARTPROP_ENGRAVED:
            for( int i = c.x() - 5; i <= c.x() + 5; i++ ) {
                for( int j = c.y() - 5; j <= c.y() + 5; j++ ) {
                    if( furn( point_bub_ms( i, j ) ) == furn_f_rubble && one_in( 3 ) ) {
                        mtrap_set( this, tripoint_bub_ms( i, j, z ), tr_shadow );
                    }
                }
            }
            break;

        case ARTPROP_BREATHING:
            for( int i = c.x() - 1; i <= c.x() + 1; i++ ) {
                for( int j = c.y() - 1; j <= c.y() + 1; j++ ) {
                    if( i == c.x() && j == c.y() ) {
                        place_spawns( GROUP_BREATHER_HUB, 1, point_bub_ms( i, j ), point_bub_ms( i, j ), z, 1,
                                      true );
                    } else {
                        place_spawns( GROUP_BREATHER, 1, point_bub_ms( i, j ), point_bub_ms( i, j ), z, 1, true );
                    }
                }
            }
            break;

        case ARTPROP_DEAD:
            for( int i = c.x() - 5; i <= c.x() + 5; i++ ) {
                for( int j = c.y() - 5; j <= c.y() + 5; j++ ) {
                    if( furn( point_bub_ms( i, j ) ) == furn_f_rubble ) {
                        mtrap_set( this, tripoint_bub_ms( i, j, z ), tr_drain );
                    }
                }
            }
            break;

        case ARTPROP_ITCHY:
            for( int i = c.x() - 5; i <= c.x() + 5; i++ ) {
                for( int j = c.y() - 5; j <= c.y() + 5; j++ ) {
                    if( furn( point_bub_ms( i, j ) ) == furn_f_rubble ) {
                        set_radiation( point_bub_ms( i, j ), rng( 0, 10 ) );
                    }
                }
            }
            break;

        case ARTPROP_ELECTRIC:
        case ARTPROP_CRACKLING:
            add_field( { c, abs_sub.z() }, fd_shock_vent, 3 );
            break;

        case ARTPROP_SLIMY:
            add_field( { c, abs_sub.z() }, fd_acid_vent, 3 );
            break;

        case ARTPROP_WARM:
            for( int i = c.x() - 5; i <= c.x() + 5; i++ ) {
                for( int j = c.y() - 5; j <= c.y() + 5; j++ ) {
                    if( furn( point_bub_ms( i, j ) ) == furn_f_rubble ) {
                        add_field( tripoint_bub_ms{ i, j, abs_sub.z() }, fd_fire_vent,
                                   1 + ( rl_dist( c, point_bub_ms( i, j ) ) % 3 ) );
                    }
                }
            }
            break;

        case ARTPROP_SCALED:
            for( int i = c.x() - 5; i <= c.x() + 5; i++ ) {
                for( int j = c.y() - 5; j <= c.y() + 5; j++ ) {
                    if( furn( point_bub_ms( i, j ) ) == furn_f_rubble ) {
                        mtrap_set( this, tripoint_bub_ms( i, j, z ), tr_snake );
                    }
                }
            }
            break;

        case ARTPROP_FRACTAL: {
            // Want to choose a random anomaly type which isn't fractal
            // (because nested fractal anomalies lead to out of bounds
            // placement).  To be able to easily choose a random type excluding
            // fractal we want ARTPROP_FRACTAL to be the last type in the enum.
            // Verify that here with a static_assert.
            static_assert( ARTPROP_FRACTAL + 1 == ARTPROP_MAX,
                           "ARTPROP_FRACTAL should be the last type in the list before "
                           "ARTPROP_MAX" );
            auto random_type = []() {
                return static_cast<artifact_natural_property>(
                           rng( ARTPROP_NULL + 1, ARTPROP_FRACTAL - 1 ) );
            };
            create_anomaly( c + point( -4, -4 ), random_type() );
            create_anomaly( c + point( 4, -4 ), random_type() );
            create_anomaly( c + point( -4, 4 ), random_type() );
            create_anomaly( c + point( 4, 4 ), random_type() );
            break;
        }
        default:
            break;
    }
}
///////////////////// part of map

void line( map *m, const ter_id &type, const point_bub_ms &p1, const point_bub_ms &p2, int z )
{
    m->draw_line_ter( type, p1, p2, z );
}
void line( tinymap *m, const ter_id &type, const point_omt_ms &p1, const point_omt_ms &p2 )
{
    m->draw_line_ter( type, p1, p2 );
}
void line_furn( map *m, const furn_id &type, const point_bub_ms &p1, const point_bub_ms &p2, int z )
{
    m->draw_line_furn( type, p1, p2, z );
}
void line_furn( tinymap *m, const furn_id &type, const point_omt_ms &p1, const point_omt_ms &p2 )
{
    m->draw_line_furn( type, p1, p2 );
}
void fill_background( map *m, const ter_id &type )
{
    m->draw_fill_background( type );
}
void fill_background( map *m, ter_id( *f )() )
{
    m->draw_fill_background( f );
}
void square( map *m, const ter_id &type, const point_bub_ms &p1, const point_bub_ms &p2 )
{
    m->draw_square_ter( type, p1, p2, m->get_abs_sub().z() );
}
void square_furn( map *m, const furn_id &type, const point_bub_ms &p1, const point_bub_ms &p2 )
{
    m->draw_square_furn( type, p1, p2, m->get_abs_sub().z() );
}
void square_furn( tinymap *m, const furn_id &type, const point_omt_ms &p1, const point_omt_ms &p2 )
{
    m->draw_square_furn( type, p1, p2 );
}
void square( map *m, ter_id( *f )(), const point_bub_ms &p1, const point_bub_ms &p2 )
{
    m->draw_square_ter( f, p1, p2 );
}
void square( map *m, const weighted_int_list<ter_id> &f, const point_bub_ms &p1,
             const point_bub_ms &p2 )
{
    m->draw_square_ter( f, p1, p2 );
}
void rough_circle( map *m, const ter_id &type, const point_bub_ms &p, int rad )
{
    m->draw_rough_circle_ter( type, p, rad );
}
void rough_circle_furn( map *m, const furn_id &type, const point_bub_ms &p, int rad )
{
    m->draw_rough_circle_furn( type, p, rad );
}
void circle( map *m, const ter_id &type, double x, double y, double rad )
{
    m->draw_circle_ter( type, rl_vec2d( x, y ), rad );
}
void circle( map *m, const ter_id &type, const point_bub_ms &p, int rad )
{
    m->draw_circle_ter( type, p, rad );
}
void circle_furn( map *m, const furn_id &type, const point_bub_ms &p, int rad )
{
    m->draw_circle_furn( type, p, rad );
}
void add_corpse( map *m, const point_bub_ms &p )
{
    m->add_corpse( { p, m->get_abs_sub().z() } );
}

//////////////////// mapgen update
update_mapgen_function_json::update_mapgen_function_json(
    JsonObject &&jsobj, const std::string &context ) :
    mapgen_function_json_base( std::move( jsobj ), context )
{
}

void update_mapgen_function_json::check() const
{
    check_common();
}

void check_mapgen_consistent_with( const std::string &key, const oter_t &ter )
{
    if( const mapgen_basic_container *container = oter_mapgen.find( key ) ) {
        container->check_consistency_with( ter );
    }
}

bool update_mapgen_function_json::setup_update( const JsonObject &jo )
{
    return setup_common( jo );
}

bool update_mapgen_function_json::setup_internal( const JsonObject &/*jo*/ )
{
    /* update_mapgen doesn't care about fill_ter or rows */
    return true;
}

ret_val<void> update_mapgen_function_json::update_map(
    const tripoint_abs_omt &omt_pos, const mapgen_arguments &args, const tripoint_rel_ms &offset,
    mission *miss, bool verify, bool mirror_horizontal, bool mirror_vertical, int rotation ) const
{
    if( omt_pos.is_invalid() ) {
        debugmsg( "Mapgen update function called with invalid tripoint" );
        return ret_val<void>::make_failure( _( "invalid position (not vehicle/appliance)" ) );
    }

    std::unique_ptr<smallmap> p_update_smap = std::make_unique<smallmap>();
    smallmap &update_smap = *p_update_smap;

    update_smap.load( omt_pos, true );
    update_smap.rotate( 4 - rotation );
    update_smap.mirror( mirror_horizontal, mirror_vertical );

    mapgendata md_base( omt_pos, *update_smap.cast_to_map(), 0.0f, calendar::start_of_cataclysm, miss );
    mapgendata md( md_base, args );

    ret_val<void> const u = update_map( md, offset, verify );
    update_smap.mirror( mirror_horizontal, mirror_vertical );
    update_smap.rotate( rotation );

    if( reality_bubble().inbounds( project_to<coords::ms>( omt_pos ) ) ) {
        // trigger main map cleanup
        p_update_smap.reset();
        // trigger new traps, etc
        g->place_player( get_avatar().pos_bub(), true );
    }

    // Trigger magic to add roofs (within load) if needed.
    if( omt_pos.z() < OVERMAP_HEIGHT ) {
        std::unique_ptr<smallmap> p_roof_smap = std::make_unique<smallmap>();
        smallmap &roof_smap = *p_roof_smap;
        // The loading of the Z level above is not necessary, but looks better.
        const tripoint_abs_omt omt_above = { omt_pos.x(), omt_pos.y(), omt_pos.z() + 1 };
        roof_smap.load( omt_above, false );
    }

    return u;
}

class rotation_guard
{
    public:
        explicit rotation_guard( const mapgendata &md )
            : md( md ), rotation( oter_get_rotation( md.terrain_type() ) ) {
            // If the existing map is rotated, we need to rotate it back to the north
            // orientation before applying our updates.
            if( rotation != 0 && !md.has_flag( jmapgen_flags::no_underlying_rotate ) ) {
                md.m.rotate( rotation );
            }
        }

        ~rotation_guard() {
            // If we rotated the map before applying updates, we now need to rotate
            // it back to where we found it.
            if( rotation != 0 && !md.has_flag( jmapgen_flags::no_underlying_rotate ) ) {
                md.m.rotate( 4 - rotation );
            }
        }
    private:
        const mapgendata &md;
        const int rotation;
};

ret_val<void> update_mapgen_function_json::update_map( const mapgendata &md,
        const tripoint_rel_ms &offset,
        const bool verify ) const
{
    mapgendata md_with_params( md, get_args( md, mapgen_parameter_scope::omt ), flags_ );

    rotation_guard rot( md_with_params );

    return apply_mapgen_in_phases( md_with_params, setmap_points, objects, offset, context_,
                                   verify );
}

mapgen_update_func add_mapgen_update_func( const JsonObject &jo, bool &defer )
{
    if( jo.has_string( "mapgen_update_id" ) ) {
        const update_mapgen_id mapgen_update_id{ jo.get_string( "mapgen_update_id" ) };
        const auto update_function = [mapgen_update_id]( const tripoint_abs_omt & omt_pos,
        mission * miss ) {
            run_mapgen_update_func( mapgen_update_id, omt_pos, {}, miss, false );
        };
        return update_function;
    }

    update_mapgen_function_json json_data( {},
                                           "unknown object in add_mapgen_update_func" );
    mapgen_defer::defer = defer;
    if( !json_data.setup_update( jo ) ) {
        const auto null_function = []( const tripoint_abs_omt &, mission * ) {
        };
        return null_function;
    }
    const auto update_function = [json_data]( const tripoint_abs_omt & omt_pos, mission * miss ) {
        json_data.update_map( omt_pos, {}, tripoint_rel_ms::zero, miss );
    };
    defer = mapgen_defer::defer;
    mapgen_defer::jsi = JsonObject();
    return update_function;
}

static const std::string missing_update_operation =
    "missing update operation (not vehicle/appliance)";

ret_val<void> run_mapgen_update_func(
    const update_mapgen_id &update_mapgen_id, const tripoint_abs_omt &omt_pos,
    const mapgen_arguments &args, mission *miss, bool cancel_on_collision, bool mirror_horizontal,
    bool mirror_vertical, int rotation )
{
    const auto update_function = update_mapgens.find( update_mapgen_id );

    if( update_function == update_mapgens.end() || update_function->second.funcs().empty() ) {
        return ret_val<void>::make_failure( _( missing_update_operation ) );
    }
    return update_function->second.funcs()[0]->update_map(
               omt_pos, args, tripoint_rel_ms::zero, miss, cancel_on_collision, mirror_horizontal,
               mirror_vertical, rotation );
}

ret_val<void> run_mapgen_update_func( const update_mapgen_id &update_mapgen_id, mapgendata &dat,
                                      const bool cancel_on_collision )
{
    const auto update_function = update_mapgens.find( update_mapgen_id );
    if( update_function == update_mapgens.end() || update_function->second.funcs().empty() ) {
        return ret_val<void>::make_failure( _( missing_update_operation ) );
    }
    return update_function->second.funcs()[0]->update_map( dat, tripoint_rel_ms::zero,
            cancel_on_collision );
}

void set_queued_points()
{
    global_variables &globvars = get_globals();
    for( std::pair<const std::string, tripoint_abs_ms> &queued_point : queued_points ) {
        globvars.set_global_value( queued_point.first, queued_point.second );
    }
    queued_points.clear();
}

bool apply_construction_marker( const update_mapgen_id &update_mapgen_id,
                                const tripoint_abs_omt &omt_pos,
                                const mapgen_arguments &args, bool mirror_horizontal,
                                bool mirror_vertical, int rotation, bool apply )
{

    const auto update_function = update_mapgens.find( update_mapgen_id );

    if( update_function == update_mapgens.end() || update_function->second.funcs().empty() ) {
        return false;
    }

    small_fake_map tmp_map( ter_t_grass );

    mapgendata base_fake_md( *tmp_map.cast_to_map(), mapgendata::dummy_settings );
    mapgendata fake_md( base_fake_md, args );
    fake_md.skip = { mapgen_phase::zones };

    std::unique_ptr<tinymap> p_update_tmap = std::make_unique<tinymap>();
    tinymap &update_tmap = *p_update_tmap;

    update_tmap.load( omt_pos, true );
    update_tmap.rotate( 4 - rotation );
    update_tmap.mirror( mirror_horizontal, mirror_vertical );

    {
        // Make sure rot goes out of scope and its destructor restores the map before the
        // "outer scope" mirroring/rotation is undone. It's unlikely inherent map rotation will
        // be present at the same time as construction rotation/mirroring is, but better safe than sorry.

        mapgendata md_base( omt_pos, *update_tmap.cast_to_map(), 0.0f, calendar::start_of_cataclysm,
                            nullptr );
        mapgendata md( md_base, args );

        rotation_guard rot( md );

        if( update_function->second.funcs()[0]->update_map( fake_md ).success() ) {
            for( const tripoint_omt_ms &pos : tmp_map.points_on_zlevel( omt_pos.z() ) ) {
                if( tmp_map.ter( pos ) != ter_t_grass || tmp_map.has_furn( pos ) ) {
                    if( apply ) {
                        update_tmap.add_field( pos, fd_construction_site, 1, time_duration::from_turns( 0 ), false );
                    } else {
                        update_tmap.delete_field( pos, fd_construction_site );
                    }
                }
            }
        }
    }

    update_tmap.mirror( mirror_horizontal, mirror_vertical );
    update_tmap.rotate( rotation );

    return true;
}

std::pair<std::map<ter_id, int>, std::map<furn_id, int>> get_changed_ids_from_update(
            const update_mapgen_id &update_mapgen_id,
            const mapgen_arguments &mapgen_args, ter_id const &base_ter )
{
    std::map<ter_id, int> terrains;
    std::map<furn_id, int> furnitures;

    const auto update_function = update_mapgens.find( update_mapgen_id );

    if( update_function == update_mapgens.end() || update_function->second.funcs().empty() ) {
        return std::make_pair( terrains, furnitures );
    }

    small_fake_map tmp_map( base_ter );

    mapgendata base_fake_md( *tmp_map.cast_to_map(), mapgendata::dummy_settings );
    mapgendata fake_md( base_fake_md, mapgen_args );
    fake_md.skip = { mapgen_phase::zones };

    if( update_function->second.funcs()[0]->update_map( fake_md ).success() ) {
        for( int z = -OVERMAP_DEPTH; z <= OVERMAP_DEPTH; z++ ) {
            for( const tripoint_omt_ms &pos : tmp_map.points_on_zlevel( z ) ) {
                const ter_id &ter_at_pos = tmp_map.ter( pos );
                if( ter_at_pos != base_ter ) {
                    terrains[ter_at_pos] += 1;
                }
                if( tmp_map.has_furn( pos ) ) {
                    const furn_id &furn_at_pos = tmp_map.furn( pos );
                    furnitures[furn_at_pos] += 1;
                }
            }
        }
    }
    return std::make_pair( terrains, furnitures );
}

bool run_mapgen_func( const std::string &mapgen_id, mapgendata &dat )
{
    return oter_mapgen.generate( dat, mapgen_id );
}

mapgen_parameters get_map_special_params( const std::string &mapgen_id )
{
    return oter_mapgen.get_map_special_params( mapgen_id );
}

int register_mapgen_function( const std::string &key )
{
    if( const building_gen_pointer ptr = get_mapgen_cfunction( key ) ) {
        return oter_mapgen.add( key, std::make_shared<mapgen_function_builtin>( ptr ) );
    }
    return -1;
}

bool has_mapgen_for( const std::string &key )
{
    return oter_mapgen.has( key );
}

bool has_update_mapgen_for( const update_mapgen_id &key )
{
    return update_mapgens.count( key );
}
