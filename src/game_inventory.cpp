#include "game_inventory.h"

#include <imgui/imgui.h>
#include <algorithm>
#include <bitset>
#include <cmath>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "bionics.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_utility.h"
#include "character.h"
#include "character_id.h"
#include "color.h"
#include "damage.h"
#include "debug.h"
#include "display.h"
#include "enums.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "input.h"
#include "input_context.h"
#include "inventory.h"
#include "inventory_ui.h"
#include "item.h"
#include "item_contents.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "map.h"
#include "mapdata.h"
#include "messages.h"
#include "npc.h"
#include "npctrade.h"
#include "options.h"
#include "output.h"
#include "pimpl.h"
#include "player_activity.h"
#include "pocket_type.h"
#include "point.h"
#include "recipe.h"
#include "recipe_dictionary.h"
#include "requirements.h"
#include "ret_val.h"
#include "skill.h"
#include "stomach.h"
#include "string_formatter.h"
#include "text.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "ui_manager.h"
#include "units.h"
#include "units_utility.h"
#include "value_ptr.h"
#include "vitamin.h"

static const activity_id ACT_CONSUME_DRINK_MENU( "ACT_CONSUME_DRINK_MENU" );
static const activity_id ACT_CONSUME_FOOD_MENU( "ACT_CONSUME_FOOD_MENU" );
static const activity_id ACT_CONSUME_MEDS_MENU( "ACT_CONSUME_MEDS_MENU" );
static const activity_id ACT_EAT_MENU( "ACT_EAT_MENU" );

static const bionic_id bio_painkiller( "bio_painkiller" );

static const flag_id json_flag_CALORIES_INTAKE( "CALORIES_INTAKE" );
static const flag_id json_flag_CALORIE_BURN( "CALORIE_BURN" );

static const itype_id itype_water_clean( "water_clean" );

static const json_character_flag json_flag_MANUAL_CBM_INSTALLATION( "MANUAL_CBM_INSTALLATION" );
static const json_character_flag json_flag_PAIN_IMMUNE( "PAIN_IMMUNE" );

static const quality_id qual_ANESTHESIA( "ANESTHESIA" );

static const requirement_id requirement_data_anesthetic( "anesthetic" );

static const skill_id skill_computer( "computer" );

static const trait_id trait_DEBUG_BIONICS( "DEBUG_BIONICS" );
static const trait_id trait_SAPROPHAGE( "SAPROPHAGE" );
static const trait_id trait_SAPROVORE( "SAPROVORE" );

using item_filter = std::function<bool ( const item & )>;
using item_location_filter = std::function<bool ( const item_location & )>;

namespace
{

std::string good_bad_none( int value )
{
    if( value > 0 ) {
        return string_format( "<good>+%d</good>", value );
    } else if( value < 0 ) {
        return string_format( "<bad>%d</bad>", value );
    }
    return std::string();
}

std::string highlight_good_bad_none( int value )
{
    if( value > 0 ) {
        return string_format( "<color_yellow_green>+%d</color>", value );
    } else if( value < 0 ) {
        return string_format( "<color_yellow_red>%d</color>", value );
    }
    return string_format( "<color_yellow>%d</color>", value );
}

} // namespace

inventory_filter_preset::inventory_filter_preset( const item_location_filter &filter )
    : filter( filter )
{}

bool inventory_filter_preset::is_shown( const item_location &location ) const
{
    return filter( location );
}

static item_location_filter convert_filter( const item_filter &filter )
{
    return [ &filter ]( const item_location & loc ) {
        return filter( *loc );
    };
}

static item_location inv_internal( Character &u, const inventory_selector_preset &preset,
                                   const std::string &title, int radius,
                                   const std::string &none_message,
                                   const std::string &hint = std::string(),
                                   item_location container = item_location(),
                                   bool add_ebooks = false )
{
    inventory_pick_selector inv_s( u, preset );

    inv_s.set_title( title );
    inv_s.set_hint( hint );
    inv_s.set_display_stats( false );

    const std::vector<activity_id> consuming {
        ACT_EAT_MENU,
        ACT_CONSUME_FOOD_MENU,
        ACT_CONSUME_DRINK_MENU,
        ACT_CONSUME_MEDS_MENU };

    u.inv->restack( u );

    inv_s.clear_items();

    if( container ) {
        // Only look inside the container.
        inv_s.add_contained_items( container );
    } else {
        // Default behavior.
        inv_s.add_character_items( u, add_ebooks );
        inv_s.add_nearby_items( radius, add_ebooks );
    }

    if( u.has_activity( consuming ) ) {
        if( !u.activity.str_values.empty() ) {
            inv_s.set_filter( u.activity.str_values[0] );
        }
        // Set position after filter to keep cursor at the right position
        bool position_set = false;
        if( !u.activity.targets.empty() ) {
            bool const hidden = u.activity.values.size() >= 3 && static_cast<bool>( u.activity.values[2] );
            position_set = inv_s.highlight_one_of( u.activity.targets, hidden );
            if( !position_set && hidden ) {
                position_set = inv_s.highlight_one_of( u.activity.targets );
            }
        }
        if( !position_set && u.activity.values.size() >= 2 ) {
            inv_s.highlight_position( std::make_pair( u.activity.values[0], u.activity.values[1] ) );
        }
    }

    if( inv_s.empty() ) {
        const std::string msg = none_message.empty()
                                ? _( "You don't have the necessary item at hand." )
                                : none_message;
        popup( msg, PF_GET_KEY );
        return item_location();
    }

    item_location location = inv_s.execute();

    if( u.has_activity( consuming ) ) {
        inventory_entry const &e = inv_s.get_highlighted();
        bool const collated = e.is_collation_entry();
        u.activity.values.clear();
        const auto init_pair = inv_s.get_highlighted_position();
        u.activity.values.push_back( init_pair.first );
        u.activity.values.push_back( init_pair.second );
        u.activity.values.push_back( collated );
        u.activity.str_values.clear();
        u.activity.str_values.emplace_back( inv_s.get_filter() );
        u.activity.targets = collated ? std::vector<item_location> { location, inv_s.get_collation_next() }
                             :
                             inv_s.get_highlighted().locations;
    }

    return location;
}

static drop_locations inv_internal_multi( Character &u, const inventory_selector_preset &preset,
        const std::string &title, int radius,
        const std::string &none_message,
        const std::string &hint = std::string(),
        item_location container = item_location() )
{
    inventory_multiselector inv_s( u, preset );

    inv_s.set_title( title );
    inv_s.set_hint( hint );
    inv_s.set_display_stats( false );

    u.inv->restack( u );

    inv_s.clear_items();

    if( container ) {
        // Only look inside the container.
        inv_s.add_contained_items( container );
    } else {
        // Default behavior.
        inv_s.add_character_items( u );
        inv_s.add_nearby_items( radius );
    }

    if( inv_s.empty() ) {
        const std::string msg = none_message.empty()
                                ? _( "You don't have the necessary item at hand." )
                                : none_message;
        popup( msg, PF_GET_KEY );
        return drop_locations();
    }

    return inv_s.execute();
}

void game_menus::inv::common()
{
    avatar &you = get_avatar();
    // Return to inventory menu on those inputs
    static const std::set<int> loop_options = { { '\0', '=', 'f', '<', '>'}};

    inventory_selector_preset inv_s_p = default_preset;
    inv_s_p.save_state = &inventory_ui_default_state;
    inventory_pick_selector inv_s( you, inv_s_p );

    inv_s.set_title( _( "Inventory" ) );
    inv_s.set_hint( string_format(
                        _( "Item hotkeys assigned: <color_light_gray>%d</color>/<color_light_gray>%d</color>" ),
                        you.allocated_invlets().count(), inv_chars.size() ) );

    int res = 0;

    item_location location;
    std::string filter;
    do {
        you.inv->restack( you );
        inv_s.drag_enabled = true;
        inv_s.clear_items();
        inv_s.add_character_items( you );
        inv_s.set_filter( filter );
        if( location != item_location::nowhere ) {
            inv_s.highlight_one_of( { location } );
        }

        location = inv_s.execute();
        filter = inv_s.get_filter();

        if( location == item_location::nowhere ) {
            break;
        }

        res = g->inventory_item_menu( location );
    } while( loop_options.count( res ) != 0 );
}

void game_menus::inv::common( item_location &loc )
{
    avatar &you = get_avatar();
    // Return to inventory menu on those inputs
    static const std::set<int> loop_options = { { '\0', '=', 'f' } };

    container_inventory_selector inv_s( you, loc );

    inv_s.set_title( string_format( _( "Inventory of %s" ), loc->tname() ) );
    inv_s.set_hint( string_format(
                        _( "Item hotkeys assigned: <color_light_gray>%d</color>/<color_light_gray>%d</color>" ),
                        you.allocated_invlets().count(), inv_chars.size() ) );

    int res = 0;

    item_location location;
    std::string filter;
    do {
        inv_s.clear_items();
        inv_s.add_contained_items( loc );
        inv_s.set_filter( filter );
        if( location != item_location::nowhere ) {
            inv_s.highlight_one_of( { location } );
        }

        location = inv_s.execute();
        filter = inv_s.get_filter();

        if( location == item_location::nowhere ) {
            break;
        }

        res = g->inventory_item_menu( location );
    } while( loop_options.count( res ) != 0 );
}

item_location game_menus::inv::titled_filter_menu( const item_filter &filter, Character &you,
        const std::string &title, int radius, const std::string &none_message )
{
    return inv_internal( you, inventory_filter_preset( convert_filter( filter ) ),
                         title, radius, none_message );
}

item_location game_menus::inv::titled_filter_menu( const item_location_filter &filter,
        Character &you, const std::string &title, int radius, const std::string &none_message )
{
    return inv_internal( you, inventory_filter_preset( filter ), title, radius, none_message );
}

drop_locations game_menus::inv::titled_multi_filter_menu( const item_location_filter &filter,
        Character &you, const std::string &title, int radius, const std::string &none_message )
{
    return inv_internal_multi( you, inventory_filter_preset( filter ), title, radius, none_message );
}

item_location game_menus::inv::titled_menu( Character &you, const std::string &title,
        const std::string &none_message )
{
    const std::string msg = none_message.empty() ? _( "Your inventory is empty." ) : none_message;
    return inv_internal( you, inventory_selector_preset(), title, -1, msg );
}

class armor_inventory_preset: public inventory_selector_preset
{
    public:
        armor_inventory_preset( Character &pl, const std::string &color_in ) :
            you( pl ), color( color_in ) {
            append_cell( [ this ]( const item_location & loc ) {
                return get_number_string( loc->get_avg_encumber( you ) );
            }, _( "AVG ENCUMBRANCE" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return string_format( "<%s>%d%%</color>", color, loc->get_avg_coverage() );
            }, _( "AVG COVERAGE" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return get_number_string( loc->get_warmth() );
            }, _( "WARMTH" ) );

            for( const damage_type &dt : damage_type::get_all() ) {
                if( dt.no_resist ) {
                    continue;
                }
                const damage_type_id &dtid = dt.id;
                append_cell( [ this, dtid ]( const item_location & loc ) {
                    return get_decimal_string( loc->resist( dtid ) );
                }, to_upper_case( dt.name.translated() ) );
            }

            append_cell( [ this ]( const item_location & loc ) {
                return get_number_string( loc->get_env_resist() );
            }, _( "ENV" ) );

            // Show total storage capacity in user's preferred volume units, to 2 decimal places
            append_cell( [ this ]( const item_location & loc ) {
                const int storage_ml = to_milliliter( loc->get_total_capacity() );
                return get_decimal_string( round_up( convert_volume( storage_ml ), 2 ) );
            }, string_format( _( "STORAGE (%s)" ), volume_units_abbr() ) );
        }

    protected:
        Character &you;
    private:
        std::string get_number_string( int number ) const {
            return number ? string_format( "<%s>%d</color>", color, number ) : std::string();
        }
        std::string get_decimal_string( double number ) const {
            return number ? string_format( "<%s>%.2f</color>", color, number ) : std::string();
        }

        const std::string color;
};

class wear_inventory_preset: public armor_inventory_preset
{
    public:
        wear_inventory_preset( Character &you, const std::string &color, const bodypart_id &bp ) :
            armor_inventory_preset( you, color ), bp( bp )
        {}

        bool is_shown( const item_location &loc ) const override {
            return loc->is_armor() &&
                   ( !loc.has_parent() || !is_worn_ablative( loc.parent_item(), loc ) ) &&
                   !you.is_worn( *loc ) &&
                   ( bp != bodypart_str_id::NULL_ID() ? loc->covers( bp ) : true );
        }

        std::string get_denial( const item_location &loc ) const override {
            const auto ret = you.can_wear( *loc );

            if( !ret.success() ) {
                return trim_trailing_punctuations( ret.str() );
            }

            return std::string();
        }

    private:
        const bodypart_id &bp;

};

item_location game_menus::inv::wear( Character &you, const bodypart_id &bp )
{
    return inv_internal( you, wear_inventory_preset( you, "color_yellow", bp ), _( "Wear item" ), 1,
                         _( "You have nothing to wear." ) );
}

class take_off_inventory_preset: public armor_inventory_preset
{
    public:
        explicit take_off_inventory_preset( const std::string &color ) :
            armor_inventory_preset( get_avatar(), color )
        {}

        bool is_shown( const item_location &loc ) const override {
            return loc->is_armor() && you.is_worn( *loc );
        }

        std::string get_denial( const item_location &loc ) const override {
            const ret_val<void> ret = you.can_takeoff( *loc );

            if( !ret.success() ) {
                return trim_trailing_punctuations( ret.str() );
            }

            return std::string();
        }
};

item_location game_menus::inv::take_off()
{
    return inv_internal( get_avatar(), take_off_inventory_preset( "color_red" ), _( "Take off item" ),
                         1, _( "You're not wearing anything." ) );
}

item_location game::inv_map_splice( const item_filter &filter, const std::string &title, int radius,
                                    const std::string &none_message )
{
    return inv_internal( u, inventory_filter_preset( convert_filter( filter ) ),
                         title, radius, none_message );
}

item_location game::inv_map_splice( const item_location_filter &filter, const std::string &title,
                                    int radius, const std::string &none_message )
{
    return inv_internal( u, inventory_filter_preset( filter ),
                         title, radius, none_message );
}

class liquid_inventory_selector_preset : public inventory_selector_preset
{
    public:
        explicit liquid_inventory_selector_preset( const item &liquid,
                const item *const avoid ) : liquid( liquid ), avoid( avoid ) {

            append_cell( []( const item_location & loc ) {
                if( loc.get_item() ) {
                    return string_format( "%s %s", format_volume( loc.get_item()->max_containable_volume() ),
                                          volume_units_abbr() );
                }
                return std::string();
            }, _( "Storage (L)" ) );
        }

        bool is_shown( const item_location &location ) const override {
            if( location.get_item() == avoid ) {
                return false;
            }
            Character *carrier = location.carrier();
            if( carrier != nullptr ) {
                return location->get_remaining_capacity_for_liquid( liquid, *carrier ) > 0;
            }
            const bool allow_buckets = location.where() == item_location::type::map;
            return location->get_remaining_capacity_for_liquid( liquid, allow_buckets ) > 0;
        }

    private:
        const item &liquid;
        const item *const avoid;
};

item_location game_menus::inv::container_for( Character &you, const item &liquid, int radius,
        const item *const avoid )
{
    return inv_internal( you, liquid_inventory_selector_preset( liquid, avoid ),
                         string_format( _( "Pour %s | %s %s into" ), liquid.display_name( liquid.charges ),
                                        format_volume( liquid.volume() ), volume_units_abbr() ), radius,
                         string_format( _( "You don't have a suitable container to pour %s into." ),
                                        liquid.tname() ) );
}

class pickup_inventory_preset : public inventory_selector_preset
{
    public:
        explicit pickup_inventory_preset( const Character &you,
                                          bool skip_wield_check = false, bool ignore_liquidcont = false ) : you( you ),
            skip_wield_check( skip_wield_check ), ignore_liquidcont( ignore_liquidcont ) {
            save_state = &pickup_sel_default_state;
            _pk_type = pocket_type::LAST;
        }

        std::string get_denial( const item_location &loc ) const override {
            if( !you.has_item( *loc ) ) {
                if( loc->made_of_from_type( phase_id::GAS ) ) {
                    return _( "Can't pick up gasses." );
                }
                if( loc->made_of_from_type( phase_id::LIQUID ) && !loc->is_frozen_liquid() ) {
                    if( loc.has_parent() ) {
                        return _( "Can't pick up liquids." );
                    } else {
                        return _( "Can't pick up spilt liquids." );
                    }
                } else if( loc->is_frozen_liquid() ) {
                    ret_val<crush_tool_type> can_crush = you.can_crush_frozen_liquid( loc );

                    if( you.can_pickVolume_partial( *loc, false, nullptr, false, true ) ) {
                        if( loc->has_flag( flag_SHREDDED ) ) {// NOLINT(bugprone-branch-clone)
                            return std::string();
                        } else if( !can_crush.success() ) {
                            return can_crush.str();
                        }
                    } else {
                        item item_copy( *loc );
                        item_copy.charges = 1;
                        item_copy.set_flag( flag_SHREDDED );
                        std::pair<item_location, item_pocket *> pocke = const_cast<Character &>( you ).best_pocket(
                                    item_copy, nullptr, false );

                        item_pocket *ip = pocke.second;
                        if( ip == nullptr || ( ip &&
                                               ( !ip->can_contain( item_copy ).success() ||
                                                 !ip->front().can_combine( item_copy ) ||
                                                 item_copy.typeId() != ip->front().typeId() ) ) ) {
                            return _( "Does not have any pocket for frozen liquids!" );
                        } else {
                            return std::string();
                        }
                    }
                } else if( !you.can_pickVolume_partial( *loc, false, nullptr, false, true ) &&
                           ( skip_wield_check || you.has_wield_conflicts( *loc ) ) ) {
                    return _( "Does not fit in any pocket!" );
                } else if( !you.can_pickWeight_partial( *loc, !get_option<bool>( "DANGEROUS_PICKUPS" ) ) ) {
                    return _( "Too heavy to pick up!" );
                }
            }

            return std::string();
        }

        bool is_shown( const item_location &loc ) const override {
            if( ignore_liquidcont && loc.where() == item_location::type::map &&
                !loc->made_of( phase_id::SOLID ) ) {
                map &here = get_map();
                if( here.has_flag( ter_furn_flag::TFLAG_LIQUIDCONT, loc.pos_bub( here ) ) ) {
                    return false;
                }
            }
            return !( loc.has_parent() && loc.parent_item()->is_ammo_belt() );
        }

    private:
        const Character &you;
        bool skip_wield_check;
        bool ignore_liquidcont;
};

class disassemble_inventory_preset : public inventory_selector_preset
{
    public:
        disassemble_inventory_preset( const Character &you, const inventory &inv ) : you( you ),
            inv( inv ) {

            check_components = true;

            append_cell( [ this ]( const item_location & loc ) {
                const requirement_data &req = get_recipe( loc ).disassembly_requirements();
                if( req.is_empty() ) {
                    return std::string();
                }
                const item *i = loc.get_item();
                std::vector<item_comp> components = i->get_uncraft_components();
                std::sort( components.begin(), components.end() );
                auto it = components.begin();
                while( ( it = std::adjacent_find( it, components.end() ) ) != components.end() ) {
                    ++( it->count );
                    components.erase( std::next( it ) );
                }
                return enumerate_as_string( components.begin(), components.end(),
                []( const decltype( components )::value_type & comps ) {
                    return comps.to_string();
                } );
            }, _( "YIELD" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return to_string_clipped( get_recipe( loc ).time_to_craft( get_player_character(),
                                          recipe_time_flag::ignore_proficiencies ) );
            }, _( "TIME" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return !get_recipe( loc ).is_null();
        }

        std::string get_denial( const item_location &loc ) const override {
            const auto ret = you.can_disassemble( *loc, inv );
            if( !ret.success() ) {
                return ret.str();
            }
            return std::string();
        }

    protected:
        const recipe &get_recipe( const item_location &loc ) const {
            return recipe_dictionary::get_uncraft( loc->typeId() );
        }

    private:
        const Character &you;
        const inventory &inv;
};

item_location game_menus::inv::disassemble( Character &you )
{
    return inv_internal( you, disassemble_inventory_preset( you, you.crafting_inventory() ),
                         _( "Disassemble item" ), PICKUP_RANGE,
                         _( "You don't have any items you could disassemble." ) );
}

class comestible_inventory_preset : public inventory_selector_preset
{
    public:
        explicit comestible_inventory_preset( const Character &you ) : you( you ) {

            _indent_entries = false;
            _collate_entries = true;

            append_cell( [&you]( const item_location & loc ) {
                const nutrients nutr = you.compute_effective_nutrients( *loc );
                return good_bad_none( nutr.kcal() );
            }, _( "CALORIES" ) );

            append_cell( []( const item_location & loc ) {
                return good_bad_none( loc->is_comestible() ? loc->get_comestible()->quench : 0 );
            }, _( "QUENCH" ) );

            append_cell( [&you]( const item_location & loc ) {
                int joy = you.fun_for( *loc ).first;
                std::string joy_str;
                if( loc->has_flag( flag_MUSHY ) ) {
                    joy_str = highlight_good_bad_none( joy );
                } else {
                    joy_str = good_bad_none( joy );
                }

                int max_joy = you.fun_for( *loc, true ).first;
                // max_joy should be 3 wide for alignment of '/'
                if( max_joy == 0 ) {
                    // this will not be an empty string when food's joy can go below 0 by consumption (very rare)
                    return joy_str;
                } else if( max_joy == joy ) {
                    return joy_str + std::string( joy > 0 ? "<good>/  =</good>" : "<bad>/  =</bad>" );
                } else {
                    return string_format( max_joy > 0 ? "%s/<good>%+3d</good>" : "%s/<bad>%3d</bad>",
                                          joy_str, max_joy );
                }
            }, _( "JOY/MAX" ) );

            append_cell( []( const item_location & loc ) {
                const int healthy = loc->is_comestible() ? loc->get_comestible()->healthy : 0;
                return healthy_bar( healthy );
            }, _( "HEALTH" ) );

            append_cell( []( const item_location & loc ) {
                const time_duration spoils = loc->is_comestible() ? loc->get_comestible()->spoils :
                                             calendar::INDEFINITELY_LONG_DURATION;
                if( spoils > 0_turns ) {
                    return to_string_clipped( spoils );
                }
                //~ Used for permafood shelf life in the Eat menu
                return std::string( _( "indefinite" ) );
            }, _( "SHELF LIFE" ) );

            append_cell( []( const item_location & loc ) {
                const item &it = *loc;

                int converted_volume_scale = 0;
                const int charges = std::max( it.charges, 1 );
                const double converted_volume = round_up( convert_volume( it.volume().value() / charges,
                                                &converted_volume_scale ), 2 );

                //~ Eat menu Volume: <num><unit>
                return string_format( _( "%.2f%s" ), converted_volume, volume_units_abbr() );
            }, _( "VOLUME" ) );

            append_cell( [&you]( const item_location & loc ) {
                const item &it = *loc;
                // Quit prematurely if the item is not food.
                if( !it.type->comestible ) {
                    return std::string();
                }
                const int calories_per_effective_volume = you.compute_calories_per_effective_volume( it );
                // Show empty cell instead of 0.
                if( calories_per_effective_volume == 0 ) {
                    return std::string();
                }
                return satiety_bar( calories_per_effective_volume );
            }, _( "SATIETY" ) );

            Character &player_character = get_player_character();
            append_cell( [&player_character]( const item_location & loc ) {
                time_duration time = player_character.get_consume_time( *loc );
                return string_format( "%s", to_string( time, true ) );
            }, _( "CONSUME TIME" ) );

            append_cell( [this, &player_character]( const item_location & loc ) {
                std::string sealed;
                item_location temp = loc;
                // check if at least one parent container is sealed
                while( temp.has_parent() ) {
                    item_pocket *pocket = temp.parent_pocket();
                    if( pocket->sealed() ) {
                        sealed = _( "sealed" );
                        break;
                    }
                    temp = temp.parent_item();
                }
                if( player_character.can_estimate_rot() ) {
                    if( loc->is_comestible() && loc->get_comestible()->spoils > 0_turns ) {
                        return sealed + ( sealed.empty() ? "" : " " ) + get_freshness( loc );
                    }
                    return std::string( "---" );
                }
                return sealed;
            }, _( "FRESHNESS" ) );

            append_cell( [ this, &player_character ]( const item_location & loc ) {
                if( player_character.can_estimate_rot() ) {
                    if( loc->is_comestible() && loc->get_comestible()->spoils > 0_turns ) {
                        if( !loc->rotten() ) {
                            return get_time_left_rounded( loc );
                        }
                    }
                    return std::string( "---" );
                }
                return std::string();
            }, _( "SPOILS IN" ) );

        }

        bool is_shown( const item_location &loc ) const override {
            return ( loc->is_comestible() && you.can_consume_as_is( *loc ) ) ||
                   loc->is_medical_tool();
        }

        std::string get_denial( const item_location &loc ) const override {
            map &here = get_map();

            const item &med = *loc;

            if(
                ( loc->made_of_from_type( phase_id::LIQUID ) &&
                  loc.where() != item_location::type::container ) &&
                !here.has_flag_furn( ter_furn_flag::TFLAG_LIQUIDCONT, loc.pos_bub( here ) ) ) {
                return _( "Can't drink spilt liquids." );
            }
            if(
                loc->made_of_from_type( phase_id::GAS ) &&
                loc.where() != item_location::type::container ) {
                return _( "Can't consume spilt gasses." );
            }

            if( med.is_medication() && !you.can_use_heal_item( med ) && !med.is_craft() ) {
                return _( "Your biology is not compatible with that item." );
            }

            const item &it = *loc;
            const ret_val<edible_rating> res =
                it.is_medical_tool() ? ret_val<edible_rating>::make_success() : you.can_eat( it );

            if( !res.success() ) {
                return res.str();
            }

            return inventory_selector_preset::get_denial( loc );
        }

        bool sort_compare( const inventory_entry &lhs, const inventory_entry &rhs ) const override {
            time_duration time_a = get_time_left( lhs.any_item() );
            time_duration time_b = get_time_left( rhs.any_item() );
            int order_a = get_order( lhs.any_item(), time_a );
            int order_b = get_order( rhs.any_item(), time_b );

            return order_a < order_b
                   || ( order_a == order_b && time_a < time_b )
                   || ( order_a == order_b && time_a == time_b &&
                        inventory_selector_preset::sort_compare( lhs, rhs ) );
        }

    protected:
        int get_order( const item_location &loc, const time_duration &time ) const {
            if( loc->typeId() == itype_water_clean ) {
                return 0;
            } else if( loc->rotten() ) {
                if( you.has_trait( trait_SAPROPHAGE ) || you.has_trait( trait_SAPROVORE ) ) {
                    return 1;
                } else {
                    return 5;
                }
            } else if( time == 0_turns ) {
                return 4;
            } else if( loc.has_parent() &&
                       loc.parent_pocket()->spoil_multiplier() == 0.0f ) {
                return 3;
            } else {
                return 2;
            }
        }

        const islot_comestible &get_edible_comestible( const item &it ) const {
            if( it.is_comestible() && you.can_eat( it ).success() ) {
                // Ok since can_eat() returns false if is_craft() is true
                return *it.type->comestible;
            }
            static const islot_comestible dummy {};
            return dummy;
        }

        time_duration get_time_left( const item_location &loc ) const {
            time_duration time_left = 0_turns;
            const time_duration shelf_life = loc->is_comestible() ? loc->get_comestible()->spoils :
                                             calendar::INDEFINITELY_LONG_DURATION;
            if( shelf_life > 0_turns ) {
                const item &it = *loc;
                const double relative_rot = it.get_relative_rot();
                time_left = shelf_life - shelf_life * relative_rot;

                // Correct for an estimate that exceeds shelf life -- this happens especially with
                // fresh items.
                if( time_left > shelf_life ) {
                    time_left = shelf_life;
                }
            }

            return time_left;
        }

        std::string get_time_left_rounded( const item_location &loc ) const {
            const item &it = *loc;
            if( it.is_going_bad() ) {
                return _( "soon!" );
            }

            time_duration time_left = get_time_left( loc );
            return to_string_approx( time_left );
        }

        std::string get_freshness( const item_location &loc ) {
            const item &it = *loc;
            const double rot_progress = it.get_relative_rot();
            if( it.is_fresh() ) {
                return _( "fresh" );
            } else if( rot_progress < 0.3 ) {
                return _( "quite fresh" );
            } else if( rot_progress < 0.5 ) {
                return _( "near midlife" );
            } else if( rot_progress < 0.7 ) {
                return _( "past midlife" );
            } else if( rot_progress < 0.9 ) {
                return _( "getting older" );
            } else if( !it.rotten() ) {
                return _( "old" );
            } else {
                return _( "rotten" );
            }
        }

    private:
        const Character &you;
};

static std::string get_consume_needs_hint( Character &you )
{
    std::string hint = std::string();
    auto desc = display::hunger_text_color( you );
    hint.append( string_format( "%s %s", _( "Food:" ), colorize( desc.first, desc.second ) ) );
    hint.append( string_format( " %s ", LINE_XOXO_S ) );
    desc = display::thirst_text_color( you );
    hint.append( string_format( "%s %s", pgettext( "inventory", "Drink:" ),
                                colorize( desc.first, desc.second ) ) );
    hint.append( string_format( " %s ", LINE_XOXO_S ) );
    desc = display::pain_text_color( you );
    hint.append( string_format( "%s %s", _( "Pain:" ), colorize( desc.first, desc.second ) ) );
    hint.append( string_format( " %s ", LINE_XOXO_S ) );
    desc = display::sleepiness_text_color( you );
    hint.append( string_format( "%s %s", _( "Rest:" ), colorize( desc.first, desc.second ) ) );
    hint.append( string_format( " %s ", LINE_XOXO_S ) );
    hint.append( string_format( "%s %s", _( "Weight:" ), display::weight_string( you ) ) );
    hint.append( string_format( " %s ", LINE_XOXO_S ) );

    int kcal_ingested_today = you.as_avatar()->get_daily_ingested_kcal( false );
    int kcal_ingested_yesterday = you.as_avatar()->get_daily_ingested_kcal( true );
    int kcal_spent_today = you.as_avatar()->get_daily_spent_kcal( false );
    int kcal_spent_yesterday = you.as_avatar()->get_daily_spent_kcal( true );
    bool has_fitness_band =  you.cache_has_item_with( json_flag_CALORIE_BURN ) ||
                             you.has_flag( json_flag_CALORIE_BURN );
    bool has_tracker = has_fitness_band || you.cache_has_item_with( json_flag_CALORIES_INTAKE ) ||
                       you.has_flag( json_flag_CALORIES_INTAKE );

    std::string kcal_estimated_intake;
    if( kcal_ingested_today == 0 ) {
        //~ kcal_estimated_intake
        kcal_estimated_intake = _( "none " );
    } else if( kcal_ingested_today < 0.2 * you.base_bmr() ) {
        //~ kcal_estimated_intake
        kcal_estimated_intake = _( "minimal " );
    } else if( kcal_ingested_today < 0.5 * you.base_bmr() ) {
        //~ kcal_estimated_intake
        kcal_estimated_intake = _( "low " );
    } else if( kcal_ingested_today < 1.0 * you.base_bmr() ) {
        //~ kcal_estimated_intake
        kcal_estimated_intake = _( "moderate " );
    } else if( kcal_ingested_today < 1.5 * you.base_bmr() ) {
        //~ kcal_estimated_intake
        kcal_estimated_intake = _( "high " );
    } else if( kcal_ingested_today < 2.0 * you.base_bmr() ) {
        //~ kcal_estimated_intake
        kcal_estimated_intake = _( "very high " );
    } else {
        //~ kcal_estimated_intake
        kcal_estimated_intake = _( "huge " );
    }

    hint.append( "\n" );
    if( has_tracker ) {
        hint.append( _( "Consumed: " ) );
        desc = std::make_pair( string_format( _( "%d kcal " ), kcal_ingested_today ), c_white );
        hint.append( string_format( "%s %s", _( "Today:" ), colorize( desc.first, desc.second ) ) );
        desc = std::make_pair( string_format( _( "%d kcal " ), kcal_ingested_yesterday ), c_white );
        hint.append( string_format( "%s %s", _( "Yesterday:" ), colorize( desc.first, desc.second ) ) );
    } else {
        hint.append( _( "Consumed today (kcal): " ) );
        hint.append( colorize( kcal_estimated_intake, c_white ) );
    }
    if( has_fitness_band ) {
        hint.append( string_format( " %s ", LINE_XOXO_S ) );
        hint.append( _( "Spent: " ) );
        desc = std::make_pair( string_format( _( "%d kcal " ), kcal_spent_today ), c_white );
        hint.append( string_format( "%s %s", _( "Today:" ), colorize( desc.first, desc.second ) ) );
        desc = std::make_pair( string_format( _( "%d kcal " ), kcal_spent_yesterday ), c_white );
        hint.append( string_format( "%s %s", _( "Yesterday:" ), colorize( desc.first, desc.second ) ) );
    }

    // add info about vitamins as well
    hint.append( _( "Vitamin Intake: " ) );

    for( const auto &v : vitamin::all() ) {
        if( v.first->type() != vitamin_type::VITAMIN || v.first->has_flag( "OBSOLETE" ) ) {
            //skip non vitamins
            continue;
        }

        const int &guessed_daily_vit_quantity = you.get_daily_vitamin( v.first, false );
        const int &vit_percent = you.vitamin_RDA( v.first, guessed_daily_vit_quantity );

        if( has_tracker ) {
            desc = std::make_pair( string_format( _( "%s: %d%%.  " ), v.second.name(), vit_percent ), c_white );
        } else {
            if( vit_percent >= 90 ) {
                desc = std::make_pair( string_format( _( "%s: %s.  " ), v.second.name(), _( "plenty" ) ), c_white );
            } else if( vit_percent >= 50 ) {
                desc = std::make_pair( string_format( _( "%s: %s.  " ), v.second.name(), _( "some" ) ), c_white );
            } else if( vit_percent > 0 ) {
                desc = std::make_pair( string_format( _( "%s: %s.  " ), v.second.name(), _( "little" ) ), c_white );
            } else {
                desc = std::make_pair( string_format( _( "%s: %s.  " ), v.second.name(), _( "none" ) ), c_white );
            }
        }
        hint.append( colorize( desc.first, desc.second ) );
    }
    return hint;
}

item_location game_menus::inv::consume( const item_location &loc )
{
    avatar &you = get_avatar();
    static item_location container_location;
    if( !you.has_activity( ACT_EAT_MENU ) ) {
        you.assign_activity( ACT_EAT_MENU );
        container_location = loc;
    }
    std::string none_message = you.activity.str_values.size() == 2 ?
                               _( "You have nothing else to consume." ) : _( "You have nothing to consume." );
    return inv_internal( you, comestible_inventory_preset( you ),
                         _( "Consume item" ), 1,
                         none_message,
                         get_consume_needs_hint( you ),
                         container_location );
}

class comestible_filtered_inventory_preset : public comestible_inventory_preset
{
    public:
        comestible_filtered_inventory_preset( const Character &you, bool( *predicate )( const item &it ) ) :
            comestible_inventory_preset( you ), predicate( predicate ) {}

        bool is_shown( const item_location &loc ) const override {
            return comestible_inventory_preset::is_shown( loc ) &&
                   predicate( *loc );
        }

    private:
        bool( *predicate )( const item &it );
};

item_location game_menus::inv::consume_food()
{
    avatar &you = get_avatar();
    if( !you.has_activity( ACT_CONSUME_FOOD_MENU ) ) {
        you.assign_activity( ACT_CONSUME_FOOD_MENU );
    }
    std::string none_message = you.activity.str_values.size() == 2 ?
                               _( "You have nothing else to eat." ) : _( "You have nothing to eat." );
    return inv_internal( you, comestible_filtered_inventory_preset( you, []( const item & it ) {
        return ( it.is_comestible() && it.get_comestible()->comesttype == "FOOD" ) ||
               it.has_flag( flag_USE_EAT_VERB );
    } ),
    _( "Consume food" ), 1,
    none_message,
    get_consume_needs_hint( you ) );
}

item_location game_menus::inv::consume_drink()
{
    avatar &you = get_avatar();
    if( !you.has_activity( ACT_CONSUME_DRINK_MENU ) ) {
        you.assign_activity( ACT_CONSUME_DRINK_MENU );
    }
    std::string none_message = you.activity.str_values.size() == 2 ?
                               _( "You have nothing else to drink." ) : _( "You have nothing to drink." );
    return inv_internal( you, comestible_filtered_inventory_preset( you, []( const item & it ) {
        return it.is_comestible() && it.get_comestible()->comesttype == "DRINK" &&
               !it.has_flag( flag_USE_EAT_VERB );
    } ),
    _( "Consume drink" ), 1,
    none_message,
    get_consume_needs_hint( you ) );
}

item_location game_menus::inv::consume_meds()
{
    avatar &you = get_avatar();
    if( !you.has_activity( ACT_CONSUME_MEDS_MENU ) ) {
        you.assign_activity( ACT_CONSUME_MEDS_MENU );
    }
    std::string none_message = you.activity.str_values.size() == 2 ?
                               _( "You have no more medication to consume." ) : _( "You have no medication to consume." );
    return inv_internal( you, comestible_filtered_inventory_preset( you, []( const item & it ) {
        return it.is_medication() || it.is_medical_tool();
    } ),
    _( "Consume medication" ), 1,
    none_message,
    get_consume_needs_hint( you ) );
}

class activatable_inventory_preset : public pickup_inventory_preset
{
    public:
        explicit activatable_inventory_preset() : pickup_inventory_preset( get_avatar() ),
            you( get_avatar() ) {
            _collate_entries = true;
            if( get_option<bool>( "INV_USE_ACTION_NAMES" ) ) {
                append_cell( [ this ]( const item_location & loc ) {
                    return string_format( "<color_light_green>%s</color>", get_action_name( *loc ) );
                }, _( "ACTION" ) );
            }
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->type->has_use() || loc->has_relic_activation();
        }

        std::string get_denial( const item_location &loc ) const override {
            const item &it = *loc;
            const auto &uses = it.type->use_methods;

            const auto &comest = it.get_comestible();
            if( comest && !comest->tool.is_null() ) {
                const bool has = item::count_by_charges( comest->tool )
                                 ? you.has_charges( comest->tool, 1 )
                                 : you.has_amount( comest->tool, 1 );
                if( !has ) {
                    return string_format( _( "You need a %s to consume that!" ), item::nname( comest->tool ) );
                }
            }
            const use_function *consume_drug = it.type->get_use( "consume_drug" );
            if( consume_drug != nullptr ) { //its a drug)
                const consume_drug_iuse *consume_drug_use = dynamic_cast<const consume_drug_iuse *>
                        ( consume_drug->get_actor_ptr() );
                for( const auto &tool : consume_drug_use->tools_needed ) {
                    const bool has = item::count_by_charges( tool.first )
                                     ? you.has_charges( tool.first, ( tool.second == -1 ) ? 1 : tool.second )
                                     : you.has_amount( tool.first, 1 );
                    if( !has ) {
                        return string_format( _( "You need a %s to consume that!" ), item::nname( tool.first ) );
                    }
                }
            }

            const use_function *smoking = it.type->get_use( "SMOKING" );
            if( smoking != nullptr ) {
                std::optional<std::string> litcig = iuse::can_smoke( you );
                if( litcig.has_value() ) {
                    return _( litcig.value_or( "" ) );
                }
            }

            if( uses.size() == 1 ) {
                const auto ret = uses.begin()->second.can_call( you, it, you.pos_bub() );
                if( !ret.success() ) {
                    return trim_trailing_punctuations( ret.str() );
                }
            }

            if( it.is_medication() && !you.can_use_heal_item( it ) && !it.is_craft() ) {
                return _( "Your biology is not compatible with that item." );
            }

            if( it.is_broken() ) {
                return string_format( _( "Your %s was broken and won't turn on." ), it.tname() );
            }

            if( uses.size() == 1 &&
                !it.ammo_sufficient( &you, uses.begin()->first ) ) {
                return string_format(
                           n_gettext( "Needs at least %d charge",
                                      "Needs at least %d charges", loc->ammo_required() ),
                           loc->ammo_required() );
            }

            if( it.is_frozen_liquid() && it.is_comestible() ) {
                return _( "You can't consume frozen liquids!" );
            }

            if( !it.has_flag( flag_ALLOWS_REMOTE_USE ) ) {
                return pickup_inventory_preset::get_denial( loc );
            }

            return std::string();
        }

    protected:
        std::string get_action_name( const item &it ) const {
            const auto &uses = it.type->use_methods;

            if( uses.size() == 1 ) {
                return uses.begin()->second.get_name();
            } else if( uses.size() == 2 ) {
                return string_format( _( "%1$s</color> <color_dark_gray>or</color> <color_light_green>%2$s" ),
                                      uses.begin()->second.get_name(), std::next( uses.begin() )->second.get_name() );
            } else if( uses.size() > 2 ) {
                return _( "…" );
            }

            return std::string();
        }

    private:
        const Character &you;
};

item_location game_menus::inv::use()
{
    return inv_internal( get_avatar(), activatable_inventory_preset(),
                         _( "Use item" ), 1,
                         _( "You don't have any items you can use." ) );
}

class gunmod_inventory_preset : public inventory_selector_preset
{
    public:
        gunmod_inventory_preset( const Character &you, const item &gunmod ) : you( you ), gunmod( gunmod ) {
            append_cell( [ this ]( const item_location & loc ) {
                const auto odds = get_odds( loc );

                if( odds.first >= 100 ) {
                    return string_format( "<color_light_green>%s</color>", _( "always" ) );
                }

                return string_format( "<color_light_green>%d%%</color>", odds.first );
            }, _( "SUCCESS CHANCE" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return good_bad_none( get_odds( loc ).second );
            }, _( "DAMAGE RISK" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->is_gun() && !loc->is_gunmod();
        }

        std::string get_denial( const item_location &loc ) const override {
            const auto ret = loc->is_gunmod_compatible( gunmod );

            if( !ret.success() ) {
                return ret.str();
            }

            if( !you.meets_requirements( gunmod, *loc ) ) {
                return string_format( _( "requires at least %s" ),
                                      you.enumerate_unmet_requirements( gunmod, *loc ) );
            }

            if( get_odds( loc ).first <= 0 ) {
                return _( "is too difficult for you to modify" );
            }

            return std::string();
        }

        bool sort_compare( const inventory_entry &lhs, const inventory_entry &rhs ) const override {
            const auto a = get_odds( lhs.any_item() );
            const auto b = get_odds( rhs.any_item() );

            if( a.first > b.first || ( a.first == b.first && a.second < b.second ) ) {
                return true;
            }

            return inventory_selector_preset::sort_compare( lhs, rhs );
        }

    protected:
        /** @return Odds for successful installation (pair.first) and gunmod damage (pair.second) */
        std::pair<int, int> get_odds( const item_location &gun ) const {
            return you.gunmod_installation_odds( gun, gunmod );
        }

    private:
        const Character &you;
        const item &gunmod;
};

item_location game_menus::inv::gun_to_modify( Character &you, const item &gunmod )
{
    return inv_internal( you, gunmod_inventory_preset( you, gunmod ),
                         _( "Select gun to modify" ), -1,
                         _( "You don't have any guns to modify." ) );
}

class gunmod_remove_inventory_preset : public inventory_selector_preset
{
    public:
        gunmod_remove_inventory_preset( const Character &you, const item &gun ) : you( you ), gun( gun ) {
            append_cell( [this]( const item_location & loc ) {
                if( !this->you.meets_requirements( *loc.get_item(), this->gun ) ) {
                    return string_format( "<color_yellow>%s</color>", _( "you lack the skills to reinstall" ) );
                }

                return string_format( "<color_light_green>%s</color>", _( "always" ) );
            }, _( "SUCCESS CHANCE" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->is_gunmod() && !loc->is_irremovable();
        }

        // TODO: Need to represent gun mods with a tree structure
        // Right now, for example, if there is one sight mod location on a gun, and there
        // are two mods attached to the gun, both of which use a sight mod location, and add
        // a sight mod location, both are removable. Ideally one should not be removable, to
        // represent the mod that has the other mod attached to its added sight mod location.
        std::string get_denial( const item_location &loc ) const override {
            item mod = *loc.get_item();
            if( ( mod.type->gunmod->location.name() == "magazine" ||
                  mod.type->gunmod->location.name() == "mechanism" ||
                  mod.type->gunmod->location.name() == "loading port" ||
                  mod.type->gunmod->location.name() == "bore" ) &&
                ( gun.ammo_remaining( ) > 0 || gun.magazine_current() ) ) {
                return _( "must be unloaded before removing this mod" );
            }

            if( !loc->type->gunmod->add_mod.empty() ) {
                std::map<gunmod_location, int> mod_locations_added = loc->type->gunmod->add_mod;

                for( const std::pair<const gunmod_location, int> &slot : mod_locations_added ) {
                    if( slot.second > 0 && this->gun.get_free_mod_locations( slot.first ) <= 0 ) {
                        return _( "has mods attached" );
                    }
                }
            }

            return std::string();
        }

    private:
        const Character &you;
        const item &gun;
};

item_location game_menus::inv::gunmod_to_remove( Character &you, item &gun )
{
    const std::string none_message = _( "You don't have any mods to remove." );

    const gunmod_remove_inventory_preset preset( you, gun );
    inventory_pick_selector inv_s( you, preset );

    inv_s.set_title( _( "Remove which modification?" ) );
    inv_s.set_display_stats( false );

    inv_s.clear_items();
    inv_s.add_contained_gunmods( you, gun );

    if( inv_s.empty() ) {
        popup( none_message, PF_GET_KEY );
        return item_location();
    }

    item_location location = inv_s.execute();

    return location;
}

class ereader_inventory_preset : public pickup_inventory_preset
{
    public:
        explicit ereader_inventory_preset( const Character &you ) : pickup_inventory_preset( you ),
            you( you ) {
            _collate_entries = true;
            if( get_option<bool>( "INV_USE_ACTION_NAMES" ) ) {
                append_cell( [ this ]( const item_location & loc ) {
                    return string_format( "<color_light_green>%s</color>", get_action_name( *loc ) );
                }, _( "ACTION" ) );
            }
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->is_estorage();
        }

        std::string get_denial( const item_location &loc ) const override {
            const item &it = *loc;

            if( it.is_broken() ) {
                return _( "E-reader is broken and won't turn on." );
            }

            if( !it.ammo_sufficient( &you, "EBOOKREAD" ) ) {
                return string_format(
                           n_gettext( "Needs at least %d charge.",
                                      "Needs at least %d charges.", loc->ammo_required() ),
                           loc->ammo_required() );
            }

            if( !it.has_flag( flag_ALLOWS_REMOTE_USE ) ) {
                return pickup_inventory_preset::get_denial( loc );
            }

            return std::string();
        }

    protected:
        std::string get_action_name( const item &it ) const {
            return string_format( "Read on %s.", it.tname() );
        }
    private:
        const Character &you;
};

item_location game_menus::inv::ereader_to_use( Character &you )
{
    const std::string msg = _( "You don't have any e-readers you can use." );
    return inv_internal( you, ereader_inventory_preset( you ), _( "Select e-reader." ), 1, msg );
}

class read_inventory_preset: public pickup_inventory_preset
{
    public:
        explicit read_inventory_preset( const Character &you ) : pickup_inventory_preset( you ),
            you( you ) {
            std::string unknown = _( "<color_dark_gray>?</color>" );

            append_cell( [ this, &you, unknown ]( const item_location & loc ) -> std::string {
                if( loc->type->can_use( "MA_MANUAL" ) ) {
                    return _( "martial arts" );
                }
                if( !is_known( loc ) ) {
                    return unknown;
                }
                const islot_book &book = get_book( loc );
                if( book.skill ) {
                    const SkillLevel &skill = you.get_skill_level_object( book.skill );
                    if( skill.can_train() ) {
                        //~ %1$s: book skill name, %2$d: book skill level, %3$d: player skill level
                        return string_format( pgettext( "skill", "%1$s to %2$d (%3$d)" ), book.skill->name(), book.level,
                                              skill.knowledgeLevel() );
                    }
                }
                return std::string();
            }, _( "TRAINS (CURRENT)" ), unknown );

            append_cell( [ this, unknown ]( const item_location & loc ) -> std::string {
                if( !is_known( loc ) ) {
                    return unknown;
                }
                const islot_book &book = get_book( loc );
                const int unlearned = book.recipes.size() - get_known_recipes( book );

                return unlearned > 0 ? std::to_string( unlearned ) : std::string();
            }, _( "RECIPES" ), unknown );
            append_cell( [ this, &you, unknown ]( const item_location & loc ) -> std::string {
                if( !is_known( loc ) ) {
                    return unknown;
                }
                return good_bad_none( you.book_fun_for( *loc, you ) );
            }, _( "FUN" ), unknown );

            append_cell( [ this, &you, unknown ]( const item_location & loc ) -> std::string {
                if( !is_known( loc ) ) {
                    return unknown;
                }
                std::vector<std::string> dummy;

                const Character *reader = you.get_book_reader( *loc, dummy );
                if( reader == nullptr ) {
                    return std::string();  // Just to make sure
                }
                // Actual reading time (in turns). Can be penalized.
                const int actual_turns = to_turns<int>( you.time_to_read( *loc, *reader ) );
                // Theoretical reading time (in turns) based on the reader speed. Free of penalties.
                const int normal_turns = to_turns<int>( get_book( loc ).time ) * reader->read_speed() / 100 ;
                std::string duration = to_string_approx( time_duration::from_turns( actual_turns ), false );

                if( actual_turns > normal_turns ) { // Longer - complicated stuff.
                    return string_format( "<color_light_red>%s</color>", duration );
                }

                return duration; // Normal speed.
            }, _( "CHAPTER IN" ), unknown );
        }

        bool is_shown( const item_location &loc ) const override {
            const item_location p_loc = loc.parent_item();
            return ( loc->is_book() || loc->type->can_use( "learn_spell" ) ) &&
                   ( !p_loc || ( !p_loc->is_estorage() || p_loc->is_estorage_usable( you ) ) );
        }

        std::string get_denial( const item_location &loc ) const override {
            std::vector<std::string> denials;
            if( ( you.get_book_reader( *loc, denials ) == nullptr &&
                  !denials.empty() &&
                  !loc->type->can_use( "learn_spell" ) ) ) {
                return denials.front();
            }
            return pickup_inventory_preset::get_denial( loc.is_efile() ? loc.parent_item() : loc );
        }

        std::function<bool( const inventory_entry & )> get_filter( const std::string &filter ) const
        override {
            auto base_filter = pickup_inventory_preset::get_filter( filter );

            return [this, base_filter, filter]( const inventory_entry & e ) {
                if( base_filter( e ) ) {
                    return true;
                }

                if( !is_known( e.any_item() ) ) {
                    return false;
                }

                const islot_book &book = get_book( e.any_item() );
                if( book.skill && you.get_skill_level_object( book.skill ).can_train() ) {
                    return lcmatch( book.skill->name(), filter );
                }

                return false;
            };
        }

        bool sort_compare( const inventory_entry &lhs, const inventory_entry &rhs ) const override {
            const bool base_sort = inventory_selector_preset::sort_compare( lhs, rhs );

            const bool known_a = is_known( lhs.any_item() );
            const bool known_b = is_known( rhs.any_item() );

            if( !known_a || !known_b ) {
                return ( !known_a && !known_b ) ? base_sort : !known_a;
            }

            const islot_book &book_a = get_book( lhs.any_item() );
            const islot_book &book_b = get_book( rhs.any_item() );

            if( !book_a.skill && !book_b.skill ) {
                return ( book_a.fun == book_b.fun ) ? base_sort : book_a.fun > book_b.fun;
            } else if( !book_a.skill || !book_b.skill ) {
                return static_cast<bool>( book_a.skill );
            }

            const bool train_a = you.get_knowledge_level( book_a.skill ) < book_a.level;
            const bool train_b = you.get_knowledge_level( book_b.skill ) < book_b.level;

            if( !train_a || !train_b ) {
                return ( !train_a && !train_b ) ? base_sort : train_a;
            }

            return base_sort;
        }

        nc_color get_color( const inventory_entry &entry ) const override {
            return entry.is_item() ? entry.any_item()->color_in_inventory( &you ) : c_magenta;
        }

    private:
        const islot_book &get_book( const item_location &loc ) const {
            return *loc->type->book;
        }

        bool is_known( const item_location &loc ) const {
            return get_avatar().has_identified( loc->typeId() );
        }

        int get_known_recipes( const islot_book &book ) const {
            int res = 0;
            for( const islot_book::recipe_with_description_t &elem : book.recipes ) {
                if( you.knows_recipe( elem.recipe ) ) {
                    ++res; // If the player knows it, they recognize it even if it's not clearly stated.
                }
            }
            return res;
        }

        const Character &you;
};

class ebookread_inventory_preset : public read_inventory_preset
{
    private:
        const Character &you;
    public:
        explicit ebookread_inventory_preset( const Character &you ) : read_inventory_preset( you ),
            you( you ) {
        }
        std::string get_denial( const item_location &loc ) const override {
            std::vector<std::string> denials;
            if( you.get_book_reader( *loc, denials ) == nullptr && !denials.empty() &&
                !loc->type->can_use( "learn_spell" ) ) {
                return denials.front();
            }
            return std::string();
        }
};

item_location game_menus::inv::read( Character &you )
{
    const std::string msg = you.is_avatar() ? _( "You have nothing to read." ) :
                            string_format( _( "%s has nothing to read." ), you.disp_name() );
    return inv_internal( you, read_inventory_preset( you ), _( "Read" ), 1, msg, "", item_location(),
                         true );
}

item_location game_menus::inv::ebookread( Character &you, item_location &ereader )
{
    const std::string none_message =
        you.is_avatar() ?
        string_format( _( "%1$s have nothing to read." ), you.disp_name( false, true ) ) :
        string_format( _( "%1$s has nothing to read." ), you.disp_name( false, true ) );

    const ebookread_inventory_preset preset( you );
    inventory_pick_selector inv_s( you, preset );

    inv_s.set_title( _( "Read" ) );
    inv_s.set_display_stats( false );

    inv_s.clear_items();
    inv_s.add_contained_ebooks( ereader );

    if( inv_s.empty() ) {
        popup( none_message, PF_GET_KEY );
        return item_location();
    }

    item_location location = inv_s.execute();

    return location;
}

drop_locations game_menus::inv::ebooksave( Character &who, item_location &ereader )
{
    std::set<itype_id> already_saved;
    for( const item *efile : ereader->efiles() ) {
        if(
            efile->is_estorable() &&
            efile->is_ecopiable()
            //already_saved.find(ebook->typeId()) == already_saved.end()
        ) {
            already_saved.insert( efile->typeId() );
        }
    }

    const inventory_filter_preset preset( [&who, &already_saved]( const item_location & loc ) {

        return ( loc->is_owned_by( who, true ) &&
                 loc->is_estorable() &&
                 loc->is_ecopiable() &&
                 !already_saved.count( loc->typeId() ) );
    } );

    const int available_charges = ereader->ammo_remaining( );
    auto make_raw_stats = [&available_charges, &ereader](
                              const std::vector<std::pair<item_location, int>> &locs
    ) {
        std::vector<item_location> books;
        books.reserve( locs.size() );
        for( const auto &pair : locs ) {
            books.push_back( pair.first );
        }
        const int required_charges = ebooksave_activity_actor::required_charges( books, ereader );
        const time_duration required_time = ebooksave_activity_actor::required_time( books );
        auto int_to_str = []( int val ) -> std::string {
            return string_format( "%d", val );
        };
        const std::string charges = string_join( display_stat( "", required_charges,
                                    available_charges,
                                    int_to_str ), "" );
        const std::string time = colorize( to_string( required_time, true ),
                                           c_light_gray );
        using stats = inventory_selector::stats;
        return stats{{
                {{ _( "Charges" ), charges }},
                {{ _( "Estimated time" ), time }}
            }};
    };

    inventory_multiselector inv_s( who, preset, _( "SELECT BOOKS TO SCAN" ),
                                   make_raw_stats, /*allow_select_contained=*/true );
    inv_s.add_character_items( who );
    inv_s.add_nearby_items( PICKUP_RANGE );
    inv_s.remove_duplicate_itypes( true );
    inv_s.set_title( _( "Scan which books?" ) );
    if( inv_s.empty() ) {
        popup( std::string( _( "You have no books to scan." ) ), PF_GET_KEY );
        return drop_locations();
    }
    return inv_s.execute();
}

drop_locations game_menus::inv::edevice_select( Character &who, item_location &used_edevice,
        bool browse_equals, bool auto_include_used_edevice, bool unusable_only, efile_action action )
{
    const map &here = get_map();

    const inventory_filter_preset preset( [&]( const item_location & loc ) {
        //make sure this is an edevice before we make edevice calls
        if( loc->is_estorage() && loc->is_owned_by( who, true ) && who.sees( here, loc.pos_bub( here ) ) ) {
            efile_activity_actor::edevice_compatible compat =
                efile_activity_actor::edevices_compatible( used_edevice, loc );
            bool is_tool_has_charge = !loc->is_tool() || loc->ammo_sufficient( &who );
            bool used_edevice_check = auto_include_used_edevice || loc != used_edevice;
            bool has_use_check = !unusable_only || !efile_activity_actor::edevice_has_use( loc.get_item() );
            bool browsed_equal_check = browse_equals == loc->is_browsed();
            bool fast_transfer = compat == efile_activity_actor::edevice_compatible::ECOMPAT_FAST;
            bool no_files_check = !( action == EF_MOVE_ONTO_THIS || action == EF_COPY_ONTO_THIS )
                                  || ( loc->is_browsed() && !loc->efiles().empty() );
            bool compatible_check = ( action == EF_BROWSE && browse_equals ) ||
                                    //if browsing, no compatibility check
                                    ( action == EF_READ && fast_transfer ) || //if reading, only fast-compatible edevices
                                    compat != efile_activity_actor::edevice_compatible::ECOMPAT_NONE; //otherwise, any compatible edevice
            bool is_not_forbidden = true;
            for( const pocket_data &pocket : loc->type->pockets ) {
                if( pocket.type == pocket_type::E_FILE_STORAGE && pocket.forbidden ) {
                    is_not_forbidden = false;
                }
            }

            bool preset_bool = is_tool_has_charge &&
                               used_edevice_check &&
                               has_use_check &&
                               browsed_equal_check &&
                               compatible_check &&
                               no_files_check &&
                               is_not_forbidden;
            if( preset_bool ) {
                add_msg_debug( debugmode::DF_ACT_EBOOK, string_format( "found edevice %s", loc->display_name() ) );
            }
            return preset_bool;
        }
        return false;
    } );

    std::string action_name = efile_activity_actor::efile_action_name( action, false, true );
    std::string inv_title = _( action_name );
    std::string used_device_name;
    if( efile_activity_actor::efile_action_exclude_used( action ) ) {
        used_device_name = string_format( _( " (using %s)?" ), used_edevice->display_name() );
    } else {
        used_device_name = "?";
    }

    if( action == EF_READ || efile_activity_actor::efile_action_is_from( action ) ) {
        inventory_pick_selector select_one_edevice( who, preset );
        select_one_edevice.add_character_items( who );
        select_one_edevice.add_nearby_items( PICKUP_RANGE );
        inv_title += _( " which device" ) + used_device_name;
        select_one_edevice.set_title( inv_title );
        if( select_one_edevice.empty() ) {
            popup( std::string( _( "You have no eligible devices to " + action_name + "." ) ), PF_GET_KEY );
            return drop_locations();
        }
        drop_locations returned_device;
        item_location selected_device = select_one_edevice.execute();
        if( selected_device ) {
            returned_device.emplace_back( selected_device, 1 );
        }
        return returned_device;
    } else {
        inventory_multiselector inv_s( who, preset, _( "Select devices to " + action_name ) );
        inv_s.add_character_items( who );
        inv_s.add_nearby_items( PICKUP_RANGE );
        inv_title += _( " which devices" ) + used_device_name;
        inv_s.set_title( inv_title );
        if( inv_s.empty() ) {
            popup( std::string( _( "You have no eligible devices to " + action_name + "." ) ), PF_GET_KEY );
            return drop_locations();
        }
        return inv_s.execute();
    }
}

drop_locations game_menus::inv::efile_select( Character &who, item_location &used_edevice,
        const std::vector<item_location> &target_edevices, efile_action action, bool from_used_edevice )
{
    item_location to_edevice = used_edevice;
    std::vector<item_location> from_edevices = target_edevices;

    if( from_used_edevice ) {
        //swap targets if this is a "from" action
        to_edevice = target_edevices.front();
        from_edevices = { used_edevice };
    }

    item_location external_transfer_edevice;
    bool copying = action == EF_COPY_FROM_THIS || action == EF_COPY_ONTO_THIS;
    bool wiping = action == EF_WIPE;

    const inventory_filter_preset preset( [&copying]( const item_location & loc ) {
        return loc.is_efile() && ( !copying || loc->is_ecopiable() );
    } );

    const int available_charges = to_edevice->ammo_remaining( );
    auto make_raw_stats = [&]( const std::vector<std::pair<item_location, int>> &locs ) {
        std::vector<item_location> efiles;
        efiles.reserve( locs.size() );
        for( const auto &drop : locs ) {
            for( int i = 0; i < drop.second; i++ ) {
                efiles.emplace_back( drop.first );
            }
        }
        time_duration required_time = efile_activity_actor::total_processing_time( used_edevice,
                                      target_edevices, efiles,
                                      action, who, who.get_skill_level( skill_computer ) < 1 );
        units::ememory total_memory;
        bool edevice_slow_transfer = false;
        for( const item_location &efile : efiles ) {
            item_location efile_parent_device = efile.parent_item();
            if( !wiping ) {
                if( efile_activity_actor::edevices_compatible( to_edevice, efile_parent_device ) !=
                    efile_activity_actor::edevice_compatible::ECOMPAT_FAST ) {
                    if( !efile_activity_actor::find_external_transfer_estorage( who, efile,
                            to_edevice, efile_parent_device ) ) {
                        edevice_slow_transfer = true;
                    }
                }
            }
            efile_transfer transfer( to_edevice, efile_parent_device );
            required_time += efile_activity_actor::efile_processing_time( efile, transfer, action,
                             who, who.get_skill_level( skill_computer ) < 1 );
            total_memory += efile->ememory_size();
        }
        const int required_charges = required_time / efile_activity_actor::charge_per_transfer_time;

        auto int_to_str = []( int val ) -> std::string {
            return string_format( "%d", val );
        };
        const units::ememory memory_max = wiping ? to_edevice->total_ememory() :
                                          to_edevice->remaining_ememory();
        const bool over_memory_limit = total_memory > memory_max;
        std::string total_memory_str = units::display( total_memory );
        const std::string ememory = string_format( "%s / %s", over_memory_limit ?
                                    colorize( total_memory_str, c_red ) : total_memory_str, units::display( memory_max ) );
        const std::string charges = string_join( display_stat( "", required_charges,
                                    available_charges,
                                    int_to_str ), "" );
        const std::string time_label = !edevice_slow_transfer ?
                                       _( "Estimated time:" ) :
                                       colorize( _( "Estimated time (slow!):" ), c_yellow );
        const std::string time = colorize( to_string( required_time, true ),
                                           c_light_gray );
        return inventory_selector::stats{ {
                {{ _( "Processed / Available Memory: " ), ememory }},
                {{ _( "Charges" ), charges }},
                {{ time_label, time}}
            } };
    };

    std::string action_name = efile_activity_actor::efile_action_name( action );

    if( action == EF_READ ) {
        inventory_pick_selector select_one_file( who, preset );
        select_one_file.add_contained_efiles( used_edevice );
        select_one_file.set_title( action_name + _( " which files?" ) );
        if( select_one_file.empty() ) {
            return drop_locations();
        }
        return { { select_one_file.execute(), 1 } };
    }
    inventory_multiselector select_multiple_efiles( who, preset, _( "Files selected:" ),
            make_raw_stats, /*allow_select_contained=*/true );
    if( !wiping ) {
        select_multiple_efiles.set_hint(
            _( "Using two devices without high-speed compatibility will result in longer transfer times.\nHaving an external storage device compatible with both devices can avoid this." ) );
    }
    for( item_location loc : from_edevices ) {
        select_multiple_efiles.add_contained_efiles( loc );
    }
    if( select_multiple_efiles.empty() ) {
        popup( std::string( _( "You have no files to " + action_name + "." ) ), PF_GET_KEY );
        return drop_locations();
    }
    select_multiple_efiles.set_title( _( "Select files to " + action_name ) );
    bool done = false;
    drop_locations selected_efiles;
    while( !done ) {
        selected_efiles = select_multiple_efiles.execute();
        units::ememory total_ememory = 0_KB;
        for( drop_location &d : selected_efiles ) {
            total_ememory += d.first->ememory_size();
        }
        if( wiping || ( total_ememory < to_edevice->remaining_ememory() || selected_efiles.empty() ) ) {
            done = true;
        } else {
            popup( std::string( _( "Selected file size exceeds available storage size." ) ), PF_GET_KEY );
        }
    }
    return selected_efiles;
}

class steal_inventory_preset : public pickup_inventory_preset
{
    public:
        explicit steal_inventory_preset( const Character &victim ) :
            pickup_inventory_preset( get_avatar() ), victim( victim ) {}

        bool is_shown( const item_location &loc ) const override {
            return !victim.is_worn( *loc ) && victim.get_wielded_item() != loc;
        }

    private:
        const Character &victim;
};

item_location game_menus::inv::steal( Character &victim )
{
    return inv_internal( victim, steal_inventory_preset( victim ),
                         string_format( _( "Steal from %s" ), victim.get_name() ), -1,
                         string_format( _( "%s's inventory is empty." ), victim.get_name() ) );
}

class weapon_inventory_preset: public inventory_selector_preset
{
    public:
        explicit weapon_inventory_preset( const Character &you ) : you( you ) {
            append_cell( [ this ]( const item_location & loc ) {
                if( !loc->is_gun() ) {
                    return std::string();
                }

                const int total_damage = loc->gun_damage( true ).total_damage();

                if( loc->has_ammo() ) {
                    const int basic_damage = loc->gun_damage( false ).total_damage();
                    const damage_instance &damage = loc->ammo_data()->ammo->damage;
                    if( !damage.empty() ) {
                        const float ammo_mult = damage.damage_units.front().unconditional_damage_mult;
                        if( ammo_mult != 1.0f ) {
                            return string_format( "%s<color_light_gray>*</color>%s <color_light_gray>=</color> %s",
                                                  get_damage_string( basic_damage, true ),
                                                  get_damage_string( ammo_mult, true ),
                                                  get_damage_string( total_damage, true )
                                                );
                        }
                    }

                    int ammo_damage;
                    if( loc->barrel_length().value() > 0 ) {
                        ammo_damage = damage.di_considering_length( loc->barrel_length() ).total_damage();
                    } else {
                        ammo_damage = damage.total_damage();
                    }

                    return string_format( "%s<color_light_gray>+</color>%s <color_light_gray>=</color> %s",
                                          get_damage_string( basic_damage, true ),
                                          get_damage_string( ammo_damage, true ),
                                          get_damage_string( total_damage, true )
                                        );
                } else {
                    return get_damage_string( total_damage );
                }
            }, pgettext( "Shot as damage", "SHOT" ) );

            for( const damage_type &dt : damage_type::get_all() ) {
                if( !dt.melee_only ) {
                    continue;
                }
                const damage_type_id &dtid = dt.id;
                append_cell( [ this, dtid ]( const item_location & loc ) {
                    return get_damage_string( loc->damage_melee( dtid ) );
                }, to_upper_case( dt.name.translated() ) );
            }

            append_cell( [ this ]( const item_location & loc ) {
                if( deals_melee_damage( *loc ) ) {
                    return good_bad_none( loc->get_to_hit() );
                }
                return std::string();
            }, _( "MELEE" ) );

            append_cell( [ this ]( const item_location & loc ) {
                if( deals_melee_damage( *loc ) ) {
                    return string_format( "<color_yellow>%d</color>", this->you.attack_speed( *loc ) );
                }
                return std::string();
            }, _( "MOVES" ) );

            append_cell( [this]( const item_location & loc ) {
                return string_format( "<color_yellow>%d</color>",
                                      loc.obtain_cost( this->you ) + ( this->you.is_wielding( *loc.get_item() ) ? 0 :
                                              ( *loc.get_item() ).on_wield_cost( this->you ) ) );
            }, _( "WIELD COST" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->made_of( phase_id::SOLID );
        }

        std::string get_denial( const item_location &loc ) const override {
            const auto ret = you.can_wield( *loc );

            if( !ret.success() ) {
                return trim_trailing_punctuations( ret.str() );
            }

            return std::string();
        }

    private:
        bool deals_melee_damage( const item &it ) const {
            for( const damage_type &dt : damage_type::get_all() ) {
                if( it.damage_melee( dt.id ) ) {
                    return true;
                }
            }
            return false;
        }

        std::string get_damage_string( float damage, bool display_zeroes = false ) const {
            return ( damage || display_zeroes ) ?
                   string_format( "<color_yellow>%g</color>", damage ) : std::string();
        }

        const Character &you;
};

item_location game_menus::inv::wield()
{
    return inv_internal( get_avatar(), weapon_inventory_preset( get_avatar() ), _( "Wield item" ), 1,
                         _( "You have nothing to wield." ) );
}

drop_locations game_menus::inv::holster( const item_location &holster )
{
    avatar &you = get_avatar();
    const std::string holster_name = holster->tname( 1, false );
    const use_function *use = holster->type->get_use( "holster" );
    const holster_actor *actor = use == nullptr ? nullptr : dynamic_cast<const holster_actor *>
                                 ( use->get_actor_ptr() );

    const std::string title = ( actor ? actor->holster_prompt.empty() : true )
                              ? string_format( _( "Put item into %s" ), holster->tname() )
                              : actor->holster_prompt.translated();
    const std::string hint = string_format( _( "Choose an item to put into your %s" ),
                                            holster_name );

    inventory_holster_preset holster_preset( holster, &get_avatar() );

    inventory_insert_selector insert_menu( you, holster_preset, _( "ITEMS TO INSERT" ),
                                           /*warn_liquid=*/false );
    insert_menu.add_character_items( you );
    insert_menu.add_nearby_items( 1 );
    insert_menu.set_display_stats( true );

    insert_menu.set_title( title );
    insert_menu.set_hint( hint );

    return insert_menu.execute();
}

void game_menus::inv::insert_items( item_location &holster )
{
    avatar &you = get_avatar();
    if( holster->will_spill_if_unsealed()
        && holster.where() != item_location::type::map
        && !you.is_wielding( *holster ) ) {

        you.add_msg_if_player( m_info, _( "The %s would spill unless it's on the ground or wielded." ),
                               holster->type_name() );
        return;
    }
    drop_locations holstered_list = game_menus::inv::holster( holster );

    if( !holstered_list.empty() ) {
        you.assign_activity( insert_item_activity_actor( holster, holstered_list ) );
    }
}

static bool valid_unload_container( const item_location &container )
{
    return
        // Item must be a container.
        container->is_container()
        // Item must be able to be unloaded
        && !container->has_flag( flag_NO_UNLOAD )
        // Container must contain at least one item
        && !container->empty_container()
        // Not all contents should not be liquid, gas or plasma
        && !container->contains_no_solids();
}

drop_locations game_menus::inv::unload_container()
{
    avatar &you = get_avatar();
    inventory_filter_preset unload_preset( valid_unload_container );

    inventory_drop_selector insert_menu( you, unload_preset, _( "CONTAINERS TO UNLOAD" ),
                                         /*warn_liquid=*/false );
    insert_menu.add_character_items( you );
    insert_menu.add_nearby_items( 1 );
    insert_menu.set_display_stats( false );

    insert_menu.set_title( _( "Select containers to unload" ) );

    drop_locations dropped;
    for( drop_location &droplc : insert_menu.execute() ) {
        for( item *it : droplc.first->all_items_top( pocket_type::CONTAINER, true ) ) {
            // no liquids and no items marked as favorite
            if( ( !it->made_of( phase_id::LIQUID ) || ( it->made_of( phase_id::LIQUID ) &&
                    it->is_frozen_liquid() ) ) && !it->is_favorite ) {
                dropped.emplace_back( item_location( droplc.first, it ), it->count() );
            }
        }
    }
    return dropped;
}

class saw_barrel_inventory_preset: public weapon_inventory_preset
{
    public:
        saw_barrel_inventory_preset( const Character &you, const item &tool,
                                     const saw_barrel_actor &actor ) :
            weapon_inventory_preset( you ), you( you ), tool( tool ), actor( actor ) {
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->is_gun();
        }

        std::string get_denial( const item_location &loc ) const override {
            const auto ret = actor.can_use_on( you, tool, *loc );

            if( !ret.success() ) {
                return trim_trailing_punctuations( ret.str() );
            }

            return std::string();
        }

    private:
        const Character &you;
        const item &tool;
        const saw_barrel_actor &actor;
};

class saw_stock_inventory_preset : public weapon_inventory_preset
{
    public:
        saw_stock_inventory_preset( const Character &you, const item &tool,
                                    const saw_stock_actor &actor ) :
            weapon_inventory_preset( you ), you( you ), tool( tool ), actor( actor ) {
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->is_gun();
        }

        std::string get_denial( const item_location &loc ) const override {
            const auto ret = actor.can_use_on( you, tool, *loc );

            if( !ret.success() ) {
                return trim_trailing_punctuations( ret.str() );
            }

            return std::string();
        }

    private:
        const Character &you;
        const item &tool;
        const saw_stock_actor &actor;
};

class attach_molle_inventory_preset : public inventory_selector_preset
{
    public:
        attach_molle_inventory_preset( const molle_attach_actor *actor, const item *vest ) :
            actor( actor ), vest( vest ) {
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->can_attach_as_pocket();
        }

        std::string get_denial( const item_location &loc ) const override {

            if( actor->size - vest->get_contents().get_additional_space_used() < loc->get_pocket_size() ) {
                return "not enough space left on the vest.";
            }

            {
                return std::string();
            }
        }

    private:
        const molle_attach_actor *actor;
        const item *vest;
};

class attach_veh_tool_inventory_preset : public inventory_selector_preset
{
    public:
        explicit attach_veh_tool_inventory_preset( const std::set<itype_id> &allowed_types )
            : allowed_types( allowed_types ) { }

        bool is_shown( const item_location &loc ) const override {
            return allowed_types.count( loc->typeId() );
        }

        std::string get_denial( const item_location &loc ) const override {
            const item &it = *loc.get_item();

            if( !it.empty() || !it.toolmods().empty() ) {
                return "item needs to be empty.";
            }

            return std::string();
        }

    private:
        const std::set<itype_id> allowed_types;
};

class salvage_inventory_preset: public inventory_selector_preset
{
    public:
        explicit salvage_inventory_preset( const salvage_actor *actor ) :
            actor( actor ) {

            append_cell( [ actor ]( const item_location & loc ) {
                return to_string_clipped( time_duration::from_turns( actor->time_to_cut_up(
                                              *loc.get_item() ) / 100 ) );
            }, _( "TIME" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return actor->valid_to_cut_up( nullptr, *loc.get_item() );
        }

    private:
        const salvage_actor *actor;
};

item_location game_menus::inv::salvage( Character &you, const salvage_actor *actor )
{
    return inv_internal( you, salvage_inventory_preset( actor ),
                         _( "Cut up what?" ), 1,
                         _( "You have nothing to cut up." ) );
}

class repair_inventory_preset: public inventory_selector_preset
{
    public:
        repair_inventory_preset( const repair_item_actor *actor, const item *main_tool, Character &you ) :
            actor( actor ), main_tool( main_tool ), character( you ) {

            _indent_entries = false;

            append_cell( [actor, &you]( const item_location & loc ) {
                const int comp_needed = std::max<int>( 1,
                                                       std::ceil( loc->base_volume() * actor->cost_scaling / 250_ml ) );
                const inventory &crafting_inv = you.crafting_inventory();
                std::function<bool( const item & )> filter;
                if( loc->is_filthy() ) {
                    filter = []( const item & component ) {
                        return component.allow_crafting_component();
                    };
                } else {
                    filter = is_crafting_component;
                }
                std::vector<std::string> material_list;
                for( const itype_id &component_id : actor->get_valid_repair_materials( *loc ) ) {
                    if( item::count_by_charges( component_id ) ) {
                        if( crafting_inv.has_charges( component_id, 1 ) ) {
                            const int num_comp = crafting_inv.charges_of( component_id );
                            material_list.push_back( colorize( string_format( _( "%s (%d)" ), item::nname( component_id ),
                                                               num_comp ), num_comp < comp_needed ? c_red : c_unset ) );
                        }
                    } else if( crafting_inv.has_amount( component_id, 1, false, filter ) ) {
                        const int num_comp = crafting_inv.amount_of( component_id, false );
                        material_list.push_back( colorize( string_format( _( "%s (%d)" ), item::nname( component_id ),
                                                           num_comp ), num_comp < comp_needed ? c_red : c_unset ) );
                    }
                }
                std::string ret = string_join( material_list, ", " );
                if( ret.empty() ) {
                    ret = _( "<color_red>NONE</color>" );
                }
                return ret;
            },
            _( "MATERIALS AVAILABLE" ) );

            append_cell( [actor, &you]( const item_location & loc ) {
                const int level = you.get_skill_level( actor->used_skill );
                const repair_item_actor::repair_type action_type = actor->default_action( *loc, level );
                const std::pair<float, float> chance = actor->repair_chance( you, *loc, action_type );
                return colorize( string_format( "%0.1f%%", 100.0f * chance.first ),
                                 chance.first == 0 ? c_yellow : ( chance.second == 0 ? c_light_green : c_unset ) );
            },
            _( "SUCCESS CHANCE" ) );

            append_cell( [actor, &you]( const item_location & loc ) {
                const int level = you.get_skill_level( actor->used_skill );
                const repair_item_actor::repair_type action_type = actor->default_action( *loc, level );
                const std::pair<float, float> chance = actor->repair_chance( you, *loc, action_type );
                return colorize( string_format( "%0.1f%%", 100.0f * chance.second ),
                                 chance.second > chance.first ? c_yellow : ( chance.second == 0 &&
                                         chance.first > 0 ? c_light_green : c_unset ) );
            },
            _( "DAMAGE CHANCE" ) );

            append_cell( [actor, &you]( const item_location & loc ) {
                const int difficulty = actor->repair_recipe_difficulty( *loc );
                return colorize( string_format( "%d", difficulty ),
                                 difficulty > you.get_skill_level( actor->used_skill ) ? c_red : c_unset );
            },
            _( "DIFFICULTY" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return actor->can_repair_target( character, *loc, false, false ) && ( &*loc != main_tool );
        }

    private:
        const repair_item_actor *actor;
        const item *main_tool;
        Character &character;
};

static std::string get_repair_hint( const Character &you, const repair_item_actor *actor,
                                    const item *main_tool )
{
    std::string hint = std::string();
    hint.append( string_format( _( "Tool: <color_cyan>%s</color>" ), main_tool->display_name() ) );
    hint.append( string_format( " | " ) );
    hint.append( string_format( _( "Skill used: <color_cyan>%s (%d)</color>" ),
                                actor->used_skill.obj().name(), static_cast<int>( you.get_skill_level( actor->used_skill ) ) ) );
    return hint;
}

item_location game_menus::inv::repair( Character &you, const repair_item_actor *actor,
                                       const item *main_tool )
{
    return inv_internal( you, repair_inventory_preset( actor, main_tool, you ),
                         _( "Repair what?" ), 1,
                         string_format( _( "You have no items that could be repaired with a %s." ),
                                        main_tool->type_name( 1 ) ), get_repair_hint( you, actor, main_tool ) );
}

item_location game_menus::inv::saw_barrel( Character &you, item &tool )
{
    const saw_barrel_actor *actor = dynamic_cast<const saw_barrel_actor *>
                                    ( tool.type->get_use( "saw_barrel" )->get_actor_ptr() );

    if( !actor ) {
        debugmsg( "Tried to use a wrong item." );
        return item_location();
    }

    return inv_internal( you, saw_barrel_inventory_preset( you, tool, *actor ),
                         _( "Saw barrel" ), 1,
                         _( "You don't have any guns." ),
                         string_format( _( "Choose a weapon to use your %s on" ),
                                        tool.tname( 1, false )
                                      )
                       );
}

item_location game_menus::inv::saw_stock( Character &you, item &tool )
{
    const saw_stock_actor *actor = dynamic_cast<const saw_stock_actor *>
                                   ( tool.type->get_use( "saw_stock" )->get_actor_ptr() );

    if( !actor ) {
        debugmsg( "Tried to use a wrong item." );
        return item_location();
    }

    return inv_internal( you, saw_stock_inventory_preset( you, tool, *actor ),
                         _( "Saw stock" ), 1,
                         _( "You don't have any guns." ),
                         string_format( _( "Choose a weapon to use your %s on" ),
                                        tool.tname( 1, false )
                                      )
                       );
}

item_location game_menus::inv::molle_attach( Character &you, item &tool )
{
    const molle_attach_actor *actor = dynamic_cast<const molle_attach_actor *>
                                      ( tool.type->get_use( "attach_molle" )->get_actor_ptr() );

    if( !actor ) {
        debugmsg( "Tried to use a wrong item." );
        return item_location();
    }

    const int vacancies = actor->size - tool.get_contents().get_additional_space_used();

    return inv_internal( you, attach_molle_inventory_preset( actor, &tool ),
                         _( "Attach an item to the vest" ), 1,
                         _( "You don't have any MOLLE compatible items." ),
                         string_format( "%s\n%s",
                                        string_format( _( "Choose an accessory to attach to your %s" ),
                                                tool.tname( 1, false ) ),
                                        string_format( n_gettext( "There is space for %d small item",
                                                "There is space for %d small items",
                                                vacancies ), vacancies )
                                      )
                       );
}

item_location game_menus::inv::veh_tool_attach( Character &you, const std::string &vp_name,
        const std::set<itype_id> &allowed_types )
{
    return inv_internal( you, attach_veh_tool_inventory_preset( allowed_types ),
                         string_format( _( "Attach an item to the %s" ), vp_name ), 1,
                         string_format( _( "You don't have any items compatible with %s.\n\nAllowed equipment:\n%s" ),
    vp_name, enumerate_as_string( allowed_types, []( const itype_id & it ) {
        return it->nname( 1 );
    } ) ),
    string_format( _( "Choose a tool to attach to %s" ), vp_name ) );
}

drop_locations game_menus::inv::multidrop( Character &you )
{
    you.inv->restack( you );

    const inventory_filter_preset preset( [ &you ]( const item_location & location ) {
        return you.can_drop( *location ).success() &&
               ( !location.get_item()->is_frozen_liquid() || !location.has_parent() ||
                 location.get_item()->has_flag( flag_SHREDDED ) );
    } );

    inventory_drop_selector inv_s( you, preset );

    inv_s.add_character_items( you );
    inv_s.set_title( _( "Multidrop" ) );
    inv_s.set_hint( _( "To drop x items, type a number before selecting." ) );

    if( inv_s.empty() ) {
        popup( std::string( _( "You have nothing to drop." ) ), PF_GET_KEY );
        return drop_locations();
    }

    return inv_s.execute();
}

drop_locations game_menus::inv::pickup( const std::optional<tripoint_bub_ms> &target,
                                        const std::vector<drop_location> &selection )
{
    avatar &you = get_avatar();
    pickup_inventory_preset preset( you, /*skip_wield_check=*/true, /*ignore_liquidcont=*/true );
    preset.save_state = &pickup_ui_default_state;

    pickup_selector pick_s( you, preset, _( "ITEMS TO PICK UP" ), target );

    // Add items from the selected tile, or from current and all surrounding tiles
    if( target ) {
        pick_s.add_vehicle_items( *target );
        pick_s.add_map_items( *target );
    } else {
        pick_s.add_nearby_items();
    }
    pick_s.set_title( _( "Pickup" ) );

    if( pick_s.empty() ) {
        if( target ) {
            add_msg( _( "There is nothing to pick up." ) );
        } else {
            add_msg( _( "There is nothing to pick up nearby." ) );
        }
        return drop_locations();
    }

    if( !selection.empty() ) {
        pick_s.apply_selection( selection );
    }

    return pick_s.execute();
}

class smokable_selector_preset : public inventory_selector_preset
{
    public:
        bool is_shown( const item_location &location ) const override {
            return !location->rotten() && location->has_flag( flag_SMOKABLE );
        }
};

drop_locations game_menus::inv::smoke_food( Character &you, units::volume total_capacity,
        units::volume used_capacity )
{
    const smokable_selector_preset preset;

    auto make_raw_stats = [ &total_capacity, &used_capacity ]
    ( const std::vector<std::pair<item_location, int>> &locs ) {
        units::volume added_volume = 0_ml;
        for( std::pair<item_location, int> loc : locs ) {
            added_volume += loc.first->volume() * loc.second / loc.first->count();
        }
        std::string volume_caption = string_format( _( "Volume (%s):" ), volume_units_abbr() );
        return inventory_selector::stats{
            {
                display_stat( volume_caption,
                              units::to_milliliter( used_capacity + added_volume ),
                              units::to_milliliter( total_capacity ), []( int v )
                {
                    return format_volume( units::from_milliliter( v ) );
                } )
            }
        };
    };

    inventory_multiselector smoke_s( you, preset, _( "FOOD TO SMOKE" ), make_raw_stats );

    smoke_s.add_nearby_items( PICKUP_RANGE );
    smoke_s.add_character_items( you );

    smoke_s.set_title( _( "Insert food into smoking rack" ) );
    smoke_s.set_hint( _( "To select x items, type a number before selecting." ) );
    smoke_s.set_invlet_type( inventory_selector::selector_invlet_type::SELECTOR_INVLET_ALPHA );

    if( smoke_s.empty() ) {
        popup( std::string( _( "You don't have any food that can be smoked." ) ), PF_GET_KEY );
        return drop_locations();
    }

    return smoke_s.execute();
}

game_menus::inv::compare_item_menu::compare_item_menu( const item &first, const item &second,
        const std::string &confirm_message ) :
    cataimgui::window( "compare", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                       ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoNavInputs ),
    first( first ),
    second( second ),
    confirm_message( confirm_message )
{
    ctxt.register_action( "HELP_KEYBINDINGS" );
    if( !confirm_message.empty() ) {
        ctxt.register_action( "CONFIRM" );
    }
    ctxt.register_action( "QUIT" );
    ctxt.register_navigate_ui_list();
    ctxt.set_timeout( 10 );

    // todo: regen info when toggling language?
    first.info( true, first_info );
    second.info( true, second_info );
}

static void draw_column( const std::string &label, const ImVec2 &size, const item &it,
                         std::vector<iteminfo> &first, std::vector<iteminfo> &second, cataimgui::scroll &s )
{
    if( ImGui::BeginChild( label.c_str(), size ) ) {
        cataimgui::TextColoredParagraph( c_light_gray, it.tname() );
        ImGui::NewLine();
        if( it.tname().find( it.type_name() ) == std::string::npos ) {
            cataimgui::TextColoredParagraph( c_light_gray, it.type_name() );
            ImGui::NewLine();
        }
        ImGui::NewLine();
        cataimgui::set_scroll( s );
        display_item_info( first, second );
    }
    ImGui::EndChild();
}

void game_menus::inv::compare_item_menu::draw_controls()
{
    float half_width = ImGui::GetContentRegionAvail().x / 2;
    float height = ImGui::GetContentRegionAvail().y;
    ImGuiStyle &style = ImGui::GetStyle();
    float spacing_x = style.ItemSpacing.x;
    float top_y = ImGui::GetCursorPosY();
    float confirm_height = confirm_message.empty() ? 0.f : ImGui::GetFrameHeightWithSpacing();

    // cataimgui::set_scroll resets scroll, so we need a copy for parallel scrolling
    // not ideal, but this is such a special use case it's probably not worth changing
    // todo: also synchronize scrolling when using mouse wheel/dragging scroll bar?
    cataimgui::scroll s_copy = s;

    draw_column( "compare_left", ImVec2( half_width - spacing_x, height - confirm_height ), first,
                 first_info, second_info, s );
    ImGui::SetCursorPos( { half_width + spacing_x, top_y } );
    draw_column( "compare_right", ImVec2( half_width - spacing_x, height - confirm_height ), second,
                 second_info, first_info, s_copy );

    if( !confirm_message.empty() ) {
        ImGui::AlignTextToFramePadding();
        cataimgui::TextColoredParagraph( c_white, confirm_message );
        // todo: spacing should be part of paragraph so it behaves the same as other elements
        ImGui::Spacing();
        ImGui::SameLine();
        action_button( "CONFIRM", ctxt.get_button_text( "CONFIRM" ) );
        ImGui::SameLine();
        action_button( "QUIT", ctxt.get_button_text( "QUIT" ) );
    }
}

cataimgui::bounds game_menus::inv::compare_item_menu::get_bounds()
{
    return { 0.f, 0.f, ImGui::GetMainViewport()->Size.x, ImGui::GetMainViewport()->Size.y };
}

bool game_menus::inv::compare_item_menu::show()
{
    while( true ) {
        ui_manager::redraw();

        std::string action = has_button_action() ? get_button_action() : ctxt.handle_input();

        if( action == "UP" ) {
            s = cataimgui::scroll::line_up;
        } else if( action == "DOWN" ) {
            s = cataimgui::scroll::line_down;
        } else if( action == "PAGE_UP" ) {
            s = cataimgui::scroll::page_up;
        } else if( action == "PAGE_DOWN" ) {
            s = cataimgui::scroll::page_down;
        } else if( action == "HOME" ) {
            s = cataimgui::scroll::begin;
        } else if( action == "END" ) {
            s = cataimgui::scroll::end;
        } else if( action == "CONFIRM" ) {
            return true;
        } else if( action == "QUIT" ) {
            return false;
        }
    }

    return false;
}

void game_menus::inv::compare( const std::optional<tripoint_rel_ms> &offset )
{
    avatar &you = get_avatar();
    you.inv->restack( you );

    inventory_compare_selector inv_s( you );

    inv_s.add_character_items( you );
    inv_s.set_title( _( "Compare" ) );
    inv_s.set_hint( _( "Select two items to compare them." ) );

    if( offset ) {
        inv_s.add_map_items( you.pos_bub() + *offset );
        inv_s.add_vehicle_items( you.pos_bub() + *offset );
    } else {
        inv_s.add_nearby_items();
    }

    if( inv_s.empty() ) {
        popup( std::string( _( "There are no items to compare." ) ), PF_GET_KEY );
        return;
    }

    do {
        const auto to_compare = inv_s.execute();

        if( to_compare.first == nullptr || to_compare.second == nullptr ) {
            break;
        }

        compare_item_menu menu( *to_compare.first, *to_compare.second );
        menu.show();
    } while( true );
}

void game_menus::inv::reassign_letter( item &it )
{
    avatar &you = get_avatar();
    while( true ) {
        const int invlet = popup_getkey(
                               _( "Enter new letter.  Press SPACE to clear a manually-assigned letter, ESCAPE to cancel." ) );

        if( invlet == KEY_ESCAPE ) {
            break;
        } else if( invlet == ' ' ) {
            you.reassign_item( it, 0 );
            const std::string auto_setting = get_option<std::string>( "AUTO_INV_ASSIGN" );
            if( auto_setting == "enabled" || ( auto_setting == "favorites" && it.is_favorite ) ) {
                popup_getkey(
                    _( "Note: The Auto Inventory Letters setting might still reassign a letter to this item.\n"
                       "If this is undesired, you may wish to change the setting in Options." ) );
            }
            break;
        } else if( inv_chars.valid( invlet ) ) {
            you.reassign_item( it, invlet );
            break;
        }
    }
}

void game_menus::inv::swap_letters()
{
    avatar &you = get_avatar();
    you.inv->restack( you );

    inventory_pick_selector inv_s( you );

    inv_s.add_character_items( you );
    inv_s.set_title( _( "Swap Inventory Letters" ) );
    inv_s.set_display_stats( false );

    if( inv_s.empty() ) {
        popup( std::string( _( "Your inventory is empty." ) ), PF_GET_KEY );
        return;
    }

    while( true ) {
        const std::string invlets = colorize_symbols( inv_chars.get_allowed_chars(),
        [ &you ]( const std::string::value_type & elem ) {
            if( you.inv->assigned_invlet.count( elem ) ) {
                return c_yellow;
            } else if( you.invlet_to_item( elem ) != nullptr ) {
                return c_white;
            } else {
                return c_dark_gray;
            }
        } );

        inv_s.set_hint( invlets );

        item_location loc = inv_s.execute();

        if( !loc ) {
            break;
        }

        reassign_letter( *loc );
    }
}

static item_location autodoc_internal( Character &you, Character &patient,
                                       const inventory_selector_preset &preset, int radius, bool surgeon = false )
{
    inventory_pick_selector inv_s( you, preset );
    std::string hint;
    int drug_count = 0;

    if( !surgeon ) {//surgeon use their own anesthetic, player just need money
        if( patient.has_flag( json_flag_PAIN_IMMUNE ) ) {
            hint = _( "<color_yellow>Patient has deadened nerves.  Anesthesia unneeded.</color>" );
        } else if( patient.has_bionic( bio_painkiller ) ) {
            hint = _( "<color_yellow>Patient has Sensory Dulling CBM installed.  Anesthesia unneeded.</color>" );
        } else {
            const inventory &crafting_inv = you.crafting_inventory();
            std::vector<const item *> a_filter = crafting_inv.items_with( []( const item & it ) {
                return it.has_quality( qual_ANESTHESIA );
            } );
            for( const item *anesthesia_item : a_filter ) {
                if( anesthesia_item->ammo_remaining( ) >= 1 ) {
                    drug_count += anesthesia_item->ammo_remaining( );
                }
            }
            hint = string_format( _( "<color_yellow>Available anesthetic: %i mL</color>" ), drug_count );
        }
    }

    std::vector<const item *> install_programs = patient.crafting_inventory().items_with( [](
                const item & it ) -> bool { return it.has_flag( flag_BIONIC_INSTALLATION_DATA ); } );

    if( !install_programs.empty() ) {
        hint += string_format(
                    _( "\n<color_light_green>Found bionic installation data.  Affected CBMs are marked with an asterisk.</color>" ) );
    }

    inv_s.set_title( string_format( _( "Bionic installation patient: %s" ), patient.get_name() ) );

    inv_s.set_hint( hint );
    inv_s.set_display_stats( false );

    do {
        you.inv->restack( you );

        inv_s.clear_items();
        inv_s.add_character_items( you );
        if( you.getID() != patient.getID() ) {
            inv_s.add_character_items( patient );
        }
        inv_s.add_nearby_items( radius );

        if( inv_s.empty() ) {
            popup( _( "You don't have any bionics to install." ), PF_GET_KEY );
            return item_location();
        }

        item_location location = inv_s.execute();

        return location;

    } while( true );
}

// Menu used by Autodoc when installing a bionic
class bionic_install_preset: public inventory_selector_preset
{
    public:
        bionic_install_preset( Character &you, Character &patient ) :
            you( you ), pa( patient ) {
            append_cell( [ this ]( const item_location & loc ) {
                return get_failure_chance( loc );
            }, _( "FAILURE CHANCE" ) );

            append_cell( [ this ]( const item_location & loc ) {
                return get_operation_duration( loc );
            }, _( "OPERATION DURATION" ) );

            append_cell( [this]( const item_location & loc ) {
                return get_anesth_amount( loc );
            }, _( "ANESTHETIC REQUIRED" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->is_bionic();
        }

        std::string get_denial( const item_location &loc ) const override {

            const ret_val<void> installable = pa.is_installable( loc.get_item(), true );
            if( installable.success() && !you.has_enough_anesth( *loc.get_item()->type, pa ) ) {
                const int weight = units::to_kilogram( pa.bodyweight() ) / 10;
                const int duration = loc.get_item()->type->bionic->difficulty * 2;
                const requirement_data req_anesth = *requirement_data_anesthetic *
                                                    duration * weight;
                return string_format( _( "%i mL" ), req_anesth.get_tools().front().front().count );
            } else if( !installable.success() ) {
                return installable.str();
            }

            return std::string();
        }

    protected:
        Character &you;
        Character &pa;

    private:
        // Returns a formatted string of how long the operation will take.
        std::string get_operation_duration( const item_location &loc ) {
            const int difficulty = loc.get_item()->type->bionic->difficulty;
            // 20 minutes per bionic difficulty.
            return to_string( time_duration::from_minutes( difficulty * 20 ) );
        }

        // Failure chance for bionic install. Combines multiple other functions together.
        std::string get_failure_chance( const item_location &loc ) {

            const int difficulty = loc.get_item()->type->bionic->difficulty;
            int chance_of_failure = 100;
            Character &installer = you;

            std::vector<const item *> install_programs = you.crafting_inventory().items_with( [loc](
                        const item & it ) -> bool { return it.typeId() == loc.get_item()->type->bionic->installation_data; } );

            const bool has_install_program = !install_programs.empty();

            if( get_player_character().has_trait( trait_DEBUG_BIONICS ) ||
                get_player_character().has_flag( json_flag_MANUAL_CBM_INSTALLATION ) ) {
                chance_of_failure = 0;
            } else {
                chance_of_failure = 100 - bionic_success_chance( true, has_install_program ? 10 : -1, difficulty,
                                    installer );
            }

            return string_format( has_install_program ? _( "<color_white>*</color> %i%%" ) : _( "%i%%" ),
                                  chance_of_failure );
        }

        std::string get_anesth_amount( const item_location &loc ) {

            const int weight = units::to_kilogram( pa.bodyweight() ) / 10;
            const int duration = loc.get_item()->type->bionic->difficulty * 2;
            const requirement_data req_anesth = *requirement_data_anesthetic *
                                                duration * weight;
            int count = 0;
            if( !req_anesth.get_tools().empty() && !req_anesth.get_tools().front().empty() ) {
                count = req_anesth.get_tools().front().front().count;
            }
            return string_format( _( "%i mL" ), count );
        }
};

// Menu used by surgeon when installing a bionic
class bionic_install_surgeon_preset : public inventory_selector_preset
{
    public:
        bionic_install_surgeon_preset( Character &you, Character &patient ) :
            you( you ), pa( patient ) {
            append_cell( [this]( const item_location & loc ) {
                return get_failure_chance( loc );
            }, _( "FAILURE CHANCE" ) );

            append_cell( [this]( const item_location & loc ) {
                return get_operation_duration( loc );
            }, _( "OPERATION DURATION" ) );

            append_cell( [this]( const item_location & loc ) {
                return get_money_amount( loc );
            }, _( "PRICE" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return ( loc->is_owned_by( you ) || loc->is_owned_by( pa ) ) && loc->is_bionic();
        }

        std::string get_denial( const item_location &loc ) const override {
            if( you.is_npc() ) {
                int const price = npc_trading::bionic_install_price( you, pa, loc );
                ret_val<void> const refusal =
                    you.as_npc()->wants_to_sell( loc, price );
                if( !refusal.success() ) {
                    std::string refused_text = refusal.str();
                    parse_tags( refused_text, you, pa );
                    return refused_text;
                }
            }
            const ret_val<void> installable = pa.is_installable( loc.get_item(), false );
            return installable.str();
        }

    protected:
        Character &you;
        Character &pa;

    private:
        // Returns a formatted string of how long the operation will take.
        std::string get_operation_duration( const item_location &loc ) {
            const int difficulty = loc.get_item()->type->bionic->difficulty;
            // 20 minutes per bionic difficulty.
            return to_string( time_duration::from_minutes( difficulty * 20 ) );
        }

        // Failure chance for bionic install. Combines multiple other functions together.
        std::string get_failure_chance( const item_location &loc ) {

            const int difficulty = loc.get_item()->type->bionic->difficulty;
            int chance_of_failure = 100;
            Character &installer = you;

            if( get_player_character().has_trait( trait_DEBUG_BIONICS ) ||
                get_player_character().has_flag( json_flag_MANUAL_CBM_INSTALLATION ) ) {
                chance_of_failure = 0;
            } else {
                chance_of_failure = 100 - bionic_success_chance( true, 20, difficulty, installer );
            }

            return string_format( _( "%i%%" ), chance_of_failure );
        }

        std::string get_money_amount( const item_location &loc ) {
            return format_money( npc_trading::bionic_install_price( you, pa, loc ) );
        }
};

item_location game_menus::inv::install_bionic( Character &installer, Character &patron,
        Character &patient, bool surgeon )
{
    if( surgeon ) {
        return autodoc_internal( patron, patient, bionic_install_surgeon_preset( installer, patient ), 5,
                                 surgeon );
    } else {
        return autodoc_internal( patron, patient, bionic_install_preset( installer, patient ), 5 );
    }

}

class change_sprite_inventory_preset: public inventory_selector_preset
{
    public:
        explicit change_sprite_inventory_preset( Character &pl ) :
            you( pl ) {
            append_cell( []( const item_location & loc ) -> std::string {
                if( loc->has_var( "sprite_override" ) ) {
                    const itype_id sprite_override( std::string( loc->get_var( "sprite_override" ) ) );
                    const std::string variant = loc->get_var( "sprite_override_variant" );
                    if( !item::type_is_defined( sprite_override ) ) {
                        return _( "Unknown" );
                    }
                    item tmp( sprite_override );
                    tmp.set_itype_variant( variant );
                    return tmp.tname();
                }
                return _( "Default" );
            }, _( "Shown as" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            return loc->is_armor();
        }

    protected:
        Character &you;
};

item_location game_menus::inv::change_sprite( Character &you )
{
    return inv_internal( you, change_sprite_inventory_preset( you ),
                         _( "Change appearance of your armor:" ), -1,
                         _( "You have nothing to wear." ) );
}

std::pair<item_location, bool> game_menus::inv::unload( Character &you )
{

    const inventory_filter_preset preset( [&you]( const item_location & location ) {
        return you.rate_action_unload( *location ) == hint_rating::good;
    } );
    unload_selector inv_s( you, preset );

    inv_s.set_title( _( "Unload item" ) );
    inv_s.set_display_stats( false );

    you.inv->restack( you );

    inv_s.clear_items();

    inv_s.add_character_items( you );
    inv_s.add_nearby_items( 1 );

    if( inv_s.empty() ) {
        popup( _( "You have nothing to unload." ), PF_GET_KEY );
        return std::make_pair( item_location(), false );
    }

    return inv_s.execute();
}

class select_ammo_inventory_preset : public inventory_selector_preset
{
    public:
        select_ammo_inventory_preset( Character &you, const item_location &target,
                                      bool empty ) : you( you ),
            target( target ), empty( empty ) {
            _indent_entries = false;
            _collate_entries = true;

            append_cell( [&you]( const item_location & loc ) {
                bool is_ammo_container = loc->is_ammo_container();
                Character &player_character = get_player_character();
                if( is_ammo_container || loc->is_container() ) {
                    if( is_ammo_container && you.is_worn( *loc ) ) {
                        return loc->type_name();
                    }
                    return string_format( _( "%s, %s" ), loc->type_name(), loc.describe( &player_character ) );
                }
                return loc.describe( &player_character );
            }, _( "LOCATION" ) );

            append_cell( [&you, target]( const item_location & loc ) {
                for( const item_location &opt : get_possible_reload_targets( target ) ) {
                    if( opt.can_reload_with( loc, true ) ) {
                        if( opt == target ) {
                            return std::string();
                        }
                        return string_format( _( "%s, %s" ), opt->type_name(), opt.describe( &you ) );
                    }
                }
                return std::string();
            }, _( "DESTINATION" ) );

            append_cell( []( const inventory_entry & entry ) {
                if( entry.any_item()->is_ammo() ) {
                    return std::to_string( entry.chosen_count );
                }
                return std::string();
            }, _( "AMOUNT" ) );

            append_cell( [&you, &target]( const item_location & loc ) {
                item::reload_option opt( &you, target, loc );
                return std::to_string( opt.moves() );
            }, _( "MOVES" ) );

            append_cell( []( const item_location & loc ) {
                const itype *ammo = loc->is_ammo_container() ? loc->first_ammo().ammo_data() :
                                    loc->ammo_data();
                if( ammo ) {
                    const damage_instance &dam = ammo->ammo->damage;
                    return std::to_string( static_cast<int>( dam.total_damage() ) );
                }
                return std::string();
            }, _( "DAMAGE" ) );

            append_cell( []( const item_location & loc ) {
                const itype *ammo = loc->is_ammo_container() ? loc->first_ammo().ammo_data() :
                                    loc->ammo_data();
                if( ammo ) {
                    const damage_instance &dam = ammo->ammo->damage;
                    return std::to_string( static_cast<int>( dam.empty() ? 0.0f : ( *dam.begin() ).res_pen ) );
                }
                return std::string();
            }, _( "PIERCE" ) );
        }

        bool is_shown( const item_location &loc ) const override {
            // todo: allow to reload a magazine/magazine well from a container pocket on the same item
            map &here = get_map();

            if( loc.parent_item() == target ) {
                return false;
            }

            if( loc->made_of( phase_id::LIQUID ) && loc.where() == item_location::type::map ) {
                if( !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_LIQUIDCONT, loc.pos_bub( here ) ) ) {
                    return false;
                }
            }

            if( loc->is_frozen_liquid() ) {
                return false;
            }

            if( !empty && loc->is_magazine() && !loc->ammo_remaining( ) ) {
                return false;
            }

            std::vector<item_location> opts = get_possible_reload_targets( target );

            for( item_location &p : opts ) {
                if( ( loc->has_flag( flag_SPEEDLOADER ) && p->allows_speedloader( loc->typeId() ) &&
                      loc->ammo_remaining( ) > 1 && p->ammo_remaining( ) < 1 ) &&
                    p.can_reload_with( loc, true ) ) {
                    return true;
                }

                if( p.can_reload_with( loc, true ) ) {
                    return true;
                }
            }

            return false;
        }

        // sort in order of move cost (ascending), then remaining ammo (descending) with empty magazines always last
        bool sort_compare( const inventory_entry &lhs, const inventory_entry &rhs ) const override {
            item_location left = lhs.any_item();
            item_location right = rhs.any_item();

            if( left->ammo_remaining( ) == 0 || right->ammo_remaining( ) == 0 ) {
                return ( left->ammo_remaining( ) != 0 ) > ( right->ammo_remaining( ) != 0 );
            }

            if( left.obtain_cost( you ) != right.obtain_cost( you ) ) {
                return left.obtain_cost( you ) < right.obtain_cost( you );
            }

            if( left->ammo_remaining( ) != right->ammo_remaining( ) ) {
                return left->ammo_remaining( ) > right->ammo_remaining( );
            }

            return inventory_selector_preset::sort_compare( lhs, rhs );
        }

    private:
        Character &you;
        const item_location target;
        bool empty;
};

item::reload_option game_menus::inv::select_ammo( Character &you, const item_location &loc,
        bool prompt, bool empty )
{
    const select_ammo_inventory_preset preset( you, loc, empty );
    ammo_inventory_selector inv_s( you, loc, preset );

    inv_s.set_title( string_format( loc->is_watertight_container() ? _( "Refill %s" ) :
                                    loc->has_flag( flag_RELOAD_AND_SHOOT ) ? _( "Select ammo for %s" ) : _( "Reload %s" ),
                                    loc->display_name() ) );
    inv_s.set_hint( _( "Choose ammo to reload" ) );
    inv_s.set_display_stats( false );

    inv_s.clear_items();
    inv_s.add_character_items( you );
    inv_s.add_nearby_items( 1 );
    inv_s.set_all_entries_chosen_count();

    if( inv_s.empty() ) {
        popup( _( "You have nothing to reload." ), PF_GET_KEY );
        return item::reload_option();
    }

    drop_location selected;
    if( !prompt && inv_s.item_entry_count() == 1 ) {
        selected = inv_s.get_only_choice();
    } else {
        selected = inv_s.execute();
    }

    if( !selected.first ) {
        return item::reload_option();
    }

    item_location target_loc;
    for( const item_location &opt : get_possible_reload_targets( loc ) ) {
        if( opt.can_reload_with( selected.first, true ) ) {
            target_loc = opt;
            break;
        }
    }

    item::reload_option opt( &you, target_loc, selected.first );
    opt.qty( selected.second );

    return opt;
}
