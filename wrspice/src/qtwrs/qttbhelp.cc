
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "qttbhelp.h"
#include "qtinterf/qttextw.h"
#include "qtinterf/qtfont.h"
#include "qttoolb.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>

void
QTtoolbar::PopUpTBhelp(ShowMode mode, GRobject parent, GRobject call_btn,
    TBH_type type)
{
    if (mode == MODE_OFF) {
        if (!tb_kw_help[type])
            return;
        delete tb_kw_help[type];
        return;
    }
    if (tb_kw_help[type])
        return;
    QTtbHelpDlg *th = new QTtbHelpDlg(parent, call_btn, type);
    tb_kw_help[type] = th;
    if (tb_kw_help_pos[type].x != 0 && tb_kw_help_pos[type].y != 0)
        th->move(tb_kw_help_pos[type].x, tb_kw_help_pos[type].y);
    th->show();
}

char *
QTtoolbar::KeywordsText(GRobject parent)
{
    sLstr lstr;
/* XXX fixme
    if (parent == (GRobject)tb_shell) {
        for (int i = 0; KW.shell(i)->word; i++)
            KW.shell(i)->print(&lstr);
    }
    else if (parent == (GRobject)tb_simdeffs) {
        for (int i = 0; KW.sim(i)->word; i++)
            KW.sim(i)->print(&lstr);
    }
    else if (parent == (GRobject)tb_commands) {
        for (int i = 0; KW.cmds(i)->word; i++)
            KW.cmds(i)->print(&lstr);
    }
    else if (parent == (GRobject)tb_plotdefs) {
        for (int i = 0; KW.plot(i)->word; i++)
            KW.plot(i)->print(&lstr);
    }
    else if (parent == (GRobject)tb_debug) {
        for (int i = 0; KW.debug(i)->word; i++)
            KW.debug(i)->print(&lstr);
    }
    else
*/
        lstr.add("Internal error.");
    return (lstr.string_trim());
}

void
QTtoolbar::KeywordsCleanup(QTtbHelpDlg *dlg)
{
    if (!dlg)
        return;
    TBH_type t = dlg->type();
    tb_kw_help[t] = 0;
    QPoint pt = dlg->mapToGlobal(QPoint(0, 0));
    tb_kw_help_pos[t].x = pt.x();
    tb_kw_help_pos[t].y = pt.y();
}
// End of QTtoolbar functions;


// Dialog to display keyword help lists.  Clicking on the list entries
// calls the main help system.  This is called from the dialogs which
// contain lists of 'set' variables to modify.
//


QTtbHelpDlg::QTtbHelpDlg(GRobject parent, GRobject call_btn, TBH_type type)
{
    th_text = 0;
    th_parent = parent;
    th_caller = call_btn;
    th_type = type;
    th_lx = 0;
    th_ly = 0;

    setWindowTitle(tr("Keyword Help"));
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);

    // label in frame
    //
    QGroupBox *gb = new QGroupBox();
    vbox->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setMargin(2);
    hb->setSpacing(2);

    QLabel *label = new QLabel(tr("Click on entries for more help: "));
    hb->addWidget(label);

    // text area
    //
    th_text = new QTtextEdit();
    vbox->addWidget(th_text);
    th_text->setReadOnly(true);
    th_text->setMouseTracking(true);
    connect(th_text, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));
    QFont *fnt;
    if (FC.getFont(&fnt, FNT_FIXED))
        th_text->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    char *s = TB()->KeywordsText(th_parent);
    th_text->set_chars(s);
    delete [] s;

/* XXX
    // This will provide an arrow cursor.
    g_signal_connect_after(G_OBJECT(th_text), "realize",
        G_CALLBACK(text_realize_proc), 0);
*/

    int wid = 80*QTfont::stringWidth(0, th_text);
    int hei = 12*QTfont::lineHeight(th_text);
//XXX    gtk_window_set_default_size(GTK_WINDOW(th_popup), wid + 8, hei + 20);

    // buttons
    //
    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setMargin(0);
    hbox->setSpacing(2);

    QPushButton *btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));
}


QTtbHelpDlg::~QTtbHelpDlg()
{
    if (th_caller)
        QTdev::Deselect(th_caller);
    TB()->KeywordsCleanup(this);
}


void
QTtbHelpDlg::dismiss_btn_slot()
{
    delete this;
}


void
QTtbHelpDlg::mouse_press_slot(QMouseEvent *ev)
{
    /*
    QTtbHelpDlg *th = (QTtbHelpDlg*)arg;
    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button.button == 1) {
            th->th_lx = (int)event->button.x;
            th->th_ly = (int)event->button.y;
            return (false);
        }
        return (true);
    }
    if (event->type == GDK_BUTTON_RELEASE) {
        if (event->button.button == 1) {
            int x = (int)event->button.x;
            int y = (int)event->button.y;
            if (abs(x - th->th_lx) <= 4 && abs(y - th->th_ly) <= 4)
                th->select(caller, th->th_lx, th->th_ly);
            return (false);
        }
    }
    */
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    const char *str = lstring::copy(
        th_text->toPlainText().toLatin1().constData());
    int x = ev->x();
    int y = ev->y();
    QTextCursor cur = th_text->cursorForPosition(QPoint(x, y));
    int pos = cur.position();

    const char *lineptr = str;
    for (int i = 0; i <= pos; i++) {
        if (str[i] == '\n') {
            if (i == pos) {
                // Clicked to right of line.
                th_text->select_range(0, 0);
                delete [] str;
                return;
            }
            lineptr = str + i+1;
        }
    }

    // find first word
    while (isspace(*lineptr) && *lineptr != '\n')
        lineptr++;
    if (*lineptr == 0 || *lineptr == '\n') {
        th_text->select_range(0, 0);
        delete [] str;
        return;
    }

    int start = lineptr - str;
    char buf[64];
    char *s = buf;
    while (!isspace(*lineptr))
        *s++ = *lineptr++;
    *s = 0;
    int end = lineptr - str;

    th_text->select_range(start, end);
#ifdef HAVE_MOZY
    HLP()->word(buf);
#endif
    delete [] str;
}


void
QTtbHelpDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (FC.getFont(&fnt, fnum)) {
            th_text->setFont(*fnt);
            //XXX redraw
        }
    }
}
