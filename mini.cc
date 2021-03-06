// A minimal window manager based off my other window manager aewm++
// 
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
// along with this program.  If not, see <http:b://www.gnu.org/licenses/>.
//
// Started: 28 January 2010
// Date: 22 February 2015

#include "mini.hh"

WindowManager *wm;
KeySym WindowManager::alt_keys[] = { XK_Delete, XK_End, XK_Home };

Config::Config() 
{
	auto *pw = getpwuid(getuid());
	string homedir = pw->pw_dir;
	string configPath = homedir + "/" + configFileName;

	ifstream ifs(configPath);

	if(ifs.good()) {
		json_object *config_json;

		string raw_json((istreambuf_iterator<char>(ifs)), (istreambuf_iterator<char>()));

		ifs.close();

		config_json = json_tokener_parse(raw_json.c_str());

		json_object *jo_font;
		json_object *jo_foreground_color;
		json_object *jo_background_color;
		json_object *jo_focused_color;
		json_object *jo_focused_border_color;
		json_object *jo_unfocused_border_color;
		json_object *jo_right_click_cmd;
    json_object *jo_alternate_cmd;
		json_object *jo_border_width;
		json_object *jo_space;
		json_object *jo_edge_snap;
		json_object *jo_snap;
		json_object *jo_text_justify;
		json_object *jo_wire_move;
		json_object *jo_transient_window_height;

		json_object_object_get_ex(config_json, "font", &jo_font);
    json_object_object_get_ex(config_json, "windowTitleTextColor", &jo_foreground_color);
		json_object_object_get_ex(config_json, "windowTitleFocusedColor", &jo_background_color);
    json_object_object_get_ex(config_json, "windowTitleUnfocusedColor", &jo_focused_color);
		json_object_object_get_ex(config_json, "focusedBorderColor", &jo_focused_border_color);
		json_object_object_get_ex(config_json, "unfocusedBorderColor", &jo_unfocused_border_color);
		json_object_object_get_ex(config_json, "rightClickCmd", &jo_right_click_cmd);
    json_object_object_get_ex(config_json, "alternateCmd", &jo_alternate_cmd);
		json_object_object_get_ex(config_json, "borderWidth", &jo_border_width);
		json_object_object_get_ex(config_json, "space", &jo_space);
		json_object_object_get_ex(config_json, "edgeSnap", &jo_edge_snap);
		json_object_object_get_ex(config_json, "snap", &jo_snap);
		json_object_object_get_ex(config_json, "textJustify", &jo_text_justify);
		json_object_object_get_ex(config_json, "wireMove", &jo_wire_move);
		json_object_object_get_ex(config_json, "transientWindowHeight", &jo_transient_window_height);

		font = json_object_get_string(jo_font);

		if(jo_foreground_color != NULL)
      foregroundColor = json_object_get_string(jo_foreground_color);
		
    if(jo_background_color != NULL)
      backgroundColor = json_object_get_string(jo_background_color);
	
  	if(jo_focused_color != NULL)
      focusedColor = json_object_get_string(jo_focused_color);

    if(jo_focused_border_color != NULL)
      focusedBorderColor = json_object_get_string(jo_focused_border_color);

    if(jo_unfocused_border_color != NULL)
      unfocusedBorderColor = json_object_get_string(jo_unfocused_border_color);

    if(jo_right_click_cmd != NULL)
      rightClickCmd = json_object_get_string(jo_right_click_cmd);
    
    if(jo_alternate_cmd != NULL)
      alternateCmd = json_object_get_string(jo_alternate_cmd);
		
    if(jo_border_width != NULL)
      borderWidth = json_object_get_int(jo_border_width);
		
    if(jo_space != NULL)
      space = json_object_get_int(jo_space);
		
    if(jo_edge_snap != NULL)
      edgeSnap = json_object_get_boolean(jo_edge_snap);
		
    if(jo_snap != NULL)
      snap = json_object_get_int(jo_snap);
	
    if(jo_wire_move != NULL)	
      wireMove = json_object_get_boolean(jo_wire_move);
	
  	if(jo_transient_window_height != NULL)
      transientWindowHeight = json_object_get_int(jo_transient_window_height);

  	string textJustifyString = json_object_get_string(jo_text_justify);

		if(borderWidth == 0) borderWidth = 1;
		if(space == 0) space = 3;
		if(snap == 0) snap = 5;
		if(transientWindowHeight == 0) transientWindowHeight = 8;

		if(textJustifyString == "Left") {
			textJustify = JustifyMode::LEFT;
		} else if (textJustifyString == "Center") {
			textJustify = JustifyMode::CENTER;
		} else if (textJustifyString == "Right") {
			textJustify = JustifyMode::RIGHT;
		} else {
			textJustify = JustifyMode::LEFT;
		}

		// focused window title color
    foregroundColor = getColor(foregroundColor, "#000000");
    // focused window background color
		backgroundColor = getColor(backgroundColor, "#999999");
    // unfocused window background color
		focusedColor = getColor(focusedColor, "#dddddd");
    focusedBorderColor = getColor(focusedBorderColor, "#000000");		
    unfocusedBorderColor = getColor(unfocusedBorderColor, "#888888");
	} else {
		initDefaults();
	}
}

string Config::getColor(string input, string defaultColor) 
{
	auto colorRegex = regex("^#(?:[0-9a-fA-F]{3}){1,2}$");

	if(regex_match(input, colorRegex)) 
		return input;
	else
		return defaultColor;
}

void Config::initDefaults() 
{
	font = "Fixed";
	foregroundColor = "#000000";
	backgroundColor = "#999999";
	focusedColor = "#dddddd";
	focusedBorderColor = "#000000";
	unfocusedBorderColor = "#888888";
	rightClickCmd = "xterm -ls -sb -bg black -fg white";
	borderWidth = 1;
	space = 3;
	edgeSnap = true;
	snap = 5;
	textJustify = JustifyMode::RIGHT;
	wireMove = false;
	transientWindowHeight = 8;
}

void WindowManager::grabKeys(Window w)
{
	for(int i=0; i<ALT_KEY_COUNT; i++)
		XGrabKey(dpy, XkbKeycodeToKeysym(dpy, alt_keys[i], 0, 1), (Mod1Mask|ControlMask), w,True,GrabModeAsync,GrabModeAsync);
}

void WindowManager::ungrabKeys(Window w)
{
	for(int i=0; i<ALT_KEY_COUNT; i++)
		XUngrabKey(dpy, XkbKeycodeToKeysym(dpy,alt_keys[i], 0, 1), (Mod1Mask|ControlMask),w);
}

WindowManager::WindowManager(int argc, char** argv)
{
	int dummy;     // not used but needed to satisfy XShapeQueryExtension call
	XColor dummyc; // not used but needed to satisfy XAllocNamedColor call

	XGCValues gv;
	XSetWindowAttributes sattr;

	wm = this;

	for (int i = 0; i < argc; i++)
		command_line = command_line + argv[i] + " ";

	signal(SIGINT, sigHandler);
	signal(SIGTERM, sigHandler);
	signal(SIGHUP, sigHandler);

	dpy = XOpenDisplay(getenv("DISPLAY"));

	if(dpy)
	{
		font = XLoadQueryFont(dpy, config.font.c_str());

		if (!font)
			font = XLoadQueryFont(dpy, "Fixed");
	}
	else
	{
		std::cerr << "can't open display! check your DISPLAY variable." << std::endl;
		exit(-1);
	}

	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	xres = DisplayWidth(dpy, screen);
	yres = DisplayHeight(dpy, screen);

	XSetErrorHandler(handleXError);

	atom_wm_state       = XInternAtom(dpy, "WM_STATE", False);
	atom_wm_change_state  = XInternAtom(dpy, "WM_CHANGE_STATE", False);
	atom_wm_protos       = XInternAtom(dpy, "WM_PROTOCOLS", False);
	atom_wm_delete       = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	atom_wm_takefocus    = XInternAtom(dpy, "WM_TAKE_FOCUS", False);

	XAllocNamedColor(dpy, DefaultColormap(dpy, screen), config.foregroundColor.c_str(), &fg, &dummyc);
	XAllocNamedColor(dpy, DefaultColormap(dpy, screen), config.backgroundColor.c_str(), &bg, &dummyc);
	XAllocNamedColor(dpy, DefaultColormap(dpy, screen), config.focusedColor.c_str(), &fc, &dummyc);
	XAllocNamedColor(dpy, DefaultColormap(dpy, screen), config.focusedBorderColor.c_str(), &focused_border, &dummyc);
	XAllocNamedColor(dpy, DefaultColormap(dpy, screen), config.unfocusedBorderColor.c_str(), &unfocused_border, &dummyc);

	shape = XShapeQueryExtension(dpy, &shape_event, &dummy);
	move_curs = XCreateFontCursor(dpy, XC_fleur);
	arrow_curs = XCreateFontCursor(dpy, XC_left_ptr);

	XDefineCursor(dpy, root, arrow_curs);

	gv.function = GXcopy;
	gv.foreground = fg.pixel;
	gv.font = font->fid;
	string_gc = XCreateGC(dpy, root, GCFunction|GCForeground|GCFont, &gv);

	gv.foreground = unfocused_border.pixel;
	unfocused_gc = XCreateGC(dpy, root, GCForeground|GCFont, &gv);

	gv.foreground = fg.pixel;
	focused_title_gc = XCreateGC(dpy, root, GCForeground|GCFont, &gv);

	gv.foreground = fg.pixel;
	gv.line_width = config.borderWidth;
	border_gc = XCreateGC(dpy, root, GCFunction|GCForeground|GCLineWidth, &gv);

	gv.foreground = fg.pixel;
	gv.function = GXinvert;
	gv.subwindow_mode = IncludeInferiors;
	invert_gc = XCreateGC(dpy, root, GCForeground|GCFunction|GCSubwindowMode|GCLineWidth|GCFont, &gv);

	sattr.event_mask = SubstructureRedirectMask |
	                   SubstructureNotifyMask   |
	                   ButtonPressMask          |
	                   ButtonReleaseMask        |
	                   FocusChangeMask          |
	                   PropertyChangeMask       |
	                   ButtonMotionMask         ;

	XChangeWindowAttributes(dpy, root, CWEventMask, &sattr);

	grabKeys(root);

	queryWindowTree();
	doEventLoop();
}

void WindowManager::queryWindowTree()
{
	unsigned int nwins, i;
	Window dummyw1, dummyw2, *wins;
	XWindowAttributes attr;

	XQueryTree(dpy, root, &dummyw1, &dummyw2, &wins, &nwins);

	for(i = 0; i < nwins; i++)
	{
		XGetWindowAttributes(dpy, wins[i], &attr);

		if (!attr.override_redirect && attr.map_state == IsViewable)
			addClient(wins[i]);
	}

	XFree(wins);
}

void WindowManager::addClient(Window w)
{
	XWindowAttributes attr;
	XWMHints *hints;

	client_window_list.push_back(w);

	auto c = std::make_shared<Client>();

	client_list.push_back(c);
	XGrabServer(dpy);

	c->window = w;

	queryClientName(c);

	XGetTransientForHint(dpy, c->window, &c->trans);
	XGetWindowAttributes(dpy, c->window, &attr);

	if (attr.map_state == IsViewable) c->ignore_unmap++;

	c->x = attr.x;
	c->y = attr.y;
	c->width = attr.width;
	c->height = attr.height;
	c->border_width = attr.border_width;
	c->size = XAllocSizeHints();

	long dummy;
	XGetWMNormalHints(dpy, c->window, c->size, &dummy);

	c->old_x = c->x;
	c->old_y = c->y;
	c->old_width = c->width;
	c->old_height = c->height;

	initClientPosition(c);

	if ((hints = XGetWMHints(dpy, w)))
	{
		if(hints->flags & InputHint)
			c->should_takefocus = !hints->input;

		if (hints->flags & StateHint)
			setWMState(c->window, hints->initial_state);
		else
			setWMState(c->window, NormalState);

		XFree(hints);
	}

	gravitateClient(c, Gravity::APPLY);
	reparentClient(c);
	unhideClient(c);
	setXFocus(c);

	XSync(dpy, False);
	XUngrabServer(dpy);
}

void WindowManager::removeClient(std::shared_ptr<Client> c)
{
	XGrabServer(dpy);

	if(c->trans)
		XUnmapWindow(dpy, c->window);

	XUngrabButton(dpy, AnyButton, AnyModifier, c->frame);

	gravitateClient(c, Gravity::REMOVE);

	XReparentWindow(dpy, c->window, root, c->x, c->y);
	XDestroyWindow(dpy, c->title);
	XDestroyWindow(dpy, c->frame);

	if (c->size)
		XFree(c->size);

	XSync(dpy, False);
	XUngrabServer(dpy);

	client_window_list.remove(c->window);
	client_list.remove(c);
}

std::shared_ptr<WindowManager::Client> WindowManager::findClient(Window w)
{
	if(client_list.size())
	{
		for(auto c : client_list)
		{
			if(w == c->title ||
			    w == c->frame ||
			    w == c->window)
				return c;
		}
	}

	return nullptr;
}

void WindowManager::doEventLoop()
{
	XEvent ev;

	for (;;)
	{
		XNextEvent(dpy, &ev);

		switch (ev.type)
		{
		case KeyPress:
			handleKeyPressEvent(&ev);
			break;

		case ButtonPress:
			handleButtonPressEvent(&ev);
			break;

		case ButtonRelease:
			handleButtonReleaseEvent(&ev);
			break;

		case ConfigureRequest:
			handleConfigureRequestEvent(&ev);
			break;

		case MotionNotify:
			handleMotionNotifyEvent(&ev);
			break;

		case MapRequest:
			handleMapRequestEvent(&ev);
			break;

		case UnmapNotify:
			handleUnmapNotifyEvent(&ev);
			break;

		case DestroyNotify:
			handleDestroyNotifyEvent(&ev);
			break;

		case FocusIn:
			handleFocusInEvent(&ev);
			break;

		case FocusOut:
			handleFocusOutEvent(&ev);
			break;

		case PropertyNotify:
			handlePropertyNotifyEvent(&ev);
			break;

		case Expose:
			handleExposeEvent(&ev);
			break;

		default:
			handleDefaultEvent(&ev);
			break;
		}
	}
}

void WindowManager::handleKeyPressEvent(XEvent *ev)
{
	auto ks = XkbKeycodeToKeysym(dpy, ev->xkey.keycode, 0, 1);

	if (ks == NoSymbol)
		return;

	switch(ks)
	{
	case XK_Delete:
		std::cerr << "Window manager is restarting..." << std::endl;
		restart();
		break;
  case XK_End:
		std::cerr << "Window manager is quitting..." << std::endl;
		quitNicely();
		break;
  case XK_Home:
 		cout << "Taking Screenshot..." << endl;
    if(fork() == 0) {
      if(config.alternateCmd.length() > 0) {
        auto t = time(nullptr);
        auto tm = *localtime(&t);
        stringstream buffer;
        buffer << put_time(&tm, "%d%b%Y@%H%M%S");
        string filename = "mini-" + buffer.str() + ".png";
        execlp(config.alternateCmd.c_str(), config.alternateCmd.c_str(), filename.c_str(), NULL);
      }
    }   
    break;
	}
}

void WindowManager::handleButtonPressEvent(XEvent *ev)
{
	auto c = findClient(ev->xbutton.window);

	if(c && c->has_title)
	{
		if(ev->xbutton.button == Button1  &&
		    ev->xbutton.type == ButtonPress  &&
		    ev->xbutton.state == Mod1Mask    &&
		    c->frame == ev->xbutton.window)
		{
			if(!(XGrabPointer(dpy, c->frame, False, PointerMotionMask|ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, move_curs, CurrentTime) == GrabSuccess))
				return;
		}
	}

	if(c)
	{
		if(c != focused_client)
		{
			setXFocus(c);
			focused_client = c;
		}

		handleClientButtonEvent(&ev->xbutton, c);
	}
}

void WindowManager::handleButtonReleaseEvent(XEvent *ev)
{
	auto c = findClient(ev->xbutton.window);

	if(c)
	{
		XUngrabPointer(dpy, CurrentTime);
		handleClientButtonEvent(&ev->xbutton, c);
	}
	else if (ev->xbutton.window==root && ev->xbutton.button==Button3)
	{
		if(fork() == 0) {
      if(config.rightClickCmd.length() > 0)
        execlp("/bin/sh", "sh", "-c", config.rightClickCmd.c_str(), NULL);
    }
	}
}

void WindowManager::handleConfigureRequestEvent(XEvent *ev)
{
	auto c = findClient(ev->xconfigurerequest.window);

	if(c)
	{
		handleClientConfigureRequest(&ev->xconfigurerequest, c);
	}
	else
	{
		XWindowChanges wc;

		wc.x = ev->xconfigurerequest.x;
		wc.y = ev->xconfigurerequest.y;
		wc.width = ev->xconfigurerequest.width;
		wc.height = ev->xconfigurerequest.height;
		wc.sibling = ev->xconfigurerequest.above;
		wc.stack_mode = ev->xconfigurerequest.detail;
		XConfigureWindow(dpy, ev->xconfigurerequest.window, ev->xconfigurerequest.value_mask, &wc);
	}
}

void WindowManager::handleMotionNotifyEvent(XEvent *ev)
{
	auto c = findClient(ev->xmotion.window);

	if(c)
		handleClientMotionNotifyEvent(&ev->xmotion, c);
}

void WindowManager::handleMapRequestEvent(XEvent *ev)
{
	auto c = findClient(ev->xmaprequest.window);

	if(c)
		handleClientMapRequest(&ev->xmaprequest, c);
	else
		addClient(ev->xmaprequest.window);
}

void WindowManager::handleUnmapNotifyEvent(XEvent *ev)
{
	auto c = findClient(ev->xunmap.window);

	if(c)
	{
		handleClientUnmapEvent(&ev->xunmap, c);
		focused_client = nullptr;
	}
}

void WindowManager::handleDestroyNotifyEvent(XEvent *ev)
{
	auto c = findClient(ev->xdestroywindow.window);

	if(c)
	{
		handleClientDestroyEvent(&ev->xdestroywindow, c);
		focused_client = nullptr;
	}
}

void WindowManager::handleFocusInEvent(XEvent *ev)
{
	if((ev->xfocus.mode == NotifyGrab) || (ev->xfocus.mode == NotifyUngrab))
		return;

	for(auto cw : client_window_list)
	{
		if(ev->xfocus.window == cw)
		{
			auto c = findClient(cw);

			if(c)
			{
				unfocusAnyStrayClients();
				handleClientFocusInEvent(&ev->xfocus, c);
				focused_client = c;
			}
		}
	}
}

void WindowManager::handleFocusOutEvent(XEvent *ev)
{
	for(auto cw : client_window_list)
	{
		if(ev->xfocus.window == cw)
		{
			auto c = findClient(cw);

			if(c)
			{
				focused_client = nullptr;
				return;
			}
		}
	}

	focusPreviousWindowInStackingOrder();
}

void WindowManager::handlePropertyNotifyEvent(XEvent *ev)
{
	auto c = findClient(ev->xproperty.window);

	if(c)
		handleClientPropertyChange(&ev->xproperty, c);
}

void WindowManager::handleExposeEvent(XEvent *ev)
{
	auto c = findClient(ev->xexpose.window);

	if(c)
		handleClientExposeEvent(&ev->xexpose, c);
}

void WindowManager::handleDefaultEvent(XEvent *ev)
{
	auto c = findClient(ev->xany.window);

	if(c && shape && ev->type == shape_event)
		handleClientShapeChange((XShapeEvent *)ev, c);
}

void WindowManager::restart()
{
	cleanup();
	execl("/bin/sh", "sh", "-c", command_line.c_str(), nullptr);
}

void WindowManager::quitNicely()
{
	cleanup();
	exit(0);
}

void WindowManager::cleanup()
{
	std::cerr << "Window manager is cleaning up..." << std::endl;

	unsigned int nwins, i;
	Window dummyw1, dummyw2, *wins;
	std::shared_ptr<Client> c;

	XQueryTree(dpy, root, &dummyw1, &dummyw2, &wins, &nwins);

	for (i = 0; i < nwins; i++)
	{
		c = findClient(wins[i]);

		if(c)
		{
			XMapWindow(dpy, c->window);
			removeClient(c);
		}
	}

	XFree(wins);
	XFreeFont(dpy, font);
	XFreeCursor(dpy, move_curs);
	XFreeCursor(dpy, arrow_curs);
	XFreeGC(dpy, invert_gc);
	XFreeGC(dpy, border_gc);
	XFreeGC(dpy, string_gc);
	XFreeGC(dpy, unfocused_gc);
	XFreeGC(dpy, focused_title_gc);
	XInstallColormap(dpy, DefaultColormap(dpy, screen));
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	ungrabKeys(root);

	XCloseDisplay(dpy);
}

void WindowManager::unfocusAnyStrayClients()
{
	for(auto c : client_list)
		setClientFocus(c, false);
}

void WindowManager::getMousePosition(int *x, int *y)
{
	Window mouse_root, mouse_win;
	int win_x, win_y;
	unsigned int mask;

	XQueryPointer(dpy, root, &mouse_root, &mouse_win, x, y, &win_x, &win_y, &mask);
}

long WindowManager::getWMState(Window window)
{
	Atom real_type;
	int real_format;
	unsigned long items_read, items_left;
	unsigned char *data, state = WithdrawnState;

	if (XGetWindowProperty(dpy, window, atom_wm_state, 0L, 2L, False, atom_wm_state, &real_type, &real_format, &items_read, &items_left, &data) == Success && items_read)
	{
		state = *data;
		XFree(data);
	}

	return state;
}

void WindowManager::setWMState(Window window, int state)
{
	CARD32 data[2];

	data[0] = state;
	data[1] = None;

	XChangeProperty(dpy, window, atom_wm_state, atom_wm_state, 32, PropModeReplace, (unsigned char *)data, 2);
}

void WindowManager::sendWMDelete(Window window)
{
	int i, n;
	bool found = false;
	Atom *protocols;

	if (XGetWMProtocols(dpy, window, &protocols, &n))
	{
		for (i=0; i<n; i++)
		{
			if (protocols[i] == atom_wm_delete) {
				found = true;
				break;
			}
		}

		XFree(protocols);
	}

	if (found)
		sendXMessage(window, atom_wm_protos, RevertToNone, atom_wm_delete);
	else
		XKillClient(dpy, window);
}

int WindowManager::sendXMessage(Window w, Atom a, long mask, long x)
{
	XEvent e;

	e.type = ClientMessage;
	e.xclient.window = w;
	e.xclient.message_type = a;
	e.xclient.format = 32;
	e.xclient.data.l[0] = x;
	e.xclient.data.l[1] = CurrentTime;

	return XSendEvent(dpy, w, False, mask, &e);
}

void WindowManager::handleClientButtonEvent(XButtonEvent *ev, std::shared_ptr<Client> c)
{
	auto theight = getClientTitleHeight(c);
	auto in_box = (ev->x >= c->width - theight) && (ev->y <= theight);

	c->old_cx = c->x;
	c->old_cy = c->y;
	c->pointer_x = ev->x_root;
	c->pointer_y = ev->y_root;

	XAllowEvents(dpy, ReplayPointer, CurrentTime);

	switch (ev->button)
	{
	case Button1:
	{
		if (ev->type == ButtonPress)
		{
			XRaiseWindow(dpy, c->frame);

			if (ev->window == c->title && in_box)
				sendWMDelete(c->window);
		}

		if (ev->type == ButtonRelease)
		{
			if(c->is_being_dragged)
			{
				c->is_being_dragged=false;
				c->do_drawoutline_once=false;
				drawClientOutline(c);
				XMoveWindow(dpy, c->frame, c->x, c->y-theight);
				sendClientConfig(c);
				XUngrabServer(dpy);
				XSync(dpy, False);
			}

			if(ev->time-c->last_button1_time < 250)
			{
				maximizeClient(c);
				c->last_button1_time = 0;
				return;
			}
			else
				c->last_button1_time = ev->time;
		}
	}
	break;

	case Button2:
	{
		if(ev->type == ButtonPress &&
		    ev->window == c->title &&
		    in_box)
		{
			if(c->is_shaded)
				shadeClient(c);

			XRaiseWindow(dpy, c->frame);
		}

		if(ev->type == ButtonRelease)
			shadeClient(c);
	}
	break;

	case Button3:
	{
		if(ev->type == ButtonRelease &&
		    ev->window == c->title &&
		    c->is_being_resized)
		{
			drawClientOutline(c);
			c->do_drawoutline_once = false;
			c->is_being_resized = false;

			XResizeWindow(dpy, c->frame, c->width, c->height + theight);
			XResizeWindow(dpy, c->title, c->width, theight);
			XResizeWindow(dpy, c->window, c->width, c->height);

			sendClientConfig(c);

			XUngrabServer(dpy);
			XSync(dpy, False);

			return;
		}
		else if (ev->type == ButtonRelease && ev->window == c->title)
		{
			if(ev->time-c->last_button1_time < 250)
			{
				shadeClient(c);
				c->last_button1_time = 0;
				return;
			}
			else
				c->last_button1_time = ev->time;
		}
	}
	break;
	}
}

void WindowManager::handleClientConfigureRequest(XConfigureRequestEvent *ev, std::shared_ptr<Client> c)
{
	auto theight = getClientTitleHeight(c);

	gravitateClient(c, Gravity::REMOVE);

	if (ev->value_mask & CWX) c->x = ev->x;
	if (ev->value_mask & CWY) c->y = ev->y;
	if (ev->value_mask & CWWidth) c->width = ev->width;
	if (ev->value_mask & CWHeight) c->height = ev->height;

	gravitateClient(c, Gravity::APPLY);

	if(! c->is_shaded)
	{
		XMoveResizeWindow(dpy, c->frame, c->x, c->y-theight, c->width, c->height+theight);
		XResizeWindow(dpy, c->title, c->width, theight);
		XMoveResizeWindow(dpy, c->window, 0, theight, c->width, c->height);
	}

	if((c->x + c->width > xres)   ||
	    (c->height + theight > yres) ||
	    (c->x > xres) ||
	    (c->y > yres) ||
	    (c->x < 0)   ||
	    (c->y < 0))
		initClientPosition(c);

	if (ev->value_mask & (CWWidth|CWHeight))
		setClientShape(c);
}

void WindowManager::handleClientMapRequest(XMapRequestEvent *ev, std::shared_ptr<Client> c)
{
	unhideClient(c);
}

void WindowManager::handleClientUnmapEvent(XUnmapEvent *ev, std::shared_ptr<Client> c)
{
	if(! c->ignore_unmap)
		removeClient(c);
}

void WindowManager::handleClientDestroyEvent(XDestroyWindowEvent *ev, std::shared_ptr<Client> c)
{
	if(c)
		removeClient(c);
}

void WindowManager::handleClientPropertyChange(XPropertyEvent *ev, std::shared_ptr<Client> c)
{
	switch (ev->atom)
	{
	case XA_WM_NAME:
		queryClientName(c);
		XClearWindow(dpy, c->title);
		redrawClient(c);
		break;

	case XA_WM_NORMAL_HINTS:
		long dummy;
		XGetWMNormalHints(dpy, c->window, c->size, &dummy);
		break;
	}
}

void WindowManager::handleClientExposeEvent(XExposeEvent *ev, std::shared_ptr<Client> c)
{
	if(ev->count == 0)
		redrawClient(c);
}

void WindowManager::handleClientFocusInEvent(XFocusChangeEvent *ev, std::shared_ptr<Client> c)
{
	setClientFocus(c, true);
}

void WindowManager::handleClientMotionNotifyEvent(XMotionEvent *ev, std::shared_ptr<Client> c)
{
	auto nx=0, ny=0;
	auto theight = getClientTitleHeight(c);

	if((ev->state & Button1Mask) && (focused_client == c))
	{
		if(! c->do_drawoutline_once && config.wireMove)
		{
			XGrabServer(dpy);
			drawClientOutline(c);
			c->do_drawoutline_once = true;
			c->is_being_dragged = true;
		}

		if(config.wireMove)
			drawClientOutline(c);

		nx = c->old_cx + (ev->x_root - c->pointer_x);
		ny = c->old_cy + (ev->y_root - c->pointer_y);

		if(config.edgeSnap)
		{
			// Move beyond edges of screen
			if(nx == xres - c->width)
				nx = xres - c->width + 1;
			else if(nx == 0)
				nx = -1;

			if(ny == yres - config.snap)
				ny = yres - config.snap - 1;
			else if(ny == theight)
				ny = theight - 1;

			// Snap to edges of screen
			if( (nx + c->width >= xres - config.snap) && (nx + c->width <= xres) )
				nx = xres - c->width;
			else if((nx <= config.snap) && (nx >= 0))
				nx = 0;

			if(c->is_shaded)
			{
				if( (ny  >= yres - config.snap) && (ny  <= yres))
					ny = yres;
				else if((ny - theight <= config.snap) && (ny - theight >= 0))
					ny = theight;
			}
			else
			{
				if((ny + c->height >= yres - config.snap) && (ny + c->height <= yres))
					ny = yres - c->height;
				else if((ny - theight <= config.snap) && (ny - theight >= 0))
					ny = theight;
			}
		}

		c->x=nx;
		c->y=ny;

		if(!config.wireMove)
		{
			XMoveWindow(dpy, c->frame, nx, ny-theight);
			sendClientConfig(c);
		}
		else
			drawClientOutline(c);
	}
	else if(ev->state & Button3Mask)
	{
		if(! c->is_being_resized)
		{
			auto in_box = (ev->x >= c->width - theight) && (ev->y <= theight);

			if(! in_box)
				return;
		}

		if(! c->do_drawoutline_once)
		{
			c->is_being_resized = true;
			c->do_drawoutline_once = true;

			XGrabServer(dpy);
			drawClientOutline(c);
			XWarpPointer(dpy, None, c->frame, 0, 0, 0, 0, c->width, c->height+theight);
		}
		else
		{
			if((ev->x > 50) && (ev->y > 50))
			{
				drawClientOutline(c);

				c->width =  ev->x;
				c->height = ev->y - theight;

				getClientIncsize(c, &c->width, &c->height, ResizeMode::PIXELS);

				if (c->size->flags & PMinSize)
				{
					if (c->width < c->size->min_width)
						c->width = c->size->min_width;
					if (c->height < c->size->min_height)
						c->height = c->size->min_height;

					if(c->width < 100)
						c->width = 100;
					if(c->height < 50)
						c->height = 50;
				}

				if (c->size->flags & PMaxSize)
				{
					if (c->width > c->size->max_width)
						c->width = c->size->max_width;
					if (c->height > c->size->max_height)
						c->height = c->size->max_height;

					if(c->width>xres)
						c->width = xres;
					if(c->height>yres)
						c->height = yres;
				}

				drawClientOutline(c);
			}
		}
	}
}

void WindowManager::handleClientShapeChange(XShapeEvent *ev, std::shared_ptr<Client> c)
{
	setClientShape(c);
}

void WindowManager::setClientFocus(std::shared_ptr<Client> c, bool focus)
{
	c->has_focus=focus;

	if (c->has_title)
	{
		if(c->has_focus)
		{
			XSetWindowBackground(dpy, c->title, bg.pixel);
			XSetWindowBorder(dpy, c->frame, focused_border.pixel);
		}
		else
		{
			XSetWindowBackground(dpy, c->title, fc.pixel);
			XSetWindowBorder(dpy, c->frame, unfocused_border.pixel);
		}

		XClearWindow(dpy, c->title);
		redrawClient(c);
	}
}

void WindowManager::setXFocus(std::shared_ptr<Client> c)
{
	if(c->should_takefocus)
		sendXMessage(c->window, atom_wm_protos, 0L, atom_wm_takefocus);
	else
		XSetInputFocus(dpy, c->window, RevertToPointerRoot, CurrentTime);
}

void WindowManager::hideClient(std::shared_ptr<Client> c)
{
	if (!c->ignore_unmap)
		c->ignore_unmap++;

	if(c->has_focus)
		setClientFocus(c, false);

	XUnmapSubwindows(dpy, c->frame);
	XUnmapWindow(dpy, c->frame);

	setWMState(c->window, WithdrawnState);

	c->is_visible = false;
}

void WindowManager::unhideClient(std::shared_ptr<Client> c)
{
	XMapSubwindows(dpy, c->frame);
	XMapRaised(dpy, c->frame);

	setWMState(c->window, NormalState);
	setXFocus(c);

	c->is_visible = true;
}

void WindowManager::shadeClient(std::shared_ptr<Client> c)
{
	auto theight = getClientTitleHeight(c);

	XRaiseWindow(dpy, c->frame);

	if(! c->is_shaded)
	{
		XResizeWindow(dpy, c->frame, c->width, theight - 1);
		c->is_shaded = true;
	}
	else
	{
		XResizeWindow(dpy, c->frame, c->width, c->height + theight);
		c->is_shaded = false;
	}
}

void WindowManager::maximizeClient(std::shared_ptr<Client> c)
{
	auto theight = getClientTitleHeight(c);

	if(c->trans)
		return;

	if(c->is_shaded)
	{
		shadeClient(c);
		return;
	}

	if(! c->is_maximized)
	{
		c->old_x = c->x;
		c->old_y = c->y;
		c->old_width = c->width;
		c->old_height = c->height;

		if (c->size->flags & PMaxSize)
		{
			c->width = c->size->max_width;
			c->height = c->size->max_height;

			XMoveResizeWindow(dpy, c->frame, c->x, c->y-theight, c->width, c->height+theight);
		}
		else
		{
			c->x = 0;
			c->y = 0;
			c->width = xres - 2;
			c->height = yres - 2;

			XMoveResizeWindow(dpy, c->frame, c->x, c->y, c->width, c->height);

			c->y = theight;
			c->height -= theight;
		}

		c->is_maximized = true;
	}
	else
	{
		c->x = c->old_x;
		c->y = c->old_y;
		c->width = c->old_width;
		c->height = c->old_height;

		XMoveResizeWindow(dpy, c->frame, c->old_x, c->old_y - theight, c->old_width, c->old_height + theight);

		c->is_maximized = false;

		if(c->is_shaded)
			c->is_shaded = false;
	}

	XResizeWindow(dpy, c->title, c->width, theight);
	XResizeWindow(dpy, c->window, c->width, c->height);

	sendClientConfig(c);
}

void WindowManager::redrawClient(std::shared_ptr<Client> c)
{
	if (!c->has_title)
		return;

	GC gc;

	auto BW = (c->has_border ? config.borderWidth : 0);
	auto theight = getClientTitleHeight(c);

	if(c->has_focus)
		gc = border_gc;
	else
		gc = unfocused_gc;

	XDrawLine(dpy, c->title, gc, 0, theight - BW + BW/2, c->width, theight - BW + BW/2);
	XDrawLine(dpy, c->title, gc, c->width - theight+ BW/2, 0, c->width - theight+ BW/2, theight);

	if(c->has_focus)
		gc = focused_title_gc;

	if (!c->trans && (c->name.length() > 0))
	{
		switch(config.textJustify)
		{
		case JustifyMode::LEFT:
			c->text_justify = config.space;
			break;

		case JustifyMode::CENTER:
			c->text_justify = ((c->width / 2) - (c->text_width / 2));
			break;

		case JustifyMode::RIGHT:
			c->text_justify = c->width - c->text_width - 25;
			break;
		}

		XDrawString(dpy, c->title, gc, c->text_justify, font->ascent + 1, c->name.c_str(), c->name.length());
	}
}

void WindowManager::drawClientOutline(std::shared_ptr<Client> c)
{
	auto theight = getClientTitleHeight(c);
	auto BW = (c->has_border ? config.borderWidth : 0);

	if(! c->is_shaded)
	{
		XDrawRectangle(dpy, root, invert_gc,
		               c->x + BW/2, c->y - theight + BW/2,
		               c->width + BW, c->height + theight + BW);

		XDrawRectangle(dpy, root, invert_gc,
		               c->x + BW/2 + 4, c->y - theight + BW/2 + 4,
		               c->width + BW - 8, c->height + theight + BW - 8);
	}
	else
		XDrawRectangle(dpy, root, invert_gc, c->x + BW/2, c->y - theight + BW/2, c->width + BW, theight + BW);
}

int  WindowManager::getClientIncsize(std::shared_ptr<Client> c, int *x_ret, int *y_ret, ResizeMode mode)
{
	int basex, basey;

	if (c->size->flags & PResizeInc)
	{
		basex = (c->size->flags & PBaseSize) ? c->size->base_width :
		        (c->size->flags & PMinSize) ? c->size->min_width : 0;

		basey = (c->size->flags & PBaseSize) ? c->size->base_height :
		        (c->size->flags & PMinSize) ? c->size->min_height : 0;

		if (mode == ResizeMode::PIXELS)
		{
			*x_ret = c->width - ((c->width - basex) % c->size->width_inc);
			*y_ret = c->height - ((c->height - basey) % c->size->height_inc);
		}
		else // INCREMENTS
		{
			*x_ret = (c->width - basex) / c->size->width_inc;
			*y_ret = (c->height - basey) / c->size->height_inc;
		}

		return 1;
	}

	return 0;
}

void WindowManager::initClientPosition(std::shared_ptr<Client> c)
{
	int mouse_x=0, mouse_y=0;
	unsigned int w, h, border_width, depth;
	XWindowAttributes attr;

	int theight = getClientTitleHeight(c);

	XGetWindowAttributes(dpy, c->window, &attr);

	if (attr.map_state == IsViewable)
		return;

	XGetGeometry(dpy, c->window, &root, &c->x, &c->y, &w, &h, &border_width, &depth);

	c->width = (int)w;
	c->height = (int)h;

	if (c->size->flags & PPosition)
	{
		if(!c->x)
			c->x = c->size->x;
		if(!c->y)
			c->y = c->size->y;
	}
	else
	{
		if (c->size->flags & USPosition)
		{
			if(!c->x)
				c->x = c->size->x;
			if(!c->y)
				c->y = c->size->y;
		}
		else if ((c->x == 0) || (c->y == 0))
		{
			if(c->width >= xres && c->height >= yres)
			{
				c->x = 0;
				c->y = 0;
				c->width = xres;
				c->height = yres-theight;
			}
			else
			{
				getMousePosition(&mouse_x, &mouse_y);

				if(mouse_x && mouse_y)
				{
					c->x = (int) (((long) (xres - c->width) * (long) mouse_x) / (long) xres);
					c->y = (int) (((long) (yres - c->height - theight) * (long) mouse_y) / (long) yres);
					c->y = (c->y<theight) ? theight : c->y;

					gravitateClient(c, Gravity::REMOVE);
				}
			}
		}
	}
}

void WindowManager::reparentClient(std::shared_ptr<Client> c)
{
	XSetWindowAttributes pattr;

	XGrabServer(dpy);

	pattr.background_pixel = fc.pixel;
	pattr.border_pixel = fc.pixel;
	pattr.do_not_propagate_mask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask;
	pattr.override_redirect = False;
	pattr.event_mask = ButtonMotionMask         |
	                   SubstructureRedirectMask |
	                   SubstructureNotifyMask   |
	                   ButtonPressMask          |
	                   ButtonReleaseMask        |
	                   ExposureMask             |
	                   EnterWindowMask          |
	                   LeaveWindowMask          ;

	auto theight = getClientTitleHeight(c);
	auto b_w = 0;
	auto BW = (c->has_border ? config.borderWidth : 0);

	if(c->border_width)
	{
		b_w = c->border_width;
		XSetWindowBorderWidth(dpy, c->window, 0);
	}
	else
		b_w = BW;

	c->frame = XCreateWindow(dpy,
	                         root,
	                         c->x,
	                         c->y - theight,
	                         c->width,
	                         c->height + theight,
	                         b_w,
	                         DefaultDepth(dpy, screen),
	                         CopyFromParent,
	                         DefaultVisual(dpy, screen),
	                         CWOverrideRedirect|CWDontPropagate|CWBackPixel|CWBorderPixel|CWEventMask,
	                         &pattr);

	c->title = XCreateWindow(dpy,
	                         c->frame,
	                         0,
	                         0,
	                         c->width,
	                         theight,
	                         0,
	                         DefaultDepth(dpy, screen),
	                         CopyFromParent,
	                         DefaultVisual(dpy, screen),
	                         CWOverrideRedirect|CWDontPropagate|CWBackPixel|CWBorderPixel|CWEventMask,
	                         &pattr);

	if (shape)
	{
		XShapeSelectInput(dpy, c->window, ShapeNotifyMask);
		setClientShape(c);
	}

	XChangeWindowAttributes(dpy, c->window, CWDontPropagate, &pattr);
	XSelectInput(dpy, c->window, FocusChangeMask|PropertyChangeMask);
	XReparentWindow(dpy, c->window, c->frame, 0, theight);
	XGrabButton(dpy,
	            Button1,
	            AnyModifier,
	            c->frame,
	            1,
	            ButtonPressMask|ButtonReleaseMask,
	            GrabModeSync,
	            GrabModeAsync, None, None);

	sendClientConfig(c);

	XSync(dpy, false);
	XUngrabServer(dpy);
}

int WindowManager::getClientTitleHeight(std::shared_ptr<Client> c)
{
	if (!c->has_title) return 0;

	return (c->trans) ? config.transientWindowHeight : (font->ascent + font->descent + config.space);
}

void WindowManager::sendClientConfig(std::shared_ptr<Client> c)
{
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.event = c->window;
	ce.window = c->window;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->width;
	ce.height = c->height;
	ce.border_width = c->border_width;
	ce.override_redirect = False;

	XSendEvent(dpy, c->window, False, StructureNotifyMask, (XEvent *)&ce);
}

void WindowManager::gravitateClient(std::shared_ptr<Client> c, Gravity multiplier)
{
	auto dy = 0;
	auto gravity = (c->size->flags & PWinGravity) ? c->size->win_gravity : NorthWestGravity;
	auto theight = getClientTitleHeight(c);

	switch (gravity)
	{
	case NorthWestGravity:
	case NorthEastGravity:
	case NorthGravity:
		dy = theight;
		break;
	case CenterGravity:
		dy = theight/2;
		break;
	}

	c->y += (int)multiplier * dy;
}

void WindowManager::setClientShape(std::shared_ptr<Client> c)
{
	auto n=0, order=0;
	XRectangle temp, *dummy;
	auto theight = getClientTitleHeight(c);
	auto BW = (c->has_border ? config.borderWidth : 0);

	dummy = XShapeGetRectangles(dpy, c->window, ShapeBounding, &n, &order);

	if (n > 1)
	{
		XShapeCombineShape(dpy, c->frame, ShapeBounding, 0, theight, c->window, ShapeBounding, ShapeSet);

		temp.x = -BW;
		temp.y = -BW;
		temp.width = c->width + 2*BW;
		temp.height = theight + BW;

		XShapeCombineRectangles(dpy, c->frame, ShapeBounding, 0, 0, &temp, 1, ShapeUnion, YXBanded);

		temp.x = 0;
		temp.y = 0;
		temp.width = c->width;
		temp.height = theight - BW;

		XShapeCombineRectangles(dpy, c->frame, ShapeClip, 0, theight, &temp, 1, ShapeUnion, YXBanded);

		c->has_been_shaped = 1;
	}
	else if (c->has_been_shaped)
	{
		temp.x = -BW;
		temp.y = -BW;
		temp.width = c->width + 2*BW;
		temp.height = c->height + theight + 2*BW;

		XShapeCombineRectangles(dpy, c->frame, ShapeBounding, 0, 0, &temp, 1, ShapeSet, YXBanded);
	}

	XFree(dummy);
}

void WindowManager::focusPreviousWindowInStackingOrder()
{
	unsigned int nwins;
	Window dummyw1, dummyw2, *wins;
	std::shared_ptr<Client> c;

	if(client_list.size())
	{
		XQueryTree(dpy, root, &dummyw1, &dummyw2, &wins, &nwins);

		std::list<std::shared_ptr<Client>> client_list_for_current_desktop;

		for (unsigned int i = 0; i < nwins; i++)
		{
			c = findClient(wins[i]);

			if(c && c->has_title)
				client_list_for_current_desktop.push_back(c);
		}

		if(client_list_for_current_desktop.size())
		{
			auto iter = client_list_for_current_desktop.end();

			iter--;

			if((*iter))
			{
				auto c = findClient((*iter)->window);

				if(c)
					setXFocus(c);

				client_list_for_current_desktop.clear();
			}
		}

		XFree(wins);
	}
}

void WindowManager::queryClientName(std::shared_ptr<Client> c)
{
	char* name = nullptr;

	XFetchName(dpy, c->window, &name);

	if(name)
	{
		c->name = name;
		XFree(name);
	} else {
    XTextProperty text;
    XGetWMName(dpy, c->window, &text);
    c->name = (char*) text.value;
  }
  
  if (c->name.compare("unknown") == 0) { 
    XClassHint class_hint;
    XGetClassHint(dpy, c->window, &class_hint);
   
    if(class_hint.res_name)
    {
      c->name = class_hint.res_name;
      XFree(class_hint.res_name);
    }
  }

	XTextExtents(font, c->name.c_str(), c->name.length(), &c->direction, &c->ascent, &c->descent, &c->overall);
	c->text_width = c->overall.width;

}

void WindowManager::sigHandler(int signal) {
	switch (signal)
	{
	case SIGINT:
	case SIGTERM:
		wm->quitNicely();
		break;

	case SIGHUP:
		wm->restart();
		break;
	}
}

int handleXError(Display *dpy, XErrorEvent *e)
{
	if ((e->error_code == BadAccess) && (e->resourceid == RootWindow(dpy, DefaultScreen(dpy))))
	{
		std::cerr << "The root window unavailable!" << std::endl;
		exit(1);
	}

	return 0;
}

int main(int argc, char** argv)
{
	if(argc>=2 && strcmp(argv[1], "--version") == 0)
		std::cout << VERSION_STRING << std::endl;
	else
		WindowManager mini(argc, argv);

	return 0;
}
