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

#include "trigger/secretarea_trigger.hpp"

#include "object/tilemap.hpp"
#include "supertux/level.hpp"
#include "supertux/globals.hpp"
#include "supertux/object_factory.hpp"
#include "supertux/resources.hpp"
#include "supertux/sector.hpp"
#include "util/gettext.hpp"
#include "util/reader.hpp"
#include "util/writer.hpp"

static const float MESSAGE_TIME=3.5;

SecretAreaTrigger::SecretAreaTrigger(const Reader& reader) :
  message_timer(),
  message_displayed(),
  fade_tilemap()
{
  reader.get("x", bbox.p1.x);
  reader.get("y", bbox.p1.y);
  float w = 32, h = 32;
  reader.get("width", w);
  reader.get("height", h);
  bbox.set_size(w, h);
  reader.get("fade-tilemap", fade_tilemap);

  message_displayed = false;
}

SecretAreaTrigger::SecretAreaTrigger(const Rect& area, std::string fade_tilemap) :
  message_timer(),
  message_displayed(),
  fade_tilemap(fade_tilemap)
{
  bbox = area;
  message_displayed = false;
}

SecretAreaTrigger::~SecretAreaTrigger()
{
}

void
SecretAreaTrigger::draw(DrawingContext& context)
{
  if (message_timer.started()) {
    context.push_transform();
    context.set_translation(Vector(0, 0));
    Vector pos = Vector(0, SCREEN_HEIGHT/2 - Resources::normal_font->get_height()/2);
    context.draw_center_text(Resources::normal_font, _("You found a secret area!"), pos, LAYER_GUI, SecretAreaTrigger::text_color);
    context.pop_transform();
  }
  if (message_timer.check()) {
    remove_me();
  }
}

void
SecretAreaTrigger::event(Player& , EventType type)
{
  if(type == EVENT_TOUCH) {
    if (!message_displayed) {
      message_timer.start(MESSAGE_TIME);
      message_displayed = true;
      Sector::current()->get_level()->stats.secrets++;

      if (fade_tilemap != "") {
        // fade away tilemaps
        Sector& sector = *Sector::current();
        for(Sector::GameObjects::iterator i = sector.gameobjects.begin(); i != sector.gameobjects.end(); ++i) {
          TileMap* tm = dynamic_cast<TileMap*>(*i);
          if (!tm) continue;
          if (tm->get_name() != fade_tilemap) continue;
          tm->fade(0.0, 1.0);
        }
      }

    }
  }
}

IMPLEMENT_FACTORY(SecretAreaTrigger, "secretarea");

/* EOF */