# A new window manager based off my other window manager aewm++ 
# Copyright (C) 2010-2014 Frank Hale <frankhale@gmail.com>
#
# aewm++ can be found here: http://code.google.com/p/aewmpp/
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 3
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Started: 28 January 2010 
# Date: 1 January 2014

CC = clang++
ADDITIONAL_CFLAGS = -ggdb -O2 -Wall -std=c++11 -Wc++11-extensions

prefix   = 
INCLUDES = -I$/usr/X11R6
LDPATH   = -L/usr/X11R6/lib
LIBS     = -lXext -lX11
HEADERS  = mini.hh 
OBJS     = mini.o 	

all: mini

mini: $(OBJS)
	$(CC) $(OBJS) $(LDPATH) $(LIBS) -o $@

$(OBJS): %.o: %.cc $(HEADERS)
	$(CC) $(CFLAGS) $(ADDITIONAL_CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

install: all
	mkdir -p $(DESTDIR)$(prefix)
	install -s mini $(DESTDIR)$(prefix)
	
clean:
	rm -f mini $(OBJS) core
