//  $Id: worldmap.cpp 3327 2006-04-13 15:02:40Z ravu_al_hemio $
//
//  SuperTux -  A Jump'n Run
//  Copyright (C) 2004 Ingo Ruhnke <grumbel@gmx.de>
//  Copyright (C) 2006 Christoph Sommer <christoph.sommer@2006.expires.deltadevelopment.de>
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#include <config.h>

#include "tux.hpp"
#include "sprite/sprite_manager.hpp"
#include "video/drawing_context.hpp"
#include "player_status.hpp"
#include "worldmap.hpp"
#include "level.hpp"
#include "special_tile.hpp"
#include "control/joystickkeyboardcontroller.hpp"
#include "main.hpp"

namespace WorldMapNS
{

static const float TUXSPEED = 200; 
static const float map_message_TIME = 2.8;

Tux::Tux(WorldMap* worldmap_)
  : worldmap(worldmap_)
{
  sprite.reset(sprite_manager->create("images/worldmap/common/tux.sprite"));
  
  offset = 0;
  moving = false;
  direction = D_NONE;
  input_direction = D_NONE;
}

Tux::~Tux()
{
}

void
Tux::draw(DrawingContext& context)
{
  switch (player_status->bonus) {
    case GROWUP_BONUS:
      sprite->set_action(moving ? "large-walking" : "large-stop");
      break;
    case FIRE_BONUS:
      sprite->set_action(moving ? "fire-walking" : "fire-stop");
      break;
    case NO_BONUS:
      sprite->set_action(moving ? "small-walking" : "small-stop");
      break;
    default:
      log_debug << "Bonus type not handled in worldmap." << std::endl;
      sprite->set_action("large-stop");
      break;
  }

  sprite->draw(context, get_pos(), LAYER_OBJECTS);
}


Vector
Tux::get_pos()
{
  float x = tile_pos.x * 32;
  float y = tile_pos.y * 32;

  switch(direction)
    {
    case D_WEST:
      x -= offset - 32;
      break;
    case D_EAST:
      x += offset - 32;
      break;
    case D_NORTH:
      y -= offset - 32;
      break;
    case D_SOUTH:
      y += offset - 32;
      break;
    case D_NONE:
      break;
    }
  
  return Vector(x, y);
}

void
Tux::stop()
{
  offset = 0;
  direction = D_NONE;
  input_direction = D_NONE;
  moving = false;
}

void
Tux::set_direction(Direction dir)
{
  input_direction = dir;
}

void 
Tux::tryStartWalking() 
{
  if (moving)
    return;  
  if (input_direction == D_NONE)
    return;

  Level* level = worldmap->at_level();

  // We got a new direction, so lets start walking when possible
  Vector next_tile;
  if ((!level || level->solved) && worldmap->path_ok(input_direction, tile_pos, &next_tile))
  {
    tile_pos = next_tile;
    moving = true;
    direction = input_direction;
    back_direction = reverse_dir(direction);
  }
  else if (input_direction == back_direction)
  {
    moving = true;
    direction = input_direction;
    tile_pos = worldmap->get_next_tile(tile_pos, direction);
    back_direction = reverse_dir(direction);
  }

}

bool 
Tux::canWalk(const Tile* tile, Direction dir)
{
  return ((tile->getData() & Tile::WORLDMAP_NORTH && dir == D_NORTH) ||
	  (tile->getData() & Tile::WORLDMAP_SOUTH && dir == D_SOUTH) ||
	  (tile->getData() & Tile::WORLDMAP_EAST && dir == D_EAST) ||
	  (tile->getData() & Tile::WORLDMAP_WEST && dir == D_WEST));
}

void 
Tux::tryContinueWalking(float elapsed_time)
{
  if (!moving) return;

  // Let tux walk
  offset += TUXSPEED * elapsed_time;

  // Do nothing if we have not yet reached the next tile
  if (offset <= 32) return;

  offset -= 32;

  // if this is a special_tile with passive_message, display it
  SpecialTile* special_tile = worldmap->at_special_tile();
  if(special_tile && special_tile->passive_message)
  {  
    // direction and the apply_action_ are opposites, since they "see"
    // directions in a different way
    if((direction == D_NORTH && special_tile->apply_action_south) ||
		    (direction == D_SOUTH && special_tile->apply_action_north) ||
		    (direction == D_WEST && special_tile->apply_action_east) ||
		    (direction == D_EAST && special_tile->apply_action_west))
    {
      worldmap->passive_message = special_tile->map_message;
      worldmap->passive_message_timer.start(map_message_TIME);
    }
  }

  // stop if we reached a level, a WORLDMAP_STOP tile or a special tile without a passive_message
  if ((worldmap->at_level()) || (worldmap->at(tile_pos)->getData() & Tile::WORLDMAP_STOP) || (special_tile && !special_tile->passive_message))
  {
    if(special_tile && !special_tile->map_message.empty() && !special_tile->passive_message) worldmap->passive_message_timer.start(0);
    stop();
    return;
  }

  // if user wants to change direction, try changing, else guess the direction in which to walk next
  const Tile* tile = worldmap->at(tile_pos);
  if (direction != input_direction)
  { 
    if(canWalk(tile, input_direction))
    {  
      direction = input_direction;
      back_direction = reverse_dir(direction);
    }
  }
  else
  {
    Direction dir = D_NONE;
    if (tile->getData() & Tile::WORLDMAP_NORTH && back_direction != D_NORTH) dir = D_NORTH;
    else if (tile->getData() & Tile::WORLDMAP_SOUTH && back_direction != D_SOUTH) dir = D_SOUTH;
    else if (tile->getData() & Tile::WORLDMAP_EAST && back_direction != D_EAST) dir = D_EAST;
    else if (tile->getData() & Tile::WORLDMAP_WEST && back_direction != D_WEST) dir = D_WEST;

    if (dir == D_NONE) 
    {
      // Should never be reached if tiledata is good
      log_warning << "Could not determine where to walk next" << std::endl;
      stop();
      return;
    }

    direction = dir;
    input_direction = direction;
    back_direction = reverse_dir(direction);
  }

  // Walk automatically to the next tile
  if(direction != D_NONE)
  {
    Vector next_tile;
    if (worldmap->path_ok(direction, tile_pos, &next_tile))
    {
      tile_pos = next_tile;
    }
    else
    {
      log_warning << "Tilemap data is buggy" << std::endl;
      stop();
    }
  }
}

void
Tux::updateInputDirection()
{
  if(main_controller->hold(Controller::UP))
    input_direction = D_NORTH;
  else if(main_controller->hold(Controller::DOWN))
    input_direction = D_SOUTH;
  else if(main_controller->hold(Controller::LEFT))
    input_direction = D_WEST;
  else if(main_controller->hold(Controller::RIGHT))
    input_direction = D_EAST;
}

void
Tux::update(float elapsed_time)
{
  updateInputDirection(); 
  if (moving) tryContinueWalking(elapsed_time); else tryStartWalking();
}

}