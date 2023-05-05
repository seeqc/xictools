
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *  See the License for the specific language governing permissions       *
 *  and limitations under the License.                                    *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL WHITELEY RESEARCH INCORPORATED      *
 *   OR STEPHEN R. WHITELEY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE       *
 *   USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                        *
 *========================================================================*
 *               XicTools Integrated Circuit Design System                *
 *                                                                        *
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "qthtext.h"
#include "qtinterf/qtfont.h"
#include "cd_property.h"
#include "dsp_inlines.h"
#include "select.h"
#include "menu.h"
#include "events.h"
#include "keymap.h"
#include <QLayout>
#include <QPushButton>
#include <QMenu>

// Help keywords:
//  promptline
//

/*
static GtkTargetEntry pl_targets[] = {
  { "STRING",     0, 0 },
  { "text/plain", 0, 1 },
  { "CELLNAME",   0, 2 },
  { "property",   0, 3 }
};
static guint n_pl_targets = 4;

static char *L_xpm[] = {
    // width height ncolors chars_per_pixel
    "6 8 2 1",
    // colors
    " 	c none",
    ".	c blue",
    // pixels
    ".     ",
    "..    ",
    "..    ",
    "..    ",
    "..    ",
    "..    ",
    "..... ",
    "......"};

static void l_btn_hdlr(GtkWidget*, void*, void*);
static int keys_hdlr(GtkWidget*, GdkEvent*, void*);
static int redraw_hdlr(GtkWidget*, GdkEvent*, void*);
static int btn_hdlr(GtkWidget*, GdkEvent*, void*);
void selection_proc(GtkWidget*, GtkSelectionData*, void*);
static void font_change_hdlr(GtkWidget*, void*, void*);
static void drag_data_received(GtkWidget*, GdkDragContext*, gint, gint,
    GtkSelectionData*, guint, guint);
*/

#ifdef notdef

// Callback for 'L' (long text) button
//
static void
l_btn_hdlr(GtkWidget*, void*, void*)
{       
    HY->hyLbtnPress();
}


// Pop up info about the keys pressed area in help mode.
//
static int
keys_hdlr(GtkWidget*, GdkEvent *event, void*)
{
    if (XM()->doing_help && event->type == GDK_BUTTON_PRESS &&
            !is_shift_down())
        GTKmainWin->PopUpHelp("keyspresd");
    return (true);
}


// Redraw callback.
//
static int
redraw_hdlr(GtkWidget*, GdkEvent*, void*)
{           
    GTKhy *h = dynamic_cast<GTKhy*>(HY);
    if (h)
        return (h->hyredraw());
    return (1);
}


// Pop up info about the prompt/reply area if the user points there
// in help mode.  Otherwise, handle button presses, in particular
// selection insertions using button 2.
//
static int
btn_hdlr(GtkWidget*, GdkEvent *event, void *client_data)
{
    if (event->type != GDK_BUTTON_PRESS)
        return (true);
    xic_bag *w = static_cast<xic_bag*>(GTKmainWin);
    GTKhy *h = dynamic_cast<GTKhy*>(HY);
    if (!h)
        return (true);
    if (XM()->doing_help && !(event->button.state & SHIFT_MASK)) {
        w->PopUpHelp("promptline");
        return (true);
    }
    if (h->active == hyOFF || h->txt[h->colmin].type == HY_LTEXT)
        return (true);
    if (event->button.button == 1 || event->button.button == 2)
        h->button_press(event->button.button, (int)event->button.x,
            (int)event->button.y);
    return (true);
}


// Selection handler, supports hypertext transfer
//
void
selection_proc(GtkWidget*, GtkSelectionData *sdata, void*)
{
    if (sdata->length < 0) {
        HY->hycursor(DRAW);
        return;
    }
    if (sdata->type != GDK_SELECTION_TYPE_STRING) {
        XM()->ErrorLog(mh::Internal,
            "Selection conversion failed. not string data.");
        HY->hycursor(DRAW);
        return;
    }

    hprlist *hpl = 0;
    GdkWindow *window = gdk_selection_owner_get(GDK_SELECTION_PRIMARY);
    if (window) {
        GtkWidget *widget;
        gdk_window_get_user_data(window, (void**)&widget);
        if (widget) {
            int code = (long)gtk_object_get_data(GTK_OBJECT(widget),
                "hyexport");
            if (code) {
#if GTK_CHECK_VERSION(1,3,15)
                int start = GTK_OLD_EDITABLE(widget)->selection_start_pos;
#else
                int start = GTK_EDITABLE(widget)->selection_start_pos;
#endif
                // The text is coming from the Property Editor or
                // Property Info pop-up, fetch the original
                // hypertext to insert.
                CDo *odesc;
                Ptxt *p = App->PropertyResolve(code, start, &odesc);
                if (p && p->pdesc)
                    hpl = CurSdesc()->hy_prp_list(odesc, p->pdesc);
            }
        }
    }
    if (!hpl) {
        // might not be 0-terminated?
        char *s = new char[sdata->length + 1];
        memcpy(s, sdata->data, sdata->length);
        s[sdata->length] = 0;
        HY->hyinsert(s, 0);
        delete [] s;
    }
    else {
        HY->hyinsert(0, hpl);
        hpl->free();
    }
}


static void
font_change_hdlr(GtkWidget*, void*, void*)
{
    if (HY) {
        int fw, fh;
        ((GTKhy*)HY)->TextExtent(0, &fw, &fh);
        gtk_drawing_area_size(GTK_DRAWING_AREA(((GTKhy*)HY)->viewport),
            -1, fh + 4);
        HY->hyinit();
    }
}


// Drag data received in main window - open the cell
//
static void
drag_data_received(GtkWidget*, GdkDragContext *context, gint, gint,
    GtkSelectionData *data, guint, guint time)
{
    bool success = false;
    if (data->target == gdk_atom_intern("property", true)) {
        if (HY->active == hyACTIVE) {
            unsigned char *val = data->data + sizeof(int);
            hprlist *hp = new hprlist(CurSdesc(), (char*)val, HyAscii);
            HY->hyinsert(0, hp);
            hp->free();
            success = true;
        }
    }
    else {
        if (data->length >= 0 && data->format == 8) {
            char *src = (char*)data->data;
            if (HY->active == hyACTIVE)
                // if editing, push into prompt line
                HY->hyinsert(src, 0);
            else {
                char *t = strchr(src, '\n');
                if (t) {
                    *t++ = 0;
                    XM()->current_win->Load(src, t, 0);
                }
                else
                    XM()->current_win->Load(src, 0, 0);
            }
            success = true;
        }
    }
    gtk_drag_finish(context, success, false, time);
}
// End of handlers

#endif

//-------------------------------

hyList *QTedit::pe_stores[PE_NUMSTORES];

QTedit *QTedit::instancePtr = 0;

QTedit::QTedit(bool nogr) : QTdraw(XW_TEXT)
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class QTedit already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    pe_disabled = nogr;

//    column = cwid = 0;
//    xpos = ypos = 0;
//    offset = 0;
//    fntwid = 0;
    pe_firstinsert = false;
    pe_indicating = false;
    if (nogr)
        return;

    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setMargin(0);
    hbox->setSpacing(2);

    int w = QTfont::stringWidth(0, FNT_SCREEN);
    int h = QTfont::lineHeight(FNT_SCREEN) + 4;
    pe_keys = new cKeys(0, 0);
    pe_keys->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    hbox->addWidget(pe_keys);

    // Recall button and menu.
    QPushButton *rcl = new QPushButton("R");
    rcl->setAutoDefault(false);
    rcl->setMaximumWidth(7*w + 4);
    rcl->setToolTip(tr("Recall edit string from a register."));
    hbox->addWidget(rcl);

    char buf[4];
    QMenu *rcl_menu = new QMenu();
    for (int i = 0; i < PE_NUMSTORES; i++) {
        buf[0] = '0' + i;
        buf[1] = 0;
        rcl_menu->addAction(buf);
    }
    rcl->setMenu(rcl_menu);

    // Store button and menu.
    QPushButton *sto = new QPushButton("S");
    sto->setAutoDefault(false);
    sto->setMaximumWidth(7*w + 4);
    sto->setToolTip(tr("Save edit string to a register."));
    hbox->addWidget(sto);

    QMenu *sto_menu = new QMenu();
    for (int i = 0; i < PE_NUMSTORES; i++) {
        buf[0] = '0' + i;
        buf[1] = 0;
        sto_menu->addAction(buf);
    }
    sto->setMenu(sto_menu);

    // Long Text button.
    QPushButton *lt = new QPushButton("L");
    lt->setAutoDefault(false);
    lt->setMaximumWidth(6*w + 4);
    lt->setToolTip(tr(
        "Associate a block of text with the label - pop up an editor."));
    hbox->addWidget(lt);

    gd_viewport = draw_if::new_draw_interface(DrawNative, false, this);
    hbox->addWidget(Viewport());

    QFont *font;
    if (FC.getFont(&font, FNT_SCREEN)) {
        gd_viewport->set_font(font);
        sto->setFont(*font);
        rcl->setFont(*font);
        lt->setFont(*font);
    }

    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed(int)), Qt::QueuedConnection);
}


QTedit::~QTedit()
{
    instancePtr = 0;
}


// Private static error exit.
//
void
QTedit::on_null_ptr()
{
    fprintf(stderr, "Singleton class QTedit used before instantiated.\n");
    exit(1);
}


// Flash a message just above the prompt line for a couple of seconds.
//
void
QTedit::flash_msg(const char *msg, ...)
{
    (void)msg;
    /*XXX
    va_list args;
    GtkWidget *popup = gtk_window_new(GTK_WINDOW_POPUP);
    if (!popup)
        return;

    char buf[256];
    va_start(args, msg);
    vsnprintf(buf, 256, msg, args);
    va_end(args);

    GtkWidget *label = gtk_label_new(buf);
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_container_add(GTK_CONTAINER(popup), label);

    QTdev::self()->SetPopupLocation(GRloc(LW_LL), popup,
        QTmainwin::self()->Viewport());
    gtk_window_set_transient_for(GTK_WINDOW(popup),
        GTK_WINDOW(QTmainwin::self()->Shell()));

    gtk_widget_show(popup);

    QTdev::self()->AddTimer(2000, fm_timeout, popup);
    */
}


// As above, but user passes the location.
//
void
QTedit::flash_msg_here(int x, int y, const char *msg, ...)
{
    (void)x;
    (void)y;
    (void)msg;
    /*XXX
    va_list args;
    GtkWidget *popup = gtk_window_new(GTK_WINDOW_POPUP);
    if (!popup)
        return;

    char buf[256];
    va_start(args, msg);
    vsnprintf(buf, 256, msg, args);
    va_end(args);

    GtkWidget *label = gtk_label_new(buf);
    gtk_widget_show(label);
    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
    gtk_container_add(GTK_CONTAINER(popup), label);

    int mwid, mhei;
    gtk_MonitorGeom(QTmainwin::self()->Shell(), 0, 0, &mwid, &mhei);
    GtkRequisition req;
    gtk_widget_get_requisition(popup, &req);
    if (x + req.width > mwid)
        x = mwid - req.width;
    if (y + req.height > mhei)
        y = mhei - req.height;
    gtk_window_move(GTK_WINDOW(popup), x, y);
    gtk_window_set_transient_for(GTK_WINDOW(popup),
        GTK_WINDOW(QTmainwin::self()->Shell()));

    gtk_widget_show(popup);

    QTdev::self()->AddTimer(2000, fm_timeout, popup);
    */
}


// Save text in register 0, called when editing finished.
//
void
QTedit::save_line()
{
    hyList::destroy(pe_stores[0]);
    pe_stores[0] = get_hyList(false);
}


// Return the pixel width of the drawing area.
//
int
QTedit::win_width(bool)
{
    return (gd_viewport->widget()->width());
}


int
QTedit::win_height()
{
//    if (!QTdev::exists() || !QTmainwin::exists())
        return (14);
//    return (pe_hei);
}


// Set the keyboard focus to the main drawing window.
//
void
QTedit::set_focus()
{
//    QTdev::self()->RevertFocus();
}


// Display the R/S/L buttons, hide the keys area while editing.
//
void
QTedit::set_indicate()
{
    /*
    static GdkGC *gc;
    GtkWidget *label = GTKmainWin->keyspressed;
    if (indicating) {
        if (!gc) {
            gc = gdk_gc_new(label->window);
            gdk_gc_copy(gc, label->style->fg_gc[label->state]);
            GdkColor *clr = gtk_PopupColor(POPC_HL1);
            if (clr)
                gdk_gc_set_foreground(gc, clr);
            GtkStyle *style = gtk_style_copy(label->style);
            gtk_widget_set_style(label, style);
        }
        GdkGC *tmpgc = label->style->fg_gc[label->state];
        label->style->fg_gc[label->state] = gc;
        gc = tmpgc;
        gtk_label_set_text(GTK_LABEL(label), "INPUT");
    }
    else {
        GdkGC *tmpgc = label->style->fg_gc[label->state];
        label->style->fg_gc[label->state] = gc;
        gc = tmpgc;
        gtk_label_set_text(GTK_LABEL(label), "");
    }
    */
}


void
QTedit::show_lt_button(bool show)
{
    (void)show;
    /*
    if (!disabled) {
        if (show)
            gtk_widget_show(Lbutton);
        else
            gtk_widget_hide(Lbutton);
    }
    */
}


void
QTedit::get_selection(bool)
{
}


void *
QTedit::setup_backing(bool use_pm)
{
    (void)use_pm;
    return (0);
}


void
QTedit::restore_backing(void *tw)
{
    (void)tw;
}


void
QTedit::init_window()
{
    SetWindowBackground(bg_pixel());
    SetBackground(bg_pixel());
    Clear();
}


bool
QTedit::check_pixmap()
{
    return (true);
}


void
QTedit::init_selection(bool)
{
}


void
QTedit::warp_pointer()
{
    // The pointer move must be in an idle proc, so it runs after
    // prompt line reconfiguration.

//    g_idle_add(warp_ptr, this);
}


void
QTedit::font_changed(int fnum)
{
    if (fnum == FNT_SCREEN) {
        QFont *fnt;
        if (FC.getFont(&fnt, FNT_SCREEN))
            gd_viewport->set_font(fnt);
        //XXX redraw
    }
}

