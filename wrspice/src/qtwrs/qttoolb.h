
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

#ifndef QTTOOLB_H
#define QTTOOLB_H

#include "qtinterf/qtinterf.h"
#include "toolbar.h"


enum tid_id { tid_toolbar, tid_bug, tid_font, tid_files, tid_circuits,
    tid_plots, tid_plotdefs, tid_colors, tid_vectors, tid_variables,
    tid_shell, tid_simdefs, tid_commands, tid_trace, tid_debug, tid_END };

extern inline class QTtoolbar *TB();

// Keep track of the active popup widgets
//
class QTtoolbar : public sToolbar, public QTbag
{
    static QTtoolbar *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline QTtoolbar *TB() { return (QTtoolbar::ptr()); }

    // save a tool state
    struct tbent_t
    {
        tbent_t(const char *n = 0, int i = -1)
        {
            name = n;
            id = i;
            active = false;
            x = y = 0;
        }

        const char *name;
        int id;
        bool active;
        int x, y;
    };

    struct tbpoint_t
    {
        int x, y;
    };

    // gtktoolb.cc
    QTtoolbar();

    // Toolbar virtual interface.
    // -------------------------

    // timer/idle utilities
    int RegisterIdleProc(int(*)(void*), void*);
    bool RemoveIdleProc(int);
    int RegisterTimeoutProc(int, int(*)(void*), void*);
    bool RemoveTimeoutProc(int);
    void RegisterBigForeignWindow(unsigned int);

    // command defaults dialog
    // gtkcmds.cc
    void PopUpCmdConfig(ShowMode, int, int);

    // color control dialog
    // gtkcolor.cc
    void PopUpColors(ShowMode, int, int);
    void UpdateColors(const char*);
    void LoadResourceColors();

    // debugging dialog
    // gtkdebug.cc
    void PopUpDebugDefs(ShowMode, int, int);

    // plot defaults dialog
    // gtkpldef.cc
    void PopUpPlotDefs(ShowMode, int, int);

    // shell defaults dialog
    // gtkshell.cc
    void PopUpShellDefs(ShowMode, int, int);

    // simulation defaults dialog
    // gtksim.cc
    void PopUpSimDefs(ShowMode, int, int);

    // misc listing panels and dialogs
    // gtkfte.cc
    void SuppressUpdate(bool);
    void PopUpPlots(ShowMode, int, int);
    void UpdatePlots(int);
    void PopUpVectors(ShowMode, int, int);
    void UpdateVectors(int);
    void PopUpCircuits(ShowMode, int, int);
    void UpdateCircuits();
    void PopUpFiles(ShowMode, int, int);
    void UpdateFiles();
    void PopUpTrace(ShowMode, int, int);
    void UpdateTrace();
    void PopUpVariables(ShowMode, int, int);
    void UpdateVariables();

    // main window and a few more
    void PopUpToolbar(ShowMode, int, int);
    void PopUpBugRpt(ShowMode, int, int);
    void PopUpFont(ShowMode, int, int);
    void PopUpTBhelp(ShowMode, GRobject, GRobject, TBH_type);
    void PopUpSpiceErr(bool, const char*);
    void PopUpSpiceMessage(const char*, int, int);
    void UpdateMain(ResUpdType);
    void CloseGraphicsConnection();

    void PopUpInfo(const char *msg);  // XXX clash!
    void PopUpNotes();
    // --------------------------------
    // End of Toolbar virtual overrides.

    void Toolbar();
//XXX    void RevertFocus(GtkWidget*);

    // Return the entry of the named tool, if found.
    //
    static tbent_t *FindEnt(const char *str)
    {
        if (str) {
            for (tbent_t *tb = tb_entries; tb->name; tb++) {
                if (!strcmp(str, tb->name))
                    return (tb);
            }
        }
        return (0);
    }

    // Register that a tool is active, or not.
    //
    static void SetActive(tid_id id, bool state)
    {
        tb_entries[id].active = state;
    }

    static void SetLoc(tid_id, QWidget*);
    static void FixLoc(int*, int*);
    static char *ConfigString();

/*
    // gtkcolor.cc
    const char *XRMgetFromDb(const char*);
*/


/*XXX
    GtkWidget *TextPop(int, int, const char*, const char*,
        const char*, void(*)(GtkWidget*, int, int), const char**, int,
        void(*)(GtkWidget*, void*));
    void RUsure(GtkWidget*, void(*)());
*/

    bool Saved()            { return (tb_saved); }
    void SetSaved(bool b)   { tb_saved = b; }

    static tbent_t *entries(int ix)
    {
        if (ix >= 0 && ix < tid_END)
            return (tb_entries + ix);
        return (0);
    }

private:
    static void tb_mail_destroy_cb(GReditPopup*);
    static void tb_font_cb(const char*, const char*, void*);

    GReditPopup *tb_mailer;
    QTfontDlg   *tb_fontsel;

    QWidget     *tb_kw_help[TBH_end];
    tbpoint_t   tb_kw_help_pos[TBH_end];

    static tbent_t tb_entries[];

    bool        tb_suppress_update;
    bool        tb_saved;

    static QTtoolbar *instancePtr;
};

#endif

