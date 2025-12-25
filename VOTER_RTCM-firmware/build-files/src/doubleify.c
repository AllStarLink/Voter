/*
* VOTER Client System Firmware for VOTER board
*
* Copyright (C) 2011
* Jim Dixon, WB6NIL <jim@lambdatel.com>
*
* This file is part of the VOTER System Project 
*
*   VOTER System is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 2 of the License, or
*   (at your option) any later version.

*   Voter System is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this project.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "TCPIP Stack/TCPIP.h"

// Return a 32-bit float of a  big-endian byte order
// double pointed to by p
//
// The swine Trimble Thunderbolt encodes its lat/long/elev in
// this format (for some reason).
// In addition the swine C library that comes with the MPLAB C30
// compiler haS A "little" problem. It has an option to make double
// values 64 bit (go figure, of *course* I would declare something double
// if I wanted it to be 32 bits (not!). Setting this "feature" breaks the
// printf() call in the C library. So this is the only C source file to which
// that option is to be applied, and that is why it is in a separate file.
// POOP!

float doubleify(BYTE *p)
{
union {
        BYTE c[8];
        double d;
} x;
BYTE     i;

        for(i = 0;i < 8; i++) x.c[7 - i] = p[i];
        return (float)x.d;
}
