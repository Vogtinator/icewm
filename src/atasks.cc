#include "config.h"

#include "ylib.h"
#include "atasks.h"
#include "wmtaskbar.h"
#include "yprefs.h"
#include "prefs.h"
#include "yxapp.h"
#include "wmmgr.h"
#include "wmframe.h"
#include "wmwinlist.h"
#include "wpixmaps.h"
#include "yrect.h"

static YColor *normalTaskBarAppFg = 0;
static YColor *normalTaskBarAppBg = 0;
static YColor *activeTaskBarAppFg = 0;
static YColor *activeTaskBarAppBg = 0;
static YColor *minimizedTaskBarAppFg = 0;
static YColor *minimizedTaskBarAppBg = 0;
static YColor *invisibleTaskBarAppFg = 0;
static YColor *invisibleTaskBarAppBg = 0;
static ref<YFont> normalTaskBarFont;
static ref<YFont> activeTaskBarFont;

YTimer *TaskBarApp::fRaiseTimer = 0;

TaskBarApp::TaskBarApp(ClientData *frame, TaskPane *taskPane, YWindow *aParent): YWindow(aParent) {
    if (normalTaskBarAppFg == 0) {
        normalTaskBarAppBg = new YColor(clrNormalTaskBarApp);
        normalTaskBarAppFg = new YColor(clrNormalTaskBarAppText);
        activeTaskBarAppBg = new YColor(clrActiveTaskBarApp);
        activeTaskBarAppFg = new YColor(clrActiveTaskBarAppText);
        minimizedTaskBarAppBg = new YColor(clrMinimizedTaskBarApp);
        minimizedTaskBarAppFg = new YColor(clrMinimizedTaskBarAppText);
        invisibleTaskBarAppBg = new YColor(clrInvisibleTaskBarApp);
        invisibleTaskBarAppFg = new YColor(clrInvisibleTaskBarAppText);
        normalTaskBarFont = YFont::getFont(XFA(normalTaskBarFontName));
        activeTaskBarFont = YFont::getFont(XFA(activeTaskBarFontName));
    }
    fTaskPane = taskPane;
    fFrame = frame;
    selected = 0;
    fShown = true;
    fFlashing = false;
    fFlashOn = false;
    fFlashTimer = 0;
    fFlashStart = zerotime();
    setToolTip(frame->getTitle());
    //setDND(true);
}

TaskBarApp::~TaskBarApp() {
    if (fTaskPane->dragging() == this)
        fTaskPane->endDrag();

    if (fRaiseTimer && fRaiseTimer->getTimerListener() == this) {
        fRaiseTimer->stopTimer();
        fRaiseTimer->setTimerListener(0);
    }
    delete fFlashTimer; fFlashTimer = 0;
}

bool TaskBarApp::isFocusTraversable() {
    return true;
}

void TaskBarApp::setShown(bool ashow) {
    if (ashow != fShown) {
        fShown = ashow;
    }
}

void TaskBarApp::setFlash(bool flashing) {
    if (fFlashing != flashing) {
        fFlashing = flashing;

        if (fFlashing && focusRequestFlashInterval > 0) {
            fFlashOn = true;
            fFlashStart = monotime();
            if (fFlashTimer == 0)
                fFlashTimer = new YTimer(focusRequestFlashInterval);
            if (fFlashTimer) {
                fFlashTimer->setTimerListener(this);
                fFlashTimer->startTimer();
            }
        } else {
            //fFlashTimer->stopTimer();
        }
    }
}

void TaskBarApp::paint(Graphics &g, const YRect &/*r*/) {
    YColor *bg, *fg;
    ref<YPixmap> bgPix;
    ref<YImage> bgGrad;

    int p(0);
    int style = 0;

    if (selected == 3)
        style = 3;
    else if (getFrame()->focused() || selected == 2)
        style = 2;
    else
        style = 1;

    if (fFlashing) {
        if (fFlashOn) {
            bg = activeTaskBarAppBg;
            fg = activeTaskBarAppFg;
            bgPix = taskbuttonactivePixmap;
            bgGrad = taskbuttonactivePixbuf;
            style = 1;
        } else {
            bg = normalTaskBarAppBg;
            fg = normalTaskBarAppFg;
            bgPix = taskbuttonPixmap;
            bgGrad = taskbuttonPixbuf;
            style = 1;
        }
    } else if (!getFrame()->visibleNow()) {
        bg = invisibleTaskBarAppBg;
        fg = invisibleTaskBarAppFg;
        bgPix = taskbackPixmap;
        bgGrad = taskbackPixbuf;
    } else if (getFrame()->isMinimized()) {
        bg = minimizedTaskBarAppBg;
        fg = minimizedTaskBarAppFg;
        bgPix = taskbuttonminimizedPixmap;
        bgGrad = taskbuttonminimizedPixbuf;
    } else if (getFrame()->focused()) {
        bg = activeTaskBarAppBg;
        fg = activeTaskBarAppFg;
        bgPix = taskbuttonactivePixmap;
        bgGrad = taskbuttonactivePixbuf;
    } else {
        bg = normalTaskBarAppBg;
        fg = normalTaskBarAppFg;
        bgPix = taskbuttonPixmap;
        bgGrad = taskbuttonPixbuf;
    }

    if (style == 3) {
        p = 2;
        g.setColor(YColor::black);
        g.drawRect(0, 0, width() - 1, height() - 1);
        g.setColor(bg);
        g.fillRect(1, 1, width() - 2, height() - 2);
    } else {
        g.setColor(bg);

        if (style == 2) {
            p = 2;
            if (wmLook == lookMetal) {
                g.drawBorderM(0, 0, width() - 1, height() - 1, false);
            } else if (wmLook == lookGtk) {
                g.drawBorderG(0, 0, width() - 1, height() - 1, false);
            }
            else if (wmLook != lookFlat)
                g.drawBorderW(0, 0, width() - 1, height() - 1, false);
        } else {
            p = 1;
            if (wmLook == lookMetal) {
                p = 2;
                g.drawBorderM(0, 0, width() - 1, height() - 1, true);
            } else if (wmLook == lookGtk) {
                g.drawBorderG(0, 0, width() - 1, height() - 1, true);
            }
            else if (wmLook != lookFlat)
                g.drawBorderW(0, 0, width() - 1, height() - 1, true);
        }

        int const dp(wmLook == lookFlat ? 0: wmLook == lookMetal ? 2 : p);
        int const ds(wmLook == lookFlat ? 0: wmLook == lookMetal ? 4 : 3);

        if (width() > ds && height() > ds) {
            if (bgGrad != null)
                g.drawGradient(bgGrad, dp, dp, width() - ds, height() - ds);
            else
            if (bgPix != null)
                g.fillPixmap(bgPix, dp, dp, width() - ds, height() - ds);
            else
                g.fillRect(dp, dp, width() - ds, height() - ds);
        }
    }

    ref<YIcon> icon(getFrame()->getIcon());

    if (taskBarShowWindowIcons && icon != null) {
        int iconSize = YIcon::smallSize();

        int const y((height() - 3 - iconSize -
                     ((wmLook == lookMetal) ? 1 : 0)) / 2);
        icon->draw(g, p + 1, p + 1 + y, iconSize);
    }

    ustring str = getFrame()->getIconTitle();
    if (str == null || str.length() == 0)
        str = getFrame()->getTitle();

    if (str != null) {
        ref<YFont> font =
            getFrame()->focused() ? activeTaskBarFont : normalTaskBarFont;

        if (font != null) {
            g.setColor(fg);
            g.setFont(font);

            int iconSize = 0;
            int pad = 1;
            if (taskBarShowWindowIcons && icon != null) {
                iconSize = YIcon::smallSize();
                pad = 3;
            }
            int const tx = pad + iconSize;
            int const ty = max(2,
                               (height() + font->height() -
                                ((wmLook == lookMetal || wmLook == lookFlat) ? 2 : 1)) / 2 -
                               font->descent());
            int const wm = width() - p - pad - iconSize - 1;

            g.drawStringEllipsis(p + tx, p + ty, str, wm);
        }
    }
}

void TaskBarApp::handleButton(const XButtonEvent &button) {
    YWindow::handleButton(button);

    if (fTaskPane->dragging())
        return;

    if (button.button == 1 || button.button == 2) {
        if (button.type == ButtonPress) {
            selected = 2;
            repaint();
        } else if (button.type == ButtonRelease) {
            if (selected == 2) {
                if (button.button == 1) {
                    if (getFrame()->focused() && getFrame()->visibleNow() &&
                        (!getFrame()->canRaise() || (button.state & ControlMask)))
                        getFrame()->wmMinimize();
                    else {
                        if (button.state & ShiftMask)
                            getFrame()->wmOccupyOnlyWorkspace(manager->activeWorkspace());
                        getFrame()->activateWindow(true);
                    }
                } else if (button.button == 2) {
                    if (hasbit(button.state, xapp->AltMask)) {
                        if (getFrame()) {
                            getFrame()->activateWindow(true);
                            if (manager->getFocus()) {
                                manager->getFocus()->wmClose();
                                return;
                            }
                        }
                    } else
                    if (getFrame()->focused() && getFrame()->visibleNow() &&
                        (!getFrame()->canRaise() || (button.state & ControlMask)))
                        getFrame()->wmLower();
                    else {
                        if (button.state & ShiftMask)
                            getFrame()->wmOccupyWorkspace(manager->activeWorkspace());
                        getFrame()->activateWindow(true);
                    }
                }
            }
            selected = 0;
            repaint();
        }
    }
}

void TaskBarApp::handleCrossing(const XCrossingEvent &crossing) {
    if (selected > 0) {
        if (crossing.type == EnterNotify) {
            selected = 2;
            repaint();
        } else if (crossing.type == LeaveNotify) {
            selected = 1;
            repaint();
        }
    }
    YWindow::handleCrossing(crossing);
}

void TaskBarApp::handleClick(const XButtonEvent &up, int /*count*/) {
    if (up.button == 3) {
        getFrame()->popupSystemMenu(this, up.x_root, up.y_root,
                                    YPopupWindow::pfCanFlipVertical |
                                    YPopupWindow::pfCanFlipHorizontal |
                                    YPopupWindow::pfPopupMenu);
    }
}

void TaskBarApp::handleDNDEnter() {
    if (fRaiseTimer == 0)
        fRaiseTimer = new YTimer(autoRaiseDelay);
    if (fRaiseTimer) {
        fRaiseTimer->setTimerListener(this);
        fRaiseTimer->startTimer();
    }
    selected = 3;
    repaint();
}

void TaskBarApp::handleDNDLeave() {
    if (fRaiseTimer && fRaiseTimer->getTimerListener() == this) {
        fRaiseTimer->stopTimer();
        fRaiseTimer->setTimerListener(0);
    }
    selected = 0;
    repaint();
}

bool TaskBarApp::handleTimer(YTimer *t) {
    if (t == fRaiseTimer) {
        getFrame()->wmRaise();
    }
    if (t == fFlashTimer) {
        if (!fFlashing) {
            fFlashOn = 0;
            fFlashStart = zerotime();
            return false;
        }
        fFlashOn = !fFlashOn;
        if (focusRequestFlashTime > 0) {
            if (fFlashStart + focusRequestFlashTime < monotime())
                fFlashing = false;
        }
        repaint();
        return fFlashing;
    }
    return false;
}

void TaskBarApp::handleBeginDrag(const XButtonEvent &down, const XMotionEvent &motion) {
    if (down.button == Button1) {
        raise();
        fTaskPane->startDrag(this, 0, down.x + x(), down.y + y());
        fTaskPane->processDrag(motion.x + x(), motion.y + y());
    }
}

TaskPane::TaskPane(IAppletContainer *taskBar, YWindow *parent): YWindow(parent) {
    fTaskBar = taskBar;
    fNeedRelayout = true;
    fDragging = 0;
    fDragX = fDragY = 0;
}

TaskPane::~TaskPane() {
    if (fDragging != 0)
        endDrag();
}

void TaskPane::insert(TaskBarApp *tapp) {
    fApps.append(tapp);
}

void TaskPane::remove(TaskBarApp *tapp) {
    findRemove(fApps, tapp);
}

TaskBarApp *TaskPane::addApp(YFrameWindow *frame) {
    if (frame->client() == windowList)
        return 0;
    if (frame->client() == taskBar)
        return 0;

    TaskBarApp *tapp = new TaskBarApp(frame, this, this);

    if (tapp != 0) {
        insert(tapp);
#if 0
        tapp->show();
        if (!frame->visibleOn(manager->activeWorkspace()) &&
            !taskBarShowAllWindows)
            tapp->setShown(0);
        if (frame->owner() != 0 && !taskBarShowTransientWindows)
            tapp->setShown(0);
        relayout();
#endif
    }
    return tapp;
}

void TaskPane::removeApp(YFrameWindow *frame) {
    for (IterType task = fApps.iterator(); ++task; ) {
        if (task->getFrame() == frame) {
            task->hide();
            remove(task);
            relayout();
            return;
        }
    }
}

void TaskPane::relayoutNow() {
    if (!fNeedRelayout)
        return ;

    fNeedRelayout = false;

    int tc = 0;

    for (IterType task = fApps.iterator(); ++task; ) {
        if (task->getShown())
            tc++;
        else
            task->hide();
    }
    if (tc == 0)
        return;
    tc = max(tc, taskBarButtonWidthDivisor);

    const int wid = (width() - 2) / tc;
    const int rem = (width() - 2) % tc;
    int x = 0;
    int lc = 0;

    for (IterType task = fApps.iterator(); ++task; ) {
        if (task->getShown()) {
            const int w1 = wid + (lc < rem);
            if (task != dragging()) {
                task->setGeometry(YRect(x, 0, w1, height()));
                task->show();
            }
            x += w1;
            lc++;
        }
    }
    if (dragging())
        dragging()->show();
}

void TaskPane::handleClick(const XButtonEvent &up, int count) {
    if (up.button == 3 && count == 1 && IS_BUTTON(up.state, Button3Mask)) {
        fTaskBar->contextMenu(up.x_root, up.y_root);
    }
}

void TaskPane::paint(Graphics &g, const YRect &/*r*/) {
    g.setColor(getTaskBarBg());
    //g.draw3DRect(0, 0, width() - 1, height() - 1, true);

    // When TaskBarDoubleHeight=1 this draws the lower half.
    ref<YImage> gradient = parent()->getGradient();

    if (gradient != null)
        g.drawImage(gradient, 0, taskBarDoubleHeight ? 0 : y(),
                    width(), height(), 0, 0);
    else
    if (taskbackPixmap != null)
        g.fillPixmap(taskbackPixmap, 0, 0, width(), height(), x(), y());
    else
        g.fillRect(0, 0, width(), height());
}

void TaskPane::handleMotion(const XMotionEvent &motion) {
    if (fDragging != 0) {
        processDrag(motion.x, motion.y);
    }
}

void TaskPane::handleButton(const XButtonEvent &button) {
    if (button.type == ButtonRelease)
        endDrag();
    YWindow::handleButton(button);
}

void TaskPane::startDrag(TaskBarApp *drag, int /*byMouse*/, int sx, int sy) {
    if (fDragging == 0) {
        if (!xapp->grabEvents(this, YXApplication::movePointer.handle(),
                              ButtonPressMask |
                              ButtonReleaseMask |
                              PointerMotionMask, 1, 1, 0))
        {
            return ;
        }
        fDragging = drag;
        fDragX = sx;
        fDragY = sy;
    }
}

void TaskPane::processDrag(int mx, int my) {
    if (fDragging == 0)
        return;

    const int w2 = width() - fDragging->width() - 2;
    const int ox = fDragX - fDragging->x();
    const int oy = fDragY - fDragging->y();
    const int cx = clamp(mx, ox, ox + w2);
    const int cy = clamp(my, oy - 1, oy + 1);
    const int dx = cx - fDragX;
    const int dy = cy - fDragY;
    if (dx == 0 && dy == 0)
        return;

    fDragging->setPosition(fDragging->x() + dx, fDragging->y() + dy);

    int drag = -1;
    int drop = -1;
    for (IterType task = fApps.iterator(); ++task; ) {
        if (task->getShown()) {
            if (task == fDragging)
                drag = task.where();
            else if (inrange(cx, task->x(), task->x() + task->width() - 1))
                drop = task.where();
        }
    }
    if (drag >= 0 && drop >= 0) {
        fApps.swap(drag, drop);
    }
    fDragX += dx;
    fDragY += dy;
    relayout();
    relayoutNow();
}

void TaskPane::endDrag() {
    if (fDragging != 0) {
        xapp->releaseEvents();
        fDragging = 0;
        relayout();
        relayoutNow();
    }
}

// vim: set sw=4 ts=4 et:
