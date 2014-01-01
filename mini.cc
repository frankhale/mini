// A new window manager based off my other window manager aewm++
// Copyright (C) 2010-2012 Frank Hale <frankhale@gmail.com>
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
// Date: 22 August 2012

#include "mini.hh"

WindowManager *wm;

KeySym WindowManager::alt_keys[] = { XK_Delete, XK_End };

void WindowManager::grabKeys(Window w)
{
  for(int i=0;i<ALT_KEY_COUNT;i++)
    XGrabKey(dpy, XkbKeycodeToKeysym(dpy, alt_keys[i], 0, 1), (Mod1Mask|ControlMask), w,True,GrabModeAsync,GrabModeAsync);
}

void WindowManager::ungrabKeys(Window w)
{
  for(int i=0;i<ALT_KEY_COUNT;i++)
    XUngrabKey(dpy, XkbKeycodeToKeysym(dpy,alt_keys[i], 0, 1), (Mod1Mask|ControlMask),w);
}

WindowManager::WindowManager(int argc, char** argv)
{
  if(argc>=2)
  {
    if(strcmp(argv[1], "--version")==0)
    {
      std::cout << VERSION_STRING << std::endl;
      exit(0);
    }
  }
    
  int dummy;     // not used but needed to satisfy XShapeQueryExtension call
  XColor dummyc; // not used but needed to satisfy XAllocNamedColor call
  
  XGCValues gv;
  XSetWindowAttributes sattr;
  focused_client=NULL;
  focus_model = DEFAULT_FOCUS_MODEL;

  wm = this;

  for (int i = 0; i < argc; i++)
    command_line = command_line + argv[i] + " ";

  signal(SIGINT, sigHandler);
  signal(SIGTERM, sigHandler);
  signal(SIGHUP, sigHandler);
  signal(SIGCHLD, sigHandler);

  WindowPlacement window_placement = DEFAULT_WINDOW_PLACEMENT;

  if(window_placement == WindowPlacement::MOUSE)
    random_window_placement = false;
  else if(window_placement == WindowPlacement::RANDOM)
    random_window_placement = true;

  dpy = XOpenDisplay(getenv("DISPLAY"));

  if(dpy)
  {
    font = XLoadQueryFont(dpy, DEFAULT_FONT);

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

  // SET UP ATOMS
  atom_wm_state       = XInternAtom(dpy, "WM_STATE", False);
  atom_wm_change_state  = XInternAtom(dpy, "WM_CHANGE_STATE", False);
  atom_wm_protos       = XInternAtom(dpy, "WM_PROTOCOLS", True);
  atom_wm_delete       = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  atom_wm_takefocus    = XInternAtom(dpy, "WM_TAKE_FOCUS", True);

  XSetWindowAttributes pattr;
  pattr.override_redirect=True;
  _button_proxy_win=XCreateSimpleWindow(dpy, root, -80, -80, 24, 24,0,0,0);
  XChangeWindowAttributes(dpy, _button_proxy_win, CWOverrideRedirect, &pattr);

  // SETUP COLORS USED FOR WINDOW TITLE BARS and WINDOW BORDERS
  XAllocNamedColor(dpy, DefaultColormap(dpy, screen), DEFAULT_FOREGROUND_COLOR, &fg, &dummyc);
  XAllocNamedColor(dpy, DefaultColormap(dpy, screen), DEFAULT_BACKGROUND_COLOR, &bg, &dummyc);
  XAllocNamedColor(dpy, DefaultColormap(dpy, screen), DEFAULT_BORDER_COLOR, &bd, &dummyc);
  XAllocNamedColor(dpy, DefaultColormap(dpy, screen), DEFAULT_FOCUS_COLOR, &fc, &dummyc);
  XAllocNamedColor(dpy, DefaultColormap(dpy, screen), FOCUSED_BORDER_COLOR, &focused_border, &dummyc);
  XAllocNamedColor(dpy, DefaultColormap(dpy, screen), UNFOCUSED_BORDER_COLOR, &unfocused_border, &dummyc);

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

  gv.foreground = bd.pixel;
  gv.line_width = DEFAULT_BORDER_WIDTH;
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
                     EnterWindowMask          |
                     LeaveWindowMask          |
                     PropertyChangeMask       |
                     ButtonMotionMask         ;

  XChangeWindowAttributes(dpy, root, CWEventMask, &sattr);

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

  XMapWindow(dpy, _button_proxy_win);
  grabKeys(_button_proxy_win);
  XSetInputFocus(dpy, _button_proxy_win, RevertToNone, CurrentTime);
}

void WindowManager::addClient(Window w)
{
  XWindowAttributes attr;
  XWMHints *hints;
  long dummy;

  client_window_list.push_back(w);

  Client *c = new Client();
  client_list.push_back(c);
  XGrabServer(dpy);

  initializeClient(c);

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
  
  XGetWMNormalHints(dpy, c->window, c->size, &dummy);

  c->old_x = c->x;
  c->old_y = c->y;
  c->old_width = c->width;
  c->old_height = c->height;

  initClientPosition(c);

  if ((hints = XGetWMHints(dpy, w)))
  {
    if(hints->flags & InputHint)
      c->should_takefocus=!hints->input;
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

void WindowManager::removeClient(Client* c)
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

WindowManager::Client* WindowManager::findClient(Window w)
{
  if(client_list.size())
  {
    std::list<Client*>::iterator iter = client_list.begin();

    for(; iter != client_list.end(); iter++)
    {
      if (w == (*iter)->title  ||
          w == (*iter)->frame  ||
          w == (*iter)->window)
        return (*iter);
    }
  }

  return NULL;
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

    case EnterNotify:
      handleEnterNotifyEvent(&ev);
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
  KeySym ks = XkbKeycodeToKeysym(dpy, ev->xkey.keycode, 0, 1);

  if (ks==NoSymbol)
    return;

  switch(ks)
  {
  case XK_Delete:
    std::cerr << "Window manager is restarting..." << std::endl;
    restart();
    break;

  case XK_End:
    std::cerr << "Window manager is quitting." << std::endl;
    quitNicely();
    break;
  }
}

void WindowManager::handleButtonPressEvent(XEvent *ev)
{
  Client* c = findClient(ev->xbutton.window);

  if(c && c->has_title)
  {
    if((ev->xbutton.button == Button1) &&
        (ev->xbutton.type==ButtonPress) &&
        (ev->xbutton.state==Mod1Mask)   &&
        (c->frame == ev->xbutton.window)
      )

      if(!XGrabPointer(dpy, c->frame, False, PointerMotionMask|ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, move_curs, CurrentTime) == GrabSuccess)
        return;
  }

  switch (focus_model)
  {
    case FocusMode::FOLLOW:
    case FocusMode::SLOPPY:
    if(c)
    {
      handleClientButtonEvent(&ev->xbutton, c);
      focused_client = c;
    }
    break;

    case FocusMode::CLICK:
    if(c)
    {
      // if this is the first time the client window's clicked, focus it
      if(c != focused_client)
      {
        //XSetInputFocus(dpy, c->window, RevertToPointerRoot, CurrentTime);
        setXFocus(c);
        focused_client = c;
      }

      handleClientButtonEvent(&ev->xbutton, c);
    }
    break;
  }
}

void WindowManager::handleButtonReleaseEvent(XEvent *ev)
{
  Client* c = findClient(ev->xbutton.window);

  if(c)
  {
    XUngrabPointer(dpy, CurrentTime);
    handleClientButtonEvent(&ev->xbutton, c);
  }
  else
  {
    if (ev->xbutton.window==root && ev->xbutton.button==Button3)
      forkExec(DEFAULT_CMD);
  }
}

void WindowManager::handleConfigureRequestEvent(XEvent *ev)
{
  Client* c = findClient(ev->xconfigurerequest.window);

  if(c)
    handleClientConfigureRequest(&ev->xconfigurerequest, c);
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
  Client* c = findClient(ev->xmotion.window);

  if(c)
    handleClientMotionNotifyEvent(&ev->xmotion, c);
}

void WindowManager::handleMapRequestEvent(XEvent *ev)
{
  Client* c = findClient(ev->xmaprequest.window);

  if(c)
    handleClientMapRequest(&ev->xmaprequest, c);
  else
    addClient(ev->xmaprequest.window);
}

void WindowManager::handleUnmapNotifyEvent(XEvent *ev)
{
  Client* c = findClient(ev->xunmap.window);

  if(c)
  {
    handleClientUnmapEvent(&ev->xunmap, c);
    focused_client = NULL;
  }
}

void WindowManager::handleDestroyNotifyEvent(XEvent *ev)
{
  Client* c = findClient(ev->xdestroywindow.window);

  if(c)
  {
    handleClientDestroyEvent(&ev->xdestroywindow, c);
    focused_client = NULL;
  }

  XSendEvent(dpy, _button_proxy_win, False, SubstructureNotifyMask, ev);
}

void WindowManager::handleEnterNotifyEvent(XEvent *ev)
{
  Client* c = findClient(ev->xcrossing.window);

  switch (focus_model)
  {
    case FocusMode::FOLLOW:
      if(c)
      {
        handleClientEnterEvent(&ev->xcrossing, c);
        focused_client = c;
      }
      else
        XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
    break;

    case FocusMode::SLOPPY:
      // if the pointer's not on a client now, don't change focus
      if(c)
      {
        handleClientEnterEvent(&ev->xcrossing, c);
        focused_client = c;
      }
      break;

    case FocusMode::CLICK:
      break;
  }
}

void WindowManager::handleFocusInEvent(XEvent *ev)
{
  if((ev->xfocus.mode==NotifyGrab) || (ev->xfocus.mode==NotifyUngrab))
    return;

  std::list<Window>::iterator iter;
  for(iter=client_window_list.begin(); iter != client_window_list.end(); iter++)
  {
    if(ev->xfocus.window == (*iter))
    {
      Client *c = findClient((*iter));

      if(c)
      {
        unfocusAnyStrayClients();
        handleClientFocusInEvent(&ev->xfocus, c);
        focused_client = c;
        grabKeys((*iter));
      }
    }
    else
    {
      if(ev->xfocus.window==root && focus_model==FocusMode::FOLLOW)
        unfocusAnyStrayClients();
    }
  }
}

void WindowManager::handleFocusOutEvent(XEvent *ev)
{
  std::list<Window>::iterator iter;
  for(iter=client_window_list.begin(); iter != client_window_list.end(); iter++)
  {
    if(ev->xfocus.window == (*iter))
    {
      Client *c = findClient( (*iter) );

      if(c)
      {
        focused_client = NULL;
        ungrabKeys( (*iter) );
        return;
      }
    }
  }

  if((focus_model == FocusMode::CLICK) ||
     (focus_model == FocusMode::SLOPPY))
    focusPreviousWindowInStackingOrder();
}

void WindowManager::handlePropertyNotifyEvent(XEvent *ev)
{
  Client* c = findClient(ev->xproperty.window);

  if(c)
    handleClientPropertyChange(&ev->xproperty, c);
}

void WindowManager::handleExposeEvent(XEvent *ev)
{
  Client* c = findClient(ev->xexpose.window);

  if(c)
    handleClientExposeEvent(&ev->xexpose, c);
}

void WindowManager::handleDefaultEvent(XEvent *ev)
{
  Client* c = findClient(ev->xany.window);

  if(c)
  {
    if (shape && ev->type == shape_event)
      handleClientShapeChange((XShapeEvent *)ev, c);
  }
}

void WindowManager::forkExec(std::string cmd)
{
  if(! (cmd.length()>0))
    return;

  switch(fork())
  {
    case 0:
      execlp("/bin/sh", "sh", "-c", cmd.c_str(), NULL);
    break;
  }
}

void WindowManager::restart()
{
  cleanup();

  execl("/bin/sh", "sh", "-c", command_line.c_str(), (char *)NULL);
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
  Client* c;

  // Preserve stacking order when removing the clientfrom the list.
  XQueryTree(dpy, root, &dummyw1, &dummyw2, &wins, &nwins);
  for (i = 0; i < nwins; i++)
  {
    c = findClient(wins[i]);

    if(c)
    {
      XMapWindow(dpy, c->window);
      removeClient(c);
      delete c;
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
  XDestroyWindow(dpy, _button_proxy_win);
  ungrabKeys(root);
  XCloseDisplay(dpy);
}

void WindowManager::unfocusAnyStrayClients()
{
  // To prevent two windows titlebars from being painted with the focus color we
  // will prevent that from happening by setting all windows to false.

  std::list<Client*>::iterator iter;

  for(iter=client_list.begin(); iter != client_list.end(); iter++)
    setClientFocus((*iter), false);
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

  if (XGetWindowProperty(dpy, window, atom_wm_state, 0L, 2L, False,
                         atom_wm_state, &real_type, &real_format, &items_read, &items_left,
                         &data) == Success && items_read)
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
  int i, n, found = 0;
  Atom *protocols;

  if (XGetWMProtocols(dpy, window, &protocols, &n))
  {
    for (i=0; i<n; i++)
      if (protocols[i] == atom_wm_delete)
        found++;

    XFree(protocols);
  }

  (found) ? sendXMessage(window, atom_wm_protos, RevertToNone, atom_wm_delete) : XKillClient(dpy, window);
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

void WindowManager::handleClientButtonEvent(XButtonEvent *ev, Client* c)
{
  // Formula to tell if the pointer is in the little
  // box on the right edge of the window. This box is
  // the iconify button, resize button and close button.
  int theight = getClientTitleHeight(c);

  int in_box = (ev->x >= c->width - theight) && (ev->y <= theight);

  //printf("in_box = %d", in_box);

  // Used to compute the pointer position on click
  // used in the motion handler when doing a window move.
  c->old_cx = c->x;
  c->old_cy = c->y;
  c->pointer_x = ev->x_root;
  c->pointer_y = ev->y_root;

  // Allow us to get clicks from anywhere on the window
  // so click to raise works.
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

        if(ev->time-c->last_button1_time<250)
        {
          maximizeClient(c);

          c->last_button1_time=0;

          return;
        }
        else
          c->last_button1_time=ev->time;
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
        c->do_drawoutline_once=false;
        c->is_being_resized=false;

        XResizeWindow(dpy, c->frame, c->width, c->height + theight);
        XResizeWindow(dpy, c->title, c->width, theight);
        XResizeWindow(dpy, c->window, c->width, c->height);

        sendClientConfig(c);

        XUngrabServer(dpy);
        XSync(dpy, False);

        return;
      }
    }
    break;
  }
}

void WindowManager::handleClientConfigureRequest(XConfigureRequestEvent *ev, Client* c)
{
  int theight = getClientTitleHeight(c);

  gravitateClient(c, Gravity::REMOVE);

  if (ev->value_mask & CWX)
    c->x = ev->x;
  if (ev->value_mask & CWY)
    c->y = ev->y;
  if (ev->value_mask & CWWidth)
    c->width = ev->width;
  if (ev->value_mask & CWHeight)
    c->height = ev->height;

  gravitateClient(c, Gravity::APPLY);

  if(! c->is_shaded)
  {
    XMoveResizeWindow(dpy, c->frame,c->x,c->y-theight, c->width, c->height+theight);
    XResizeWindow(dpy, c->title, c->width, theight);
    XMoveResizeWindow(dpy, c->window,0,theight,c->width,c->height);
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

void WindowManager::handleClientMapRequest(XMapRequestEvent *ev, Client* c)
{
  unhideClient(c);
}

void WindowManager::handleClientUnmapEvent(XUnmapEvent *ev, Client* c)
{
  if (! c->ignore_unmap)
  {
    removeClient(c);
    delete c;
  }
}

void WindowManager::handleClientDestroyEvent(XDestroyWindowEvent *ev, Client* c)
{
  if(c)
  {
    removeClient(c);
    delete c;
  }
}

void WindowManager::handleClientPropertyChange(XPropertyEvent *ev, Client* c)
{
  long dummy;

  switch (ev->atom)
  {
  case XA_WM_NAME:
    queryClientName(c);
    XClearWindow(dpy, c->title);
    redrawClient(c);
    break;

  case XA_WM_NORMAL_HINTS:
    XGetWMNormalHints(dpy, c->window, c->size, &dummy);
    break;
  }
}

void WindowManager::handleClientEnterEvent(XCrossingEvent *ev, Client* c)
{
  if(focus_model == FocusMode::FOLLOW)
    setXFocus(c);
}

void WindowManager::handleClientExposeEvent(XExposeEvent *ev, Client* c)
{
  if (ev->count == 0)
    redrawClient(c);
}

void WindowManager::handleClientFocusInEvent(XFocusChangeEvent *ev, Client* c)
{
  
  setClientFocus(c, true);
}

void WindowManager::handleClientMotionNotifyEvent(XMotionEvent *ev, Client* c)
{
  int nx=0, ny=0;
  int theight = getClientTitleHeight(c);

  if((ev->state & Button1Mask) && (focused_client == c))
  {
    if(! c->do_drawoutline_once && WIRE_MOVE)
    {
      XGrabServer(dpy);
      drawClientOutline(c);
      c->do_drawoutline_once=true;
      c->is_being_dragged=true;
    }

    if(WIRE_MOVE)
      drawClientOutline(c);

    nx = c->old_cx + (ev->x_root - c->pointer_x);
    ny = c->old_cy + (ev->y_root - c->pointer_y);

    if(EDGE_SNAP)
    {
      // Move beyond edges of screen
      if(nx == xres - c->width)
        nx = xres - c->width + 1;
      else if(nx == 0)
        nx = -1;

      if(ny == yres - SNAP)
        ny = yres - SNAP - 1;
      else if(ny == theight)
        ny = theight - 1;

      // Snap to edges of screen
      if( (nx + c->width >= xres - SNAP) && (nx + c->width <= xres) )
        nx = xres - c->width;
      else if( (nx <= SNAP) && (nx >= 0) )
        nx = 0;

      if(c->is_shaded)
      {
        if( (ny  >= yres - SNAP) && (ny  <= yres))
          ny = yres;
        else if( (ny - theight <= SNAP) && (ny - theight >= 0))
          ny = theight;
      }
      else
      {
        if((ny + c->height >= yres - SNAP) && (ny + c->height <= yres))
          ny = yres - c->height;
        else if((ny - theight <= SNAP) && (ny - theight >= 0))
          ny = theight;
      }
    }

    c->x=nx;
    c->y=ny;

    if(!WIRE_MOVE)
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
      int in_box = (ev->x >= c->width - theight) && (ev->y <= theight);

      if(! in_box)
        return;
    }

    if(! c->do_drawoutline_once)
    {
      c->is_being_resized=true;
      c->do_drawoutline_once=true;

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

          if(c->width<100)
            c->width=100;
          if(c->height<50)
            c->height=50;
        }

        if (c->size->flags & PMaxSize)
        {
          if (c->width > c->size->max_width)
            c->width = c->size->max_width;
          if (c->height > c->size->max_height)
            c->height = c->size->max_height;

          if(c->width>xres)
            c->width=xres;
          if(c->height>yres)
            c->height=yres;
        }

        drawClientOutline(c);
      }
    }
  }
}

void WindowManager::handleClientShapeChange(XShapeEvent *ev, Client* c)
{
  setClientShape(c);
}

void WindowManager::setClientFocus(Client* c, bool focus)
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

void WindowManager::setXFocus(Client *c)
{
  if(c->should_takefocus)
    sendXMessage(c->window, atom_wm_protos, 0L, atom_wm_takefocus);
  else
    XSetInputFocus(dpy, c->window, RevertToPointerRoot, CurrentTime);
}

void WindowManager::hideClient(Client* c)
{
  if (!c->ignore_unmap)
    c->ignore_unmap++;

  if(c->has_focus)
    setClientFocus(c, false);

  XUnmapSubwindows(dpy, c->frame);
  XUnmapWindow(dpy, c->frame);

  setWMState(c->window, WithdrawnState);

  c->is_visible=false;
}

void WindowManager::unhideClient(Client* c)
{
  XMapSubwindows(dpy, c->frame);
  XMapRaised(dpy, c->frame);

  setWMState(c->window, NormalState);

  setXFocus(c);

  c->is_visible=true;
}

void WindowManager::shadeClient(Client* c)
{
  int theight = getClientTitleHeight(c);

  XRaiseWindow(dpy, c->frame);

  if(! c->is_shaded)
  {
    XResizeWindow(dpy, c->frame, c->width, theight - 1);
    c->is_shaded=true;
  }
  else
  {
    XResizeWindow(dpy, c->frame, c->width, c->height + theight);
    c->is_shaded=false;
  }
}

void WindowManager::maximizeClient(Client* c)
{
  int theight = getClientTitleHeight(c);

  if(c->trans)
    return;

  if(c->is_shaded)
  {
    shadeClient(c);
    return;
  }

  if(! c->is_maximized)
  {
    c->old_x=c->x;
    c->old_y=c->y;
    c->old_width=c->width;
    c->old_height=c->height;

    if (c->size->flags & PMaxSize)
    {
      c->width = c->size->max_width;
      c->height = c->size->max_height;

      XMoveResizeWindow(dpy, c->frame, c->x, c->y-theight, c->width, c->height+theight);
    }
    else
    {
      c->x=0;
      c->y=0;
      c->width=xres-2;
      c->height=yres-2;

      XMoveResizeWindow(dpy, c->frame, c->x, c->y, c->width, c->height);

      c->y = theight;
      c->height -= theight;
    }

    c->is_maximized=true;
  }
  else
  {
    c->x=c->old_x;
    c->y=c->old_y;
    c->width=c->old_width;
    c->height=c->old_height;

    XMoveResizeWindow(dpy, c->frame, c->old_x, c->old_y - theight, c->old_width, c->old_height + theight);

    c->is_maximized=false;

    if(c->is_shaded)
      c->is_shaded=false;
  }

  XResizeWindow(dpy, c->title, c->width, theight);
  XResizeWindow(dpy, c->window, c->width, c->height);

  sendClientConfig(c);
}

void WindowManager::initializeClient(Client* c)
{
  c->window = None;
  c->frame = None;
  c->title = None;
  c->trans = None;
  c->x = 0;
  c->y = 0;
  c->old_x = 0;
  c->old_y = 0;
  c->old_cx = 0;
  c->old_cy = 0;
  c->pointer_x = 0;
  c->pointer_y = 0;
  c->width = 1;
  c->height = 1;
  c->old_width = 1;
  c->old_height = 1;
  c->ignore_unmap = 0;
  c->last_button1_time = 0;
  c->direction = 0;
  c->ascent = 0;
  c->descent = 0;
  c->text_width = 0;
  c->text_justify = 0;
  c->has_title = true;
  c->has_border = true;
  c->button_pressed = false;
  c->has_been_shaped = false;
  c->has_focus = false;
  c->is_shaded = false;
  c->is_maximized = false;
  c->is_visible = false;
  c->is_being_dragged = false;
  c->do_drawoutline_once = false;
  c->is_being_resized = false;
  c->should_takefocus = false;
}

void WindowManager::redrawClient(Client* c)
{
  if (!c->has_title)
    return;

  GC gc;

  int BW = (c->has_border ? DEFAULT_BORDER_WIDTH : 0);
  int theight = getClientTitleHeight(c);

  if(c->has_focus)
    gc = border_gc;
  else
    gc = unfocused_gc;

  XDrawLine(dpy, c->title, gc, 0, theight - BW + BW/2, c->width, theight - BW + BW/2);
  XDrawLine(dpy, c->title, gc, c->width - theight+ BW/2, 0, c->width - theight+ BW/2, theight);

  if(c->has_focus)
    gc = focused_title_gc;

  if (!c->trans && (c->name.length()>0))
  {
    switch(TEXT_JUSTIFY)
    {
     case JustifyMode::LEFT:
       c->text_justify = SPACE;
      break;

     case JustifyMode::CENTER:
      c->text_justify = ( (c->width / 2) - (c->text_width / 2) );
      break;

     case JustifyMode::RIGHT:
      c->text_justify = c->width - c->text_width - 25;
      break;
    }

    XDrawString(dpy, c->title, gc, c->text_justify, font->ascent+1, c->name.c_str(), c->name.length());
  }
}

void WindowManager::drawClientOutline(Client* c)
{
  int theight = getClientTitleHeight(c);
  int BW = (c->has_border ? DEFAULT_BORDER_WIDTH : 0);

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

int  WindowManager::getClientIncsize(Client* c, int *x_ret, int *y_ret, ResizeMode mode)
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

void WindowManager::initClientPosition(Client* c)
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
    else if ( (c->x==0) || (c->y==0)  )
    {
      if(c->width>=xres && c->height>=yres)
      {
        c->x=0;
        c->y=0;
        c->width=xres;
        c->height=yres-theight;
      }
      else
      {
        getMousePosition(&mouse_x, &mouse_y);

        if(mouse_x && mouse_y)
        {
          if(random_window_placement)
          {
            c->x = (rand() % (unsigned int) ((xres - c->width) * 0.94)) + ((unsigned int) (xres * 0.03));
            c->y = (rand() % (unsigned int) ((yres - c->height) * 0.94)) + ((unsigned int) (yres * 0.03));
          }
          else
          {
            c->x = (int) (((long) (xres - c->width) * (long) mouse_x) / (long) xres);
            c->y = (int) (((long) (yres - c->height - theight) * (long) mouse_y) / (long) yres);
            c->y = (c->y<theight) ? theight : c->y;
          }

          gravitateClient(c, Gravity::REMOVE);
        }
      }
    }
  }
}

void WindowManager::reparentClient(Client* c)
{
  XSetWindowAttributes pattr;

  XGrabServer(dpy);

  pattr.background_pixel = fc.pixel;
  pattr.border_pixel = bd.pixel;
  pattr.do_not_propagate_mask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask;
  pattr.override_redirect=False;
  pattr.event_mask = ButtonMotionMask         |
                     SubstructureRedirectMask |
                     SubstructureNotifyMask   |
                     ButtonPressMask          |
                     ButtonReleaseMask        |
                     ExposureMask             |
                     EnterWindowMask          |
                     LeaveWindowMask          ;

  int theight = getClientTitleHeight(c);
  int b_w = 0;
  int BW = (c->has_border ? DEFAULT_BORDER_WIDTH : 0);

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

int WindowManager::getClientTitleHeight(Client* c)
{
  if (!c->has_title) return 0;

  return (c->trans) ? TRANSIENT_WINDOW_HEIGHT : (font->ascent + font->descent + SPACE);
}

void WindowManager::sendClientConfig(Client* c)
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

void WindowManager::gravitateClient(Client* c, Gravity multiplier)
{
  int dy = 0;
  int gravity = (c->size->flags & PWinGravity) ? c->size->win_gravity : NorthWestGravity;
  int theight = getClientTitleHeight(c);

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

void WindowManager::setClientShape(Client* c)
{
  int n=0, order=0;
  XRectangle temp, *dummy;
  int theight = getClientTitleHeight(c);
  int BW = (c->has_border ? DEFAULT_BORDER_WIDTH : 0);

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
  Client *c=NULL;

  XSetInputFocus(dpy, _button_proxy_win, RevertToNone, CurrentTime);

  XQueryTree(dpy, root, &dummyw1, &dummyw2, &wins, &nwins);
  
  if(client_list.size())
  {
    std::list<Client*> client_list_for_current_desktop;

    for (unsigned int i = 0; i < nwins; i++)
    {
      c = findClient(wins[i]);

      if(c)
      {
        if(c->has_title)
          client_list_for_current_desktop.push_back(c);
      }
    }

    if(client_list_for_current_desktop.size())
    {
      std::list<Client*>::iterator iter = client_list_for_current_desktop.end();

      iter--;

      if( (*iter) )
      {
        Client* c = findClient((*iter)->window);
        
        if(c)
          setXFocus(c);

        client_list_for_current_desktop.clear();

        XFree(wins);

        return;
      }
    }
  }

  XFree(wins);
}

void WindowManager::queryClientName(Client* c)
{
  char* name=NULL;

  XFetchName(dpy, c->window, &name);

  if(name)
  {
    c->name=name;

    XTextExtents(font, c->name.c_str(), c->name.length(), &c->direction, &c->ascent, &c->descent, &c->overall);

    c->text_width = c->overall.width;

    XFree(name);
  }
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

  case SIGCHLD:
    wait(NULL);
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
  WindowManager mini(argc, argv);
  
  return 0;
}
