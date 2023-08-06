
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "qtinterf.h"
#include "qttext.h"
#include "qtfont.h"
#include "qtinput.h"
#include "qtmsg.h"
#include "miscutil/filestat.h"

#include <QAction>
#include <QGroupBox>
#include <QLayout>
#include <QTextEdit>
#include <QPushButton>


char *QTtextDlg::tx_errlog;

QTtextDlg::QTtextDlg(QTbag *owner, const char *message_str, PuType which,
    STYtype sty) : QDialog(owner ? owner->Shell() : 0)
{
    p_parent = owner;
    tx_which = which;
    tx_style = sty;
    tx_desens = false;

    if (owner)
        owner->MonitorAdd(this);

    const char *t;
    if (tx_which == PuWarn)
        t = "Warning";
    else if (tx_which == PuErr || tx_which == PuErrAlso)
        t = "ERROR";
    else if (tx_which == PuInfo || tx_which == PuInfo2 ||
            tx_which == PuHTML) {
        t = "Info";
    }
    else
        t = "ERROR";
    setWindowTitle(tr(t));
    setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(2);
    vbox->setSpacing(2);
    QGroupBox *gb = new QGroupBox();
    vbox->addWidget(gb);

    QVBoxLayout *vb = new QVBoxLayout(gb);
    vb->setMargin(2);
    vb->setSpacing(2);

    tx_tbox = new QTextEdit();
    tx_tbox->setReadOnly(true);
    vb->addWidget(tx_tbox);

    if (sty == STY_FIXED) {
        QFont *f;
        if (FC.getFont(&f, FNT_FIXED)) {
            tx_tbox->setCurrentFont(*f);
            tx_tbox->setFont(*f);
        }
    }
    setText(message_str);

    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setMargin(0);
    hbox->setSpacing(2);

    if (tx_style != STY_HTML) {
        tx_save = new QPushButton(tr("Save Text "));
        tx_save->setCheckable(true);
        hbox->addWidget(tx_save);
        connect(tx_save, SIGNAL(toggled(bool)),
            this, SLOT(save_btn_slot(bool)));
    }
    if ((tx_which == PuErr || tx_which == PuErrAlso) &&
            tx_errlog && p_parent) {
        tx_showlog = new QPushButton(tr("Show Error Log"));
        hbox->addWidget(tx_showlog);
        connect(tx_showlog, SIGNAL(clicked()),
            this, SLOT(showlog_btn_slot()));
    }
    if (tx_which == PuInfo2) {
        QPushButton *btn = new QPushButton(tr("Help"));
        hbox->addWidget(btn);
        connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

        tx_activate = new QPushButton(tr("Activate"));
        tx_activate->setCheckable(true);
        tx_activate->setChecked(true);
        hbox->addWidget(tx_activate);
        connect(tx_activate, SIGNAL(toggled(bool)),
            this, SLOT(activate_btn_slot(bool)));
    }

    tx_cancel = new QPushButton(tr("Dismiss"));
    hbox->addWidget(tx_cancel);
    connect(tx_cancel, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));
}


QTtextDlg::~QTtextDlg()
{
    /* XXX
    if (tx_activate && QTdev::GetStatus(tx_activate))
        gtk_button_clicked(GTK_BUTTON(pw_btn));
    */
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (owner)
            owner->ClearPopup(this);
    }
    if (tx_save_pop)
        tx_save_pop->popdown();
    if (tx_msg_pop)
        tx_msg_pop->popdown();
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller)
        QTdev::Deselect(p_caller);
}


// GRpopup override
//
void
QTtextDlg::popdown()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    deleteLater();
}


void
QTtextDlg::setTitle(const char *title)
{
    setWindowTitle(title);
}


void
QTtextDlg::setText(const char *message_str)
{
    if (tx_style == STY_HTML)
        tx_tbox->setHtml(message_str);
    else
        tx_tbox->setPlainText(message_str);
}


// GRtextPopup override
//
bool
QTtextDlg::get_btn2_state()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return (false);
    }
    if (tx_activate)
        return (QTdev::GetStatus(tx_activate));
    return (false);
}


// GRtextPopup override
//
void
QTtextDlg::set_btn2_state(bool state)
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    if (tx_activate)
        QTdev::SetStatus(tx_activate, state);
}


// Attempt to reuse an existing widget to show message_str.  If not
// possible, return false, otherwise return true;
//
bool
QTtextDlg::update(const char *message_str)
{
    /* XXX
    if (tx_style == STY_HTML) {
#ifdef HAVE_MOZY
        if (pw_viewer) {
            if (has_body_tag(message_str))
                pw_viewer->set_source(message_str);
            else {
                sLstr lstr;
                lstr.add("<body bgcolor=\"");
                lstr.add(HTML_TEXT_BG);
                lstr.add("\">");
                lstr.add(message_str);
                lstr.add("</body>");
                pw_viewer->set_source(lstr.string());
            }
            return (true);
        }
#endif
    }
    */
    if (tx_tbox) {
        tx_tbox->setText(message_str);
            return (true);
    }
    return (false);
}


// Private static.
// Callback for the save file name pop-up.
//
ESret
QTtextDlg::tx_save_cb(const char *string, void *arg)
{
    QTtextDlg *txtp = (QTtextDlg*)arg;
    if (string) {
        if (!filestat::create_bak(string)) {
            txtp->tx_save_pop->update(
                "Error backing up existing file, try again", 0);
            return (ESTR_IGN);
        }
        FILE *fp = fopen(string, "w");
        if (!fp) {
            txtp->tx_save_pop->update("Error opening file, try again", 0);
            return (ESTR_IGN);
        }
        QByteArray tx_ba = txtp->tx_tbox->toPlainText().toLatin1();
        const char *txt = tx_ba.constData();
        if (txt) {
            unsigned int len = strlen(txt);
            if (len) {
                if (fwrite(txt, 1, len, fp) != len) {
                    txtp->tx_save_pop->update("Write failed, try again", 0);
                    fclose(fp);
                    return (ESTR_IGN);
                }
            }
        }
        fclose(fp);

        if (txtp->tx_msg_pop)
            txtp->tx_msg_pop->popdown();
        txtp->tx_msg_pop = new QTmsgDlg(0, "Text saved in file.", false);
        txtp->tx_msg_pop->register_usrptr((void**)&txtp->tx_msg_pop);
        QTdev::self()->SetPopupLocation(GRloc(), txtp->tx_msg_pop, txtp);
        txtp->tx_msg_pop->set_visible(true);

        QTdev::self()->AddTimer(2000, tx_timeout, txtp);
    }
    return (ESTR_DN);
}


int
QTtextDlg::tx_timeout(void *arg)
{
    // This just cancels the message after a while.
    QTtextDlg *txtp = (QTtextDlg*)arg;
    if (txtp->tx_msg_pop)
        txtp->tx_msg_pop->popdown();
    return (0);
}


void
QTtextDlg::save_btn_slot(bool state)
{
    if (!state)
        return;
    if (tx_save_pop)
        return;
    tx_save_pop = new QTledDlg(0,
        "Enter path to file for saved text:", "", "Save", false);
    tx_save_pop->register_caller(tx_save, false, true);
    tx_save_pop->register_callback(&tx_save_cb);
    tx_save_pop->set_callback_arg(this);
    tx_save_pop->register_usrptr((void**)&tx_save_pop);

    QTdev::self()->SetPopupLocation(GRloc(), tx_save_pop, this);
    tx_save_pop->set_visible(true);
}


void
QTtextDlg::showlog_btn_slot()
{
    if (!p_parent || !tx_errlog)
        return;
    p_parent->PopUpFileBrowser(tx_errlog);
}


void
QTtextDlg::help_btn_slot()
{
    if (p_callback)
        (*p_callback)(false, GRtextPopupHelp);
}


void
QTtextDlg::activate_btn_slot(bool state)
{
    if (p_callback) {
        if ((*p_callback)(state, p_cb_arg))
            popdown();
    }
}


void
QTtextDlg::dismiss_btn_slot()
{
    deleteLater();
}
