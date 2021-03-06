//  SuperTux
//  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_SUPERTUX_SUPERTUX_MENU_CONTRIB_MENU_HPP
#define HEADER_SUPERTUX_SUPERTUX_MENU_CONTRIB_MENU_HPP

#include "gui/menu.hpp"

class ContribWorldMenu;
class World;

class ContribMenu : public Menu
{
private:
  std::auto_ptr<ContribWorldMenu> m_contrib_world_menu;
  std::vector<World*> m_contrib_worlds;

public:
  ContribMenu();
  ~ContribMenu();
  
  void check_menu();

private:
  ContribMenu(const ContribMenu&);
  ContribMenu& operator=(const ContribMenu&);
};

#endif

/* EOF */
