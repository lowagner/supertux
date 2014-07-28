//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "audio/sound_manager.hpp"
#include "badguy/bomb.hpp"
#include "object/explosion.hpp"
#include "object/player.hpp"
#include "sprite/sprite.hpp"
#include "supertux/sector.hpp"

/** if the player throws the bomb straight up... */
static const float THROW_UP_SPEED = -700;
/** if the player throws the bomb straight down... */
static const float THROW_DOWN_SPEED = 300;
/** or if the player throws in a certain direction, he also throws it up */
static const float THROW_SPEED_Y = -300;
static const float THROW_SPEED_X = 350;
/** velocity gets hit by this fraction when bomb hits ground */
static const float HIT_GROUND_FRACTION_VELOCITY_X = 0.8;
/** when hitting back, bomb bounces back a bit with this fraction of velocity */
static const float BOMB_VELOCITY_HIT_WALL_MULTILPLIER = 0.10f;
/** (de)-acceleration when bomb hits ground and is moving */
static const float GROUND_FRICTION = 250;

Bomb::Bomb(const Vector& pos, Direction dir, std::string custom_sprite /*= "images/creatures/mr_bomb/mr_bomb.sprite"*/ ) :
  BadGuy( pos, dir, custom_sprite ), 
  state(),
  grabbed(false), 
  grabber(NULL),
  ticking()
{
  state = STATE_TICKING;
  set_action(dir == LEFT ? "ticking-left" : "ticking-right", 1);
  countMe = false;

  ticking.reset(sound_manager->create_sound_source("sounds/fizz.wav"));
  ticking->set_position(get_pos());
  ticking->set_looping(true);
  ticking->set_gain(2.0);
  ticking->set_reference_distance(32);
  ticking->play();
}

void
Bomb::collision_solid(const CollisionHit& hit)
{
  if(hit.bottom) {
     physic.set_velocity_y(0);
     physic.set_velocity_x(HIT_GROUND_FRACTION_VELOCITY_X*physic.get_velocity_x());
  } else if(hit.top) {
    float vy = physic.get_velocity_y();
    if(vy < 0)
      physic.set_velocity_y( -vy * BOMB_VELOCITY_HIT_WALL_MULTILPLIER );
  } else if (hit.left) {
    float vx = physic.get_velocity_x();
    if ( vx < 0 ) {
      physic.set_velocity_x( -BOMB_VELOCITY_HIT_WALL_MULTILPLIER * vx );
      std::cout << "    BOMB HIT, new vx = " << vx << std::endl;
    } 
  } else if (hit.right) {
    float vx = physic.get_velocity_x();
    if ( vx > 0 ) {
      physic.set_velocity_x( -BOMB_VELOCITY_HIT_WALL_MULTILPLIER * vx );
      std::cout << "    BOMB HIT, new vx = " << vx << std::endl;
    }
  }

  update_on_ground_flag(hit);
}

HitResponse
Bomb::collision_player(Player& , const CollisionHit& )
{
  return ABORT_MOVE;
}

HitResponse
Bomb::collision_badguy(BadGuy& , const CollisionHit& )
{
  return ABORT_MOVE;
}

void
Bomb::active_update(float elapsed_time)
{
  ticking->set_position(get_pos());
  if(sprite->animation_done()) {
    explode();
  } else if (!grabbed) {
    movement = physic.get_movement(elapsed_time);
    if ( on_ground() ) {
      if(physic.get_velocity_x() < 0) {
        physic.set_acceleration_x(GROUND_FRICTION);
      } else if(physic.get_velocity_x() > 0) {
        physic.set_acceleration_x(-GROUND_FRICTION);
      } // no friction for physic.get_velocity_x() == 0
    }
  }
}

void
Bomb::explode()
{
  ticking->stop();

  // Make the player let go before we explode, otherwise the player is holding
  // an invalid object. There's probably a better way to do this than in the
  // Bomb class.
  if (grabber != NULL) {
    Player* player = dynamic_cast<Player*>(grabber);
    
    if (player)
      player->stop_grabbing();
  }

  if(is_valid()) {
    remove_me();
    Explosion* explosion = new Explosion(get_bbox().get_middle());
    Sector::current()->add_object(explosion);
  }

  run_dead_script();
}

void
Bomb::kill_fall()
{
  explode();
}

void
Bomb::grab(MovingObject& object, const Vector& pos, Direction dir)
{
  movement = pos - get_pos();
  this->dir = dir;

  // We actually face the opposite direction of Tux here to make the fuse more
  // visible instead of hiding it behind Tux
  sprite->set_action_continued(dir == LEFT ? "ticking-right" : "ticking-left");
  set_colgroup_active(COLGROUP_DISABLED);
  grabbed = true;
  grabber = &object;
}

void
Bomb::ungrab(MovingObject& object, Direction dir)
{
  this->dir = dir;
  // here we chuck the bomb away from tux

//  float vx = physic.get_velocity_x();  // FIXME:  i don't think these are working quite right.
//  float vy = physic.get_velocity_y(); // would want them to be the velocity of Tux.
  Vector mov = object.get_movement();
  float vx = mov.x; // but I DON't think these are working, either.
  float vy = mov.y;
  if ( dir == UP ) {
    // first set the position a bit far away from us, so we can't grab it right after chucking it
    set_pos(object.get_pos() - Vector(0, get_bbox().get_height()*0.66666 - 16));
    physic.set_velocity_y( vy + THROW_UP_SPEED );
  } else if ( dir == LEFT ) {
    set_pos(object.get_pos() + Vector(-16, get_bbox().get_height()*0.66666 - 32));
    physic.set_velocity_y( vy + THROW_SPEED_Y );
    physic.set_velocity_x( vx - THROW_SPEED_X );
  } else if ( dir == RIGHT ) {
    set_pos(object.get_pos() + Vector(16, get_bbox().get_height()*0.66666 - 32));
    physic.set_velocity_y( vy + THROW_SPEED_Y );
    physic.set_velocity_x( vx + THROW_SPEED_X );
  } else if ( dir == DOWN ) {
    // first set the position a bit far away from us, so we can't grab it right after chucking it
    set_pos(object.get_pos() + Vector(0, get_bbox().get_height()*0.66666 - 16));
    physic.set_velocity_y( vy + THROW_DOWN_SPEED );
  }

  set_colgroup_active(COLGROUP_MOVING);
  grabbed = false;
}

/* EOF */
