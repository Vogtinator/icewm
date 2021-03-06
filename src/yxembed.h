#ifndef __YXEMBED_H
#define __YXEMBED_H

#include "ywindow.h"
#include "yarray.h"
#include "yxapp.h"
#include <X11/Xatom.h>

#define XEMBED_PROTOCOL_VERSION         0

#define XEMBED_MAPPED                   (1<<0)

#define XEMBED_EMBEDDED_NOTIFY          0
#define XEMBED_WINDOW_ACTIVATE          1
#define XEMBED_WINDOW_DEACTIVATE        2
#define XEMBED_REQUEST_FOCUS            3
#define XEMBED_FOCUS_IN                 4
#define XEMBED_FOCUS_OUT                5
#define XEMBED_FOCUS_NEXT               6
#define XEMBED_FOCUS_PREV               7
#define XEMBED_MODALITY_ON              10
#define XEMBED_MODALITY_OFF             11
#define XEMBED_REGISTER_ACCELERATOR     12
#define XEMBED_UNREGISTER_ACCELERATOR   13
#define XEMBED_ACTIVATE_ACCELERATOR     14

class YXEmbedClient;

class YXEmbed: public YWindow {
public:
    YXEmbed(YWindow *aParent = 0);
    virtual ~YXEmbed();

//    YObjectArray<YWindow> fClients;

//    YXEmbedClient *manage(YXEmbed *embedder, Window win);
    virtual bool destroyedClient(Window /*win*/) = 0;
    virtual void handleClientUnmap(Window win) = 0;
    virtual void handleClientMap(Window win) = 0;
};

class YXEmbedClient: public YWindow {
public:
    YXEmbedClient(YXEmbed *embedder, YWindow *aParent = 0, Window win = 0);
    virtual ~YXEmbedClient();

    void handleDestroyWindow(const XDestroyWindowEvent &destroyWindow);
    void handleReparentNotify(const XReparentEvent &reparent);
    void handleProperty(const XPropertyEvent &property);
    void handleUnmap(const XUnmapEvent &unmap);

private:
    YXEmbed *fEmbedder;
    YAtom _XEMBED_INFO;
};

#endif

// vim: set sw=4 ts=4 et:
