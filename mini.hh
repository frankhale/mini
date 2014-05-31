// A new window manager based off my other window manager aewm++
// Copyright (C) 2010-2014 Frank Hale <frankhale@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 3
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Started: 28 January 2010
// Updated: 4 January 2014

#ifndef __MINI_H__
#define __MINI_H__

#define VERSION_STRING "Mini Window Manager | 3 January 2014 | http://github.com/frankhale | Frank Hale <frankhale@gmail.com>"

#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>
#include <X11/extensions/shape.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <list>
#include <string>
#include <string.h>
#include <iostream>

enum class JustifyMode { LEFT, CENTER, RIGHT };
enum class FocusMode { FOLLOW, SLOPPY, CLICK };
enum class Gravity { APPLY = 1, REMOVE = -1 };
enum class ResizeMode { PIXELS = 0, INCREMENTS = 1 };
enum class WindowPlacement { MOUSE = 0, RANDOM = 1 };

#define DEFAULT_FONT                "Fixed"
#define DEFAULT_FOREGROUND_COLOR    "#000000" // Window title font color
#define DEFAULT_BACKGROUND_COLOR    "#999999" // Window title background color
#define DEFAULT_FOCUS_COLOR         "#dddddd"
#define DEFAULT_BORDER_COLOR        "#000000"
#define FOCUSED_BORDER_COLOR        "#000000"
#define UNFOCUSED_BORDER_COLOR      "#888888"
#define FOCUSED_WINDOW_TITLE_COLOR  "#FFFFFF"

#define DEFAULT_CMD                 "xterm -ls -sb -bg black -fg white"
#define DEFAULT_BORDER_WIDTH        1
#define SPACE                       3
#define MINSIZE                     15
#define EDGE_SNAP                   true
#define SNAP                        5
#define TEXT_JUSTIFY                JustifyMode::RIGHT
#define WIRE_MOVE                   false
#define DEFAULT_FOCUS_MODEL         FocusMode::CLICK
#define DEFAULT_WINDOW_PLACEMENT    WindowPlacement::MOUSE
#define TRANSIENT_WINDOW_HEIGHT     8
#define ALT_KEY_COUNT               2

int handleXError(Display *dpy, XErrorEvent *e);

class WindowManager {
private:
  struct Client {
    Window window = None;
    Window frame = None;
    Window title = None;
    Window trans = None;

    bool has_focus = false;
    bool has_title = true;
    bool has_border = true;
    bool is_being_dragged = false;
    bool is_being_resized = false;
    bool do_drawoutline_once = false;
    bool is_shaded = false;
    bool is_maximized = false;
    bool is_visible = false;
    bool has_been_shaped = false;

    bool button_pressed = false;
    Time last_button1_time = 0;
    int ignore_unmap = 0;

    XSizeHints *size = NULL;
    int border_width = 1;
    int x = 0;4
    int y = 0;
    int width = 1;
    int height = 1;

    int old_x = 0;
    int old_y = 0;
    int old_width = 0;
    int old_height = 0;
    int pointer_x = 0;
    int pointer_y = 0;
    int old_cx = 0;
    int old_cy = 0;

    std::string name = "unknown";
    XCharStruct overall;
    int direction = 0;
    int ascent = 0;
    int descent = 0;
    int text_width = 0;
    int text_justify = 0;

    bool should_takefocus = false;
  };
  
  std::string command_line;
  std::list<Client*> client_list;
  std::list<Window> client_window_list;

  Client* focused_client = NULL;
  XFontStruct *font;

  GC invert_gc;
  GC string_gc;
  GC border_gc;
  GC unfocused_gc;
  GC focused_title_gc;

  XColor fg;
  XColor bg;
  XColor bd;
  XColor fc;
  XColor focused_border;
  XColor unfocused_border;

  Cursor move_curs;
  Cursor arrow_curs;

  Display *dpy;
  Window root;
  Window _button_proxy_win;

  bool random_window_placement;

  int screen = 0;
  int xres = 0;
  int yres = 0;
  int shape = 0;
  int shape_event = 0;
  FocusMode focus_model = DEFAULT_FOCUS_MODEL;

  static KeySym alt_keys[];

  Atom atom_wm_state;
  Atom atom_wm_change_state;
  Atom atom_wm_protos;
  Atom atom_wm_delete;
  Atom atom_wm_takefocus;

  /* Functions */

  void cleanup();
  void doEventLoop();
  void queryWindowTree();
  void quitNicely();
  void restart();

  static void sigHandler(int signal);

  void handleKeyPressEvent(XEvent *ev);
  void handleButtonPressEvent(XEvent *ev);
  void handleButtonReleaseEvent(XEvent *ev);
  void handleConfigureRequestEvent(XEvent *ev);
  void handleMotionNotifyEvent(XEvent *ev);
  void handleMapRequestEvent(XEvent *ev);
  void handleUnmapNotifyEvent(XEvent *ev);
  void handleDestroyNotifyEvent(XEvent *ev);
  void handleEnterNotifyEvent(XEvent *ev);
  void handleFocusInEvent(XEvent *ev);
  void handleFocusOutEvent(XEvent *ev);
  void handlePropertyNotifyEvent(XEvent *ev);
  void handleExposeEvent(XEvent *ev);
  void handleDefaultEvent(XEvent *ev);

  void handleClientButtonEvent(XButtonEvent *ev, Client* c);
  void handleClientConfigureRequest(XConfigureRequestEvent *ev, Client* c);
  void handleClientMapRequest(XMapRequestEvent *ev, Client* c);
  void handleClientUnmapEvent(XUnmapEvent *ev, Client* c);
  void handleClientDestroyEvent(XDestroyWindowEvent *ev, Client* c);
  void handleClientPropertyChange(XPropertyEvent *ev, Client* c);
  void handleClientEnterEvent(XCrossingEvent *ev, Client* c);
  void handleClientExposeEvent(XExposeEvent *ev, Client* c);
  void handleClientFocusInEvent(XFocusChangeEvent *ev, Client* c);
  void handleClientMotionNotifyEvent(XMotionEvent *ev, Client* c);
  void handleClientShapeChange(XShapeEvent *ev, Client* c);

  void addClient(Window w);
  void removeClient(Client* c);
  Client* findClient(Window w);
  void setXFocus(Client *c);
  void setClientFocus(Client* c, bool focus);
  void hideClient(Client* c);
  void unhideClient(Client* c);
  void shadeClient(Client* c);
  void maximizeClient(Client* c);
  void redrawClient(Client* c);
  void drawClientOutline(Client* c);
  int getClientIncsize(Client* c, int *x_ret, int *y_ret, ResizeMode mode);
  void initClientPosition(Client* c);
  void reparentClient(Client* c);
  int getClientTitleHeight(Client *c);
  void sendClientConfig(Client* c);
  void gravitateClient(Client* c, Gravity multiplier);
  void setClientShape(Client* c);
  void queryClientName(Client* c);
  void forkExec(std::string cmd);
  void unfocusAnyStrayClients();
  void focusPreviousWindowInStackingOrder();
  long getWMState(Window window);
  void setWMState(Window window, int state);
  void sendWMDelete(Window window);
  int sendXMessage(Window w, Atom a, long mask, long x);
  void setFocusModel(int new_fm);
  void grabKeys(Window w);
  void ungrabKeys(Window w);
  void getMousePosition(int *x, int *y);
  
public:
  WindowManager(int argc, char** argv);
};

#endif
