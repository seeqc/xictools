
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

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "frontend.h"
#include "fteparse.h"
#include "ftedebug.h"
#include "ftemeas.h"
#include "outplot.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "toolbar.h"
#include "outdata.h"
#include "device.h"
#include "rundesc.h"
#include "spnumber/hash.h"
#include "spnumber/spnumber.h"


//
// The stop command and related, general support for "debugs".
//

// keywords
const char *kw_stop   = "stop";
const char *kw_after  = "after";
const char *kw_at     = "at";
const char *kw_before = "before";
const char *kw_when   = "when";
const char *kw_active = "active";
const char *kw_inactive = "inactive";

// This is for debugs entered interactively
sDebug DB;


// Set a breakpoint. Possible commands are:
//  stop after|at|before n
//  stop when expr
//
// If more than one is given on a command line, then this is a conjunction.
//
void
CommandTab::com_stop(wordlist *wl)
{
    sDbComm *d = 0, *thisone = 0;
    while (wl) {
        if (thisone == 0)
            thisone = d = new sDbComm;
        else {
            d->set_also(new sDbComm);
            d = d->also();
        }

        // Figure out what the first condition is.
        if (lstring::eq(wl->wl_word, kw_after) ||
                lstring::eq(wl->wl_word, kw_at) ||
                lstring::eq(wl->wl_word, kw_before)) {
            if (lstring::eq(wl->wl_word, kw_after))
                d->set_type(DB_STOPAFTER);
            else if (lstring::eq(wl->wl_word, kw_at))
                d->set_type(DB_STOPAT);
            else
                d->set_type(DB_STOPBEFORE);
            int i = 0;
            wl = wl->wl_next;
            if (wl) {
                if (wl->wl_word) {
                    for (char *s = wl->wl_word; *s; s++)
                        if (!isdigit(*s))
                            goto bad;
                    i = atoi(wl->wl_word);
                }
                wl = wl->wl_next;
            }
            d->set_point(i);
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_when)) {
            d->set_type(DB_STOPWHEN);
            wl = wl->wl_next;
            if (!wl)
                goto bad;
            wordlist *wx;
            int i;
            for (i = 0, wx = wl; wx; wx = wx->wl_next) {
                if (lstring::eq(wx->wl_word, kw_after) ||
                        lstring::eq(wx->wl_word, kw_at) ||
                        lstring::eq(wx->wl_word, kw_before) ||
                        lstring::eq(wx->wl_word, kw_when))
                    break;
                i += strlen(wx->wl_word) + 1;
            }
            i++;
            char *string = new char[i];
            char *s = lstring::stpcpy(string, wl->wl_word);
            for (wl = wl->wl_next; wl; wl = wl->wl_next) {
                if (lstring::eq(wl->wl_word, kw_after) ||
                        lstring::eq(wl->wl_word, kw_at) ||
                        lstring::eq(wl->wl_word, kw_before) ||
                        lstring::eq(wl->wl_word, kw_when))
                    break;
                char *t = wl->wl_word;
                *s++ = ' ';
                while (*t)
                    *s++ = *t++;
            }
            *s = '\0';
            CP.Unquote(string);
            d->set_string(string);
        }
        else
            goto bad;
    }
    if (thisone) {
        thisone->set_number(DB.new_count());
        thisone->set_active(true);
        if (CP.GetFlag(CP_INTERACTIVE) || !Sp.CurCircuit()) {
            if (DB.stops()) {
                for (d = DB.stops(); d->next(); d = d->next())
                    ;
                d->set_next(thisone);
            }
            else
                DB.set_stops(thisone);
        }
        else {
            if (!Sp.CurCircuit()->debugs())
                Sp.CurCircuit()->set_debugs(new sDebug);
            sDebug *db = Sp.CurCircuit()->debugs();
            if (db->stops()) {
                for (d = db->stops(); d->next(); d = d->next())
                    ;
                d->set_next(thisone);
            }
            else
                db->set_stops(thisone);
        }
    }
    ToolBar()->UpdateTrace();
    return;

bad:
    GRpkgIf()->ErrPrintf(ET_ERROR, "syntax.\n");
}


// Step a number of output increments.
//
void
CommandTab::com_step(wordlist *wl)
{
    int n;
    if (wl)
        n = atoi(wl->wl_word);
    else
        n = 1;
    DB.set_step_count(n);
    DB.set_num_steps(n);
    Sp.Simulate(SIMresume, 0);
}


// Print out the currently active breakpoints and traces.
//
void
CommandTab::com_status(wordlist*)
{
    Sp.DebugStatus(true);
}


// Delete breakpoints and traces. Usage is:
//   delete [[in]active][all | stop | trace | iplot | save | number] ...
// With inactive true (args to right of keyword), the debug is
// cleared only if it is inactive.
//
void
CommandTab::com_delete(wordlist *wl)
{
    if (!wl) {
        sDbComm *d = DB.stops();
        // find the earlist debug
        if (!d || (DB.traces() && d->number() > DB.traces()->number()))
            d = DB.traces();
        if (!d || (DB.iplots() && d->number() > DB.iplots()->number()))
            d = DB.iplots();
        if (!d || (DB.saves() && d->number() > DB.saves()->number()))
            d = DB.saves();
        if (Sp.CurCircuit() && Sp.CurCircuit()->debugs()) {
            sDebug *db = Sp.CurCircuit()->debugs();
            if (!d || (db->stops() && d->number() > db->stops()->number()))
                d = db->stops();
            if (!d || (db->traces() && d->number() > db->traces()->number()))
                d = db->traces();
            if (!d || (db->iplots() && d->number() > db->iplots()->number()))
                d = db->iplots();
            if (!d || (db->saves() && d->number() > db->saves()->number()))
                d = db->saves();
        }
        if (!d) {
            TTY.printf("No debugs are in effect.\n");
            return;
        }
        d->print(0);
        if (CP.GetFlag(CP_NOTTYIO))
            return;
        if (!TTY.prompt_for_yn(true, "delete [y]? "))
            return;
        if (d == DB.stops())
            DB.set_stops(d->next());
        else if (d == DB.traces())
            DB.set_traces(d->next());
        else if (d == DB.iplots())
            DB.set_iplots(d->next());
        else if (d == DB.saves())
            DB.set_saves(d->next());
        else if (Sp.CurCircuit() && Sp.CurCircuit()->debugs()) {
            sDebug *db = Sp.CurCircuit()->debugs();
            if (d == db->stops())
                db->set_stops(d->next());
            else if (d == db->traces())
                db->set_traces(d->next());
            else if (d == db->iplots())
                db->set_iplots(d->next());
            else if (d == db->saves())
                db->set_saves(d->next());
        }
        ToolBar()->UpdateTrace();
        sDbComm::destroy(d);
        return;
    }
    bool inactive = false;
    for (; wl; wl = wl->wl_next) {
        if (lstring::eq(wl->wl_word, kw_inactive)) {
            inactive = true;
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_active)) {
            inactive = false;
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_all)) {
            Sp.DeleteDbg(true, true, true, true, inactive, -1);
            return;
        }
        if (lstring::eq(wl->wl_word, kw_stop)) {
            Sp.DeleteDbg(true, false, false, false, inactive, -1);
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_trace)) {
            Sp.DeleteDbg(false, true, false, false, inactive, -1);
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_iplot)) {
            Sp.DeleteDbg(false, false, true, false, inactive, -1);
            continue;
        }
        if (lstring::eq(wl->wl_word, kw_save)) {
            Sp.DeleteDbg(false, false, false, true, inactive, -1);
            continue;
        }

        char *s;
        // look for range n1-n2
        if ((s = strchr(wl->wl_word, '-')) != 0) {
            if (s == wl->wl_word || !*(s+1)) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "badly formed range %s.\n",
                    wl->wl_word);
                continue;
            }
            char *t;
            for (t = wl->wl_word; *t; t++) {
                if (!isdigit(*t) && *t != '-') {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "badly formed range %s.\n",
                        wl->wl_word);
                    break;
                }
            }
            if (*t)
                continue;
            int n1 = atoi(wl->wl_word);
            s++;
            int n2 = atoi(s);
            if (n1 > n2) {
                int nt = n1;
                n1 = n2;
                n2 = nt;
            }
            while (n1 <= n2) {
                Sp.DeleteDbg(true, true, true, true, inactive, n1);
                n1++;
            }
            continue;
        }
                
        for (s = wl->wl_word; *s; s++) {
            if (!isdigit(*s)) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "%s isn't a debug number.\n",
                    wl->wl_word);
                break;
            }
        }
        if (*s)
            continue;
        int i = atoi(wl->wl_word);
        Sp.DeleteDbg(true, true, true, true, inactive, i);
    }
    ToolBar()->UpdateTrace();
}
// End of CommandTab functions.


void
IFsimulator::SetDbgActive(int dbnum, bool state)
{
    sDbComm *d;
    for (d = DB.stops(); d; d = d->next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }
    for (d = DB.traces(); d; d = d->next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }
    for (d = DB.iplots(); d; d = d->next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }
    for (d = DB.saves(); d; d = d->next()) {
        if (d->number() == dbnum) {
            d->set_active(state);
            return;
        }
    }
    if (Sp.CurCircuit() && Sp.CurCircuit()->debugs()) {
        sDebug *db = Sp.CurCircuit()->debugs();
        for (d = db->stops(); d; d = d->next()) {
            if (d->number() == dbnum) {
                d->set_active(state);
                return;
            }
        }
        for (d = db->traces(); d; d = d->next()) {
            if (d->number() == dbnum) {
                d->set_active(state);
                return;
            }
        }
        for (d = db->iplots(); d; d = d->next()) {
            if (d->number() == dbnum) {
                d->set_active(state);
                return;
            }
        }
        for (d = db->saves(); d; d = d->next()) {
            if (d->number() == dbnum) {
                d->set_active(state);
                return;
            }
        }
    }
}


char *
IFsimulator::DebugStatus(bool tofp)
{
    const char *msg = "No debugs are in effect.\n";
    if (!DB.isset() && (!Sp.CurCircuit() || !Sp.CurCircuit()->debugs() ||
            !Sp.CurCircuit()->debugs()->isset())) {
        if (tofp) {
            TTY.send(msg);
            return (0);
        }
        else
            return (lstring::copy(msg));
    }
    char *s = 0, **t;
    if (tofp)
        t = 0;
    else
        t = &s;
    sDbComm *d;
    sDebug *db = 0;
    if (Sp.CurCircuit() && Sp.CurCircuit()->debugs())
        db = Sp.CurCircuit()->debugs();
    for (d = DB.stops(); d; d = d->next())
        d->print(t);
    if (db)
        for (d = db->stops(); d; d = d->next())
            d->print(t);
    for (d = DB.traces(); d; d = d->next())
        d->print(t);
    if (db)
        for (d = db->traces(); d; d = d->next())
            d->print(t);
    for (d = DB.iplots(); d; d = d->next())
        d->print(t);
    if (db)
        for (d = db->iplots(); d; d = d->next())
            d->print(t);
    for (d = DB.saves(); d; d = d->next())
        d->print(t);
    if (db)
        for (d = db->saves(); d; d = d->next())
            d->print(t);
    return (s);
}


void
IFsimulator::DeleteDbg(bool stop, bool trace, bool iplot, bool save,
    bool inactive, int num)
{
    sDbComm *d, *dlast, *dnext;
    if (stop) {
        dlast = 0;
        for (d = DB.stops(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    DB.set_stops(dnext);
                else
                    dlast->set_next(dnext);
                sDbComm::destroy(d);
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (Sp.CurCircuit() && Sp.CurCircuit()->debugs()) {
            sDebug *db = Sp.CurCircuit()->debugs();
            dlast = 0;
            for (d = db->stops(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db->set_stops(dnext);
                    else
                        dlast->set_next(dnext);
                    sDbComm::destroy(d);
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
    if (trace) {
        dlast = 0;
        for (d = DB.traces(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    DB.set_traces(dnext);
                else
                    dlast->set_next(dnext);
                sDbComm::destroy(d);
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (Sp.CurCircuit() && Sp.CurCircuit()->debugs()) {
            sDebug *db = Sp.CurCircuit()->debugs();
            dlast = 0;
            for (d = db->traces(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db->set_traces(dnext);
                    else
                        dlast->set_next(dnext);
                    sDbComm::destroy(d);
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
    if (iplot) {
        dlast = 0;
        for (d = DB.iplots(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    DB.set_iplots(dnext);
                else
                    dlast->set_next(dnext);
                sDbComm::destroy(d);
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (Sp.CurCircuit() && Sp.CurCircuit()->debugs()) {
            sDebug *db = Sp.CurCircuit()->debugs();
            dlast = 0;
            for (d = db->iplots(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db->set_iplots(dnext);
                    else
                        dlast->set_next(dnext);
                    sDbComm::destroy(d);
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
    if (save) {
        dlast = 0;
        for (d = DB.saves(); d; d = dnext) {
            dnext = d->next();
            if ((num < 0 || num == d->number()) &&
                    (!inactive || !d->active())) {
                if (!dlast)
                    DB.set_saves(dnext);
                else
                    dlast->set_next(dnext);
                sDbComm::destroy(d);
                if (num >= 0)
                    return;
                continue;
            }
            dlast = d;
        }
        if (Sp.CurCircuit() && Sp.CurCircuit()->debugs()) {
            sDebug *db = Sp.CurCircuit()->debugs();
            dlast = 0;
            for (d = db->saves(); d; d = dnext) {
                dnext = d->next();
                if ((num < 0 || num == d->number()) &&
                        (!inactive || !d->active())) {
                    if (!dlast)
                        db->set_saves(dnext);
                    else
                        dlast->set_next(dnext);
                    sDbComm::destroy(d);
                    if (num >= 0)
                        return;
                    continue;
                }
                dlast = d;
            }
        }
    }
}
// End of IFsimulator functions.

void
IFoutput::initDebugs(sRunDesc *run)
{
    if (!run || !run->circuit())
        return;
    sDebug *db = run->circuit()->debugs();
    sDbComm *d, *dt;
    // called at beginning of run
    if (DB.step_count() != DB.num_steps()) {
        // left over from last run
        DB.set_step_count(0);
        DB.set_num_steps(0);
    }
    for (d = DB.stops(); d; d = d->next()) {
        for (dt = d; dt; dt = dt->also()) {
            if (dt->type() == DB_STOPWHEN)
                dt->set_point(0);
        }
    }
    for (dt = DB.traces(); dt; dt = dt->next())
        dt->set_point(0);
    for (dt = DB.iplots(); dt; dt = dt->next()) {
        if (dt->type() == DB_DEADIPLOT) {
            // user killed the window
            if (dt->graphid())
                GP.DestroyGraph(dt->graphid());
            dt->set_type(DB_IPLOT);
        }
        if (run->check() && run->check()->out_mode == OutcCheckSeg)
            dt->set_reuseid(dt->graphid());
        if (!(run->check() && (run->check()->out_mode == OutcCheckMulti ||
                run->check()->out_mode == OutcLoop))) {
            dt->set_point(0);
            dt->set_graphid(0);
        }
    }
    if (db) {
        for (d = db->stops(); d; d = d->next()) {
            for (dt = d; dt; dt = dt->also()) {
                if (dt->type() == DB_STOPWHEN)
                    dt->set_point(0);
            }
        }
        for (dt = db->traces(); dt; dt = dt->next())
            dt->set_point(0);
        for (dt = db->iplots(); dt; dt = dt->next()) {
            if (dt->type() == DB_DEADIPLOT) {
                // user killed the window
                if (dt->graphid())
                    GP.DestroyGraph(dt->graphid());
                dt->set_type(DB_IPLOT);
            }
            if (run->check() && run->check()->out_mode == OutcCheckSeg)
                dt->set_reuseid(dt->graphid());
            if (!(run->check() && (run->check()->out_mode == OutcCheckMulti ||
                    run->check()->out_mode == OutcLoop))) {
                dt->set_point(0);
                dt->set_graphid(0);
            }
        }
    }
}


// The output functions call this function to update the debugs, and
// to see if the run should continue.  If it returns true, then the
// run should continue.  This should be called with point = -1 at the
// end of analysis.
//
bool
IFoutput::breakPtCheck(sRunDesc *run)
{
    if (!run)
        return (false);
    if (run->pointCount() <= 0)
        return (true);
    sDebug *db = 0;
    if (run->circuit() && run->circuit()->debugs())
        db = run->circuit()->debugs();
    bool nohalt = true;
    if (DB.traces() || DB.stops() || (db && (db->traces() || db->stops()))) {
        bool tflag = true;
        run->scalarizeVecs();
        sDbComm *d;
        for (d = DB.traces(); d; d = d->next())
            d->print_trace(run->runPlot(), &tflag, run->pointCount());
        if (db) {
            for (d = db->traces(); d; d = d->next())
                d->print_trace(run->runPlot(), &tflag, run->pointCount());
        }
        for (d = DB.stops(); d; d = d->next()) {
            if (d->should_stop(run->pointCount())) {
                bool need_pr = TTY.is_tty() && CP.GetFlag(CP_WAITING);
                TTY.printf("%-2d: condition met: stop ", d->number());
                d->printcond(0);
                nohalt = false;
                if (need_pr)
                    CP.Prompt();
            }
        }
        if (db) {
            for (d = db->stops(); d; d = d->next()) {
                if (d->should_stop(run->pointCount())) {
                    bool need_pr = TTY.is_tty() && CP.GetFlag(CP_WAITING);
                    TTY.printf("%-2d: condition met: stop ", d->number());
                    d->printcond(0);
                    nohalt = false;
                    if (need_pr)
                        CP.Prompt();
                }
            }
        }
        run->unscalarizeVecs();
    }
    if (DB.iplots() || (db && db->iplots())) {
        sDataVec *xs = run->runPlot()->scale();
        if (xs) {
            int len = xs->length();
            if (len >= IPOINTMIN || (xs->flags() & VF_ROLLOVER)) {
                bool doneone = false;
                for (sDbComm *d = DB.iplots(); d; d = d->next()) {
                    if (d->type() == DB_IPLOT) {
                        OP.iplot(d, run);
                        if (GRpkgIf()->CurDev() &&
                                GRpkgIf()->CurDev()->devtype == GRfullScreen) {
                            doneone = true;
                            break;
                        }
                    }
                }
                if (run->circuit()->debugs() && !doneone) {
                    db = run->circuit()->debugs();
                    for (sDbComm *d = db->iplots(); d; d = d->next()) {
                        if (d->type() == DB_IPLOT) {
                            OP.iplot(d, run);
                            if (GRpkgIf()->CurDev() &&
                                    GRpkgIf()->CurDev()->devtype ==
                                    GRfullScreen)
                                break;
                        }
                    }
                }
            }
        }
    }

    if (DB.step_count() > 0 && DB.dec_step_count() == 0) {
        if (DB.num_steps() > 1) {
            bool need_pr = TTY.is_tty() && CP.GetFlag(CP_WAITING);
            TTY.printf("Stopped after %d steps.\n", DB.num_steps());
            if (need_pr)
                CP.Prompt();
        }
        return (false);
    }
    return (nohalt);
}
// End of IFoutput functions.


// Static function.
void
sDbComm::destroy(sDbComm *dd)
{
    while (dd) {
        sDbComm *dx = dd;
        dd = dd->db_also;
        if (dx->db_type == DB_DEADIPLOT && dx->db_graphid) {
            // User killed the window.
            GP.DestroyGraph(dx->db_graphid);
        }
        delete dx;
    }
}


namespace { const char *Msg = "can't evaluate %s.\n"; }

bool
sDbComm::istrue()
{
    if (db_point < 0)
        return (true);

    wordlist *wl = CP.LexStringSub(db_string);
    if (!wl) {
        GRpkgIf()->ErrPrintf(ET_ERROR, Msg, db_string);
        db_point = -1;
        return (true);
    }
    char *str = wordlist::flatten(wl);
    wordlist::destroy(wl);

    const char *t = str;
    pnode *p = Sp.GetPnode(&t, true);
    delete [] str;
    sDataVec *v = 0;
    if (p) {
        v = Sp.Evaluate(p);
        delete p;
    }
    if (!v) {
        GRpkgIf()->ErrPrintf(ET_ERROR, Msg, db_string);
        db_point = -1;
        return (true);
    }
    if (v->realval(0) != 0.0 || v->imagval(0) != 0.0)
        return (true);
    return (false);
}


bool
sDbComm::should_stop(int pnt)
{
    if (!db_active)
        return (false);
    bool when = true;
    bool after = true;
    for (sDbComm *dt = this; dt; dt = dt->db_also) {
        if (dt->db_type == DB_STOPAFTER) {
            if (pnt < dt->db_point) {
                after = false;
                break;
            }
        }
        else if (dt->db_type == DB_STOPAT) {
            if (pnt != dt->db_point) {
                after = false;
                break;
            }
        }
        else if (dt->db_type == DB_STOPBEFORE) {
            if (pnt > dt->db_point) {
                after = false;
                break;
            }
        }
        else if (dt->db_type == DB_STOPWHEN) {
            if (!dt->istrue()) {
                when = false;
                break;
            }
        }
    }
    return (when && after);
}


void
sDbComm::print(char **retstr)
{
    char buf[BSIZE_SP];
    sDbComm *dc = 0;
    const char *msg0 = "%c %-4d %s %s\n";
    const char *msg1 = "%c %-4d stop";
    const char *msg2 = "%c %-4d %s %s";

    switch (db_type) {
    case DB_SAVE:
        if (!retstr)
            TTY.printf(msg0, db_active ? ' ' : 'I', db_number,
                kw_save,  db_string);
        else {
            sprintf(buf, msg0, db_active ? ' ' : 'I', db_number,
                kw_save,  db_string);
            *retstr = lstring::build_str(*retstr, buf);
        }
        break;

    case DB_TRACE:
        if (!retstr)
            TTY.printf(msg0, db_active ? ' ' : 'I', db_number,
                kw_trace,  db_string ? db_string : "");
        else {
            sprintf(buf, msg0, db_active ? ' ' : 'I', db_number,
                kw_trace,  db_string ? db_string : "");
            *retstr = lstring::build_str(*retstr, buf);
        }
        break;

    case DB_STOPWHEN:
    case DB_STOPAFTER:
    case DB_STOPAT:
    case DB_STOPBEFORE:
        if (!retstr) {
            TTY.printf(msg1, db_active ? ' ' : 'I', db_number);
            printcond(0);
        }
        else {
            sprintf(buf, msg1, db_active ? ' ' : 'I', db_number);
            *retstr = lstring::build_str(*retstr, buf);
            printcond(retstr);
        }
        break;

    case DB_IPLOT:
    case DB_DEADIPLOT:
        if (!retstr) {
            TTY.printf(msg2, db_active ? ' ' : 'I', db_number,
                kw_iplot, db_string);
            for (dc = db_also; dc; dc = dc->db_also)
                TTY.printf(" %s", dc->db_string);
            TTY.send("\n");
        }
        else {
            sprintf(buf, msg2, db_active ? ' ' : 'I', db_number,
                kw_iplot, db_string);
            for (dc = db_also; dc; dc = dc->db_also)
                sprintf(buf + strlen(buf), " %s", dc->db_string);
            strcat(buf, "\n");
            *retstr = lstring::build_str(*retstr, buf);
        }
        break;

    default:
        GRpkgIf()->ErrPrintf(ET_INTERR, "com_sttus: bad db %d.\n", db_type);
        break;
    }
}


// Print the node voltages for trace.
//
bool
sDbComm::print_trace(sPlot *plot, bool *flag, int pnt)
{
    if (!db_active)
        return (true);
    if (*flag) {
        sDataVec *v1 = plot->scale();
        if (v1)
            TTY.printf_force("%-5d %s = %s\n", pnt, v1->name(), 
                SPnum.printnum(v1->realval(0), v1->units(), false));
        *flag = false;
    }

    if (db_string) {
        if (db_point < 0)
            return (false);
        wordlist *wl = CP.LexStringSub(db_string);
        if (!wl) {
            GRpkgIf()->ErrPrintf(ET_ERROR, Msg, db_string);
            db_point = -1;
            return (false);
        }

        sDvList *dvl = 0;
        pnlist *pl = Sp.GetPtree(wl, true);
        wordlist::destroy(wl);
        if (pl)
            dvl = Sp.DvList(pl);
        if (!dvl) {
            GRpkgIf()->ErrPrintf(ET_ERROR, Msg, db_string);
            db_point = -1;
            return (false);
        }
        for (sDvList *dv = dvl; dv; dv = dv->dl_next) {
            sDataVec *v1 = dv->dl_dvec;
            if (v1 == plot->scale())
                continue;
            if (v1->length() <= 0) {
                if (pnt == 1)
                    GRpkgIf()->ErrPrintf(ET_WARN, Msg, v1->name());
                continue;
            }
            if (v1->isreal())
                TTY.printf_force("  %-10s %s\n", v1->name(), 
                    SPnum.printnum(v1->realval(0), v1->units(), false));

            else {
                // remember printnum returns static data!
                TTY.printf_force("  %-10s %s, ", v1->name(), 
                    SPnum.printnum(v1->realval(0), v1->units(), false));
                TTY.printf_force("%s\n",
                    SPnum.printnum(v1->imagval(0), v1->units(), false));
            }
        }
        sDvList::destroy(dvl);
    }
    return (true);
}


// Print the condition.  If fp is 0 and retstr is not 0,
// add the text to *retstr, otherwise print to fp;
//
void
sDbComm::printcond(char **retstr)
{
    char buf[BSIZE_SP];
    const char *msg1 = " %s %d";
    const char *msg2 = " %s %s";

    buf[0] = '\0';
    for (sDbComm *dt = this; dt; dt = dt->db_also) {
        if (retstr) {
            if (dt->db_type == DB_STOPAFTER)
                sprintf(buf + strlen(buf), msg1, kw_after, dt->db_point);
            else if (dt->db_type == DB_STOPAT)
                sprintf(buf + strlen(buf), msg1, kw_at, dt->db_point);
            else if (dt->db_type == DB_STOPBEFORE)
                sprintf(buf + strlen(buf), msg1, kw_before, dt->db_point);
            else
                sprintf(buf + strlen(buf), msg2, kw_when, dt->db_string);
        }
        else {
            if (dt->db_type == DB_STOPAFTER)
                TTY.printf(msg1, kw_after, dt->db_point);
            else if (dt->db_type == DB_STOPAT)
                TTY.printf(msg1, kw_at, dt->db_point);
            else if (dt->db_type == DB_STOPBEFORE)
                TTY.printf(msg1, kw_before, dt->db_point);
            else
                TTY.printf(msg2, kw_when, dt->db_string);
        }
    }
    if (retstr) {
        strcat(buf, "\n");
        *retstr = lstring::build_str(*retstr, buf);
    }
    else
        TTY.send("\n");
}
// End of sDbComm functions.

