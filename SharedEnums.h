/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2012  <copyright holder> <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef SHAREDENUMS_H
#define SHAREDENUMS_H

namespace SCADAMessageTypes
{
  enum T : uint8_t {Command=1, Event=2,Alive=3};
}
namespace PLCMessageTypes
{
  enum T : uint8_t {StatusUpdate=1};
}
namespace SCADASourceTypes
{
  enum T : uint8_t {Internal=1,ITZ500=2, SW_Keller=3};
}
namespace PLCSourceTypes
{
  enum T : uint8_t {Jalousie=1,Rollo=2};
}
namespace Functions
{
  enum T : uint8_t {OnOff=1,Dim=2};
}
namespace PLCUnitProperties
{
  enum T  : uint8_t {OnOff=1,UpDown=2};
}

namespace CULMessageTypes
{
  enum T : uint8_t {StatusUpdate=1};
}

namespace CULSourceTypes
{
  enum T : uint8_t {Switch=1,Remote=2};
}

namespace CULUnitProperties
{
  enum T  : uint8_t {OnOff=1,Dim=2};
}

#endif // SHAREDENUMS_H
