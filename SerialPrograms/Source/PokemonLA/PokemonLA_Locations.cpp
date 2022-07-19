/*  Pokemon Legends Arceus Locations
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include <map>
#include "PokemonLA_Locations.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonLA{



const char* MAP_REGION_NAMES[] = {
    "None",
    "Jubilife Village",
    "Obsidian Fieldlands",
    "Crimson Mirelands",
    "Cobalt Coastlands",
    "Coronet Highlands",
    "Alabaster Icelands",
    "Ancient Retreat",
};

const char* WILD_REGION_SHORT_NAMES[] = {
    "Fieldlands",
    "Mirelands",
    "Coastlands",
    "Highlands",
    "Icelands",
};

bool is_wild_land(MapRegion region){
    return region == MapRegion::FIELDLANDS || region == MapRegion::MIRELANDS || region == MapRegion::COASTLANDS
           || region == MapRegion::HIGHLANDS || region == MapRegion::ICELANDS;
}

}
}
}
