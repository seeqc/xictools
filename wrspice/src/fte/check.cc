
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

//
// Functions for operating range and related analysis
//

#include "frontend.h"
#include "outplot.h"
#include "outdata.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "input.h"
#include "toolbar.h"
#include "psffile.h"
#include "trnames.h"
#include "spnumber/hash.h"
#include "miscutil/filestat.h"
#include <stdarg.h>

//
// The operating range and Monte Carlo analysis commands.
//


// The main command to initiate and control margin analysis.
//
void
CommandTab::com_check(wordlist *wl)
{
    Sp.MargAnalysis(wl);
}


// Command to run a single trial when performing atomic Monte Carlo
// analysis.
//
void
CommandTab::com_mctrial(wordlist*)
{
    if (!Sp.CurCircuit())
        return;
    sCHECKprms *cj = Sp.CurCircuit()->check();
    if (!cj)
        return;
    Sp.SetFlag(FT_MONTE, true);
    int ret;
    cj->set_no_output(true);
    if (!cj->out_cir)
        ret = cj->initial();
    else
        ret = cj->trial(0, 0, 0.0, 0.0);
    cj->set_no_output(false);
    Sp.SetFlag(FT_MONTE, false);
    Sp.SetVar("trial_return", ret);
}


void
CommandTab::com_findrange(wordlist*)
{
    if (!Sp.CurCircuit())
        return;
    sCHECKprms *cj = Sp.CurCircuit()->check();
    if (!cj)
        return;
    int ret = true;

    bool mbak = cj->monte();
    bool abak = cj->doall();
    int s1 = cj->step1();
    int s2 = cj->step2();
    cj->set_monte(false);
    cj->set_doall(false);
    cj->set_step1(0);
    cj->set_step2(0);

    if (!cj->out_cir)
        ret = cj->initial();
    if (ret)
        ret = cj->findRange();

    cj->set_monte(mbak);
    cj->set_doall(abak);
    cj->set_step1(s1);
    cj->set_step2(s2);
    cj->initInput(cj->val1(), cj->val2());
}


namespace {
    void printit(const char *fmt, ...)
    {
        va_list args;
        char buf[1024];
        va_start(args, fmt);
        vsnprintf(buf, 1024, fmt, args);
        va_end(args);

        if (Sp.CurCircuit()->check()->outfp())
            fputs(buf, Sp.CurCircuit()->check()->outfp());
    }
}


// An echo command that puts output in the current analysis output
// file.  This can be called from scripts when running margin
// analysis.
//
void
CommandTab::com_echof(wordlist *wlist)
{
    if (!Sp.CurCircuit() || !Sp.CurCircuit()->check() ||
            !Sp.CurCircuit()->check()->outfp())
        return;
    bool nl = true;
    if (wlist && lstring::eq(wlist->wl_word, "-n")) {
        wlist = wlist->wl_next;
        nl = false;
    }
    while (wlist) {
        char *t = lstring::copy(wlist->wl_word);
        CP.Unquote(t);
        printit(t);
        delete [] t;
        if (wlist->wl_next)
            printit(" ");
        wlist = wlist->wl_next;
    }
    if (nl)
        printit("\n");
}


// This will dump the alter list to the output file, for use in Monte
// Carlo analysis.  In this approach, the alter command, or
// equivalently forms like "let @device[param] = trial_value" are used
// to set trial values in the exec block.  Once set, this can be
// called to dump the values into the output file.
//
void
CommandTab::com_alterf(wordlist*)
{
    if (!Sp.CurCircuit() || !Sp.CurCircuit()->check() ||
            !Sp.CurCircuit()->check()->outfp())
        return;
    Sp.CurCircuit()->printAlter(Sp.CurCircuit()->check()->outfp());
}
// End of CommandTab functions.


sHtab *sCHECKprms::ch_plotnames;

namespace {
    // Hard-wired names for generated range vectors.
    const char *OPLO1 = "opmin1";
    const char *OPHI1 = "opmax1";
    const char *OPLO2 = "opmin2";
    const char *OPHI2 = "opmax2";
    const char *OPVEC = "range";
    const char *OPSCALE = "r_scale";

    const char *checkFAIL = "checkFAIL";
    const char *checkPNTS = "checkPNTS";
    const char *checkINIT = "checkINIT";

    // Hard-wired names for user-given input range vectors.
    const char *checkVAL1 = "checkVAL1";
    const char *checkSTP1 = "checkSTP1";
    const char *checkDEL1 = "checkDEL1";
    const char *checkVAL2 = "checkVAL2";
    const char *checkSTP2 = "checkSTP2";
    const char *checkDEL2 = "checkDEL2";
}


// The margin analysis function.
//
void
IFsimulator::MargAnalysis(wordlist *wl)
{
    if (!ft_curckt) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "no circuit loaded.\n");
        return;
    }

    checkargs args;
    if (ft_curckt->runtype() == MONTE_GIVEN)
        args.setup_monte();
    else if (ft_curckt->runtype() == CHECKALL_GIVEN)
        args.setup_doall();

    const char *po, *pe;
    int err = args.parse(&wl, &po, &pe);
    if (err != OK)
        return;

    sCHECKprms *cj = ft_curckt->check();
    if (args.clear()) {
        if (cj) {
            cj->endJob();
            delete cj;
        }
        return;
    }

    if (cj) {
        // resuming
        if (!args.brk()) {
            TTY.printf("Resuming check run in progress.\n");
            ft_flags[FT_SIMFLAG] = true;
            // Setting the FT_MONTE flag enables random number
            // generation functions.
            ft_flags[FT_MONTE] = args.monte();
            if (cj->initial())
                cj->loop();
            if (cj->ended()) {
                cj->endJob();
                delete cj;
                cj = 0;
            }
            ft_flags[FT_MONTE] = false;
            ft_flags[FT_SIMFLAG] = false;
        }
        return;
    }

    GCarray<const char*> gc_po(po);
    GCarray<const char*> gc_pe(po);

    if (ft_curckt->runonce())
        ft_curckt->rebuild(true);

    err = ft_curckt->checkCodeblocks();
    if (err != OK) {
        wordlist::destroy(wl);
        return;
    }

    ft_flags[FT_INTERRUPT] = false;

    cj = new sCHECKprms();
    err = cj->setup(args, wl);
    if (err != OK) {
        delete cj;
        return;
    }

    if (args.brk())
        return;

    ft_flags[FT_SIMFLAG] = true;
    if (args.findedge())
        cj->findEdge(po, pe);
    else {
        ft_flags[FT_MONTE] = args.monte();
        // Setting the FT_MONTE flag enables random number generation
        // functions.
        if (cj->initial())
            cj->loop();
        ft_flags[FT_MONTE] = false;
    }
    if (cj->ended()) {
        cj->endJob();
        delete cj;
        cj = 0;
    }
    ft_flags[FT_SIMFLAG] = false;
}
// End of IFsimulator functions.


// Check the codeblocks.  There are two, one from the EBLK_KW
// (".exec") lines, which is optional, and one from the CBLK_KW
// (".control") lines, which is essential.  Alternatively, external
// codeblocks can be bound, and are referenced by name, if the name
// field of the circuit struct is not 0.
//
int
sFtCirc::checkCodeblocks()
{
    if (!controls()->name()) {
        if (!controls()->tree()) {
            if (!controls()->text()) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "no control statements or codeblock.\n");
                return (E_NOTFOUND);
            }
            controls()->set_tree(CP.MakeControl(controls()->text()));
            if (!controls()->tree()) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "control statements parse failed.\n");
                return(E_FAILED);
            }
        }
    }
    else {
        if (!CP.IsBlockDef(controls()->name())) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "control codeblock %s not found.\n", 
                controls()->name());
            return (E_NOTFOUND);
        }
    }
    if (!execs()->name()) {
        if (!execs()->tree()) {
            if (!execs()->text())
                GRpkgIf()->ErrPrintf(ET_WARN,
                    "no exec statements or codeblock.\n");
            else {
                execs()->set_tree(CP.MakeControl(execs()->text()));
                if (!execs()->tree()) {
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "exec statements parse failed.\n");
                    return(E_FAILED);
                }
            }
        }
    }
    else {
        if (!CP.IsBlockDef(execs()->name())) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "exec codeblock %s not found.\n", 
                execs()->name());
            return (E_NOTFOUND);
        }
    }
    return (OK);
}


// Obtain an analysis specification from the deck, if not already given.
// Frees wordlist on error, frees elements read.
//
int
sFtCirc::setAnalysis(wordlist **pwl)
{
    if (!pwl)
        return (OK);
    wordlist *wl = *pwl;
    if (!wl || !wl->wl_word || !*wl->wl_word) {
        wordlist *wn = getAnalysisFromDeck();
        if (wn == 0) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "no analysis specified in deck.\n");
            wordlist::destroy(wl);
            *pwl = 0;
            return (E_NOTFOUND);
        }
        if (wn->wl_next) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "more than one analysis specified in deck.\n");
            wordlist::destroy(wl);
            *pwl = 0;
            return (E_TOOMUCH);
        }

        // wn has the entire command in one word, need to tokenize.

        char *ts = wn->wl_word;
        wn->wl_word = 0;
        delete wn;
        wn = CP.Lexer(ts);
        delete [] ts;

        wordlist::destroy(wl);
        wl = wn;
        *pwl = wn;
    }
    char *c;
    for (c = wl->wl_word; *c; c++) {
        if (isupper(*c))
            *c = tolower(*c);
    }
    return (OK);
}
// End of sFtCirc functions.


// Parse check command options.
//
int
checkargs::parse(wordlist **pwl, const char **pe, const char **po)
{
    if (*pe)
        *pe = 0;
    if (*po)
        *po = 0;
    if (!pwl || !*pwl)
        return (OK);

//XXX fixme, add range
    const char *help_mesg =
        "\n  check [options] [analysis]\n"
        "  options are:\n"
        "   -a    Cover all points in range analysis\n"
        "   -b    Perform setup and return, pausing run\n"
        "   -c    Clear paused analysis\n"
        "   -e invec outvec    Find the operating boundary between vecs\n"
        "   -f    Save data during each trial\n"
        "   -h    Show this message\n"
        "   -k    Save all data\n"
        "   -m    Perform Monte Carlo analysis\n"
        "   -r    Use remote servers\n"
        "   -s    Save data during each trial and dump it\n"
        "   -v    Verbose mode\n"
        "  anything left is taken as an analysis command.\n\n";

    wordlist *wl = wordlist::copy(*pwl);
    *pwl = 0;
    wordlist *ww, *wn;
    for (ww = wl; wl; wl = wn) {
        wn = wl->wl_next;
        if (wl->wl_word[0] == '-') {
            for (char *c = wl->wl_word + 1; *c; c++) {
                if (*c == 'a') {
                    // cover all points, not just extrema
                    ca_doall = true;
                }
                else if (*c == 'b') {
                    // stop after setup
                    ca_break = true;
                }
                else if (*c == 'c') {
                    ca_clear = true;
                    wordlist::destroy(wl);
                    return (OK);
                }
                else if (*c == 'e') {
                    // the "find edge" function
                    if (!wl->wl_next || !wl->wl_next->wl_next) {
                        GRpkgIf()->ErrPrintf(ET_ERROR,
                "operating and external vector names must be given.\n");
                        wordlist::destroy(wl);
                        return (E_SYNTAX);
                    }
                    wordlist *wx = wn;
                    wn = wl->wl_next = wl->wl_next->wl_next->wl_next;
                    if (wn)
                        wn->wl_prev = wl;
                    *po = wx->wl_word;
                    *pe = wx->wl_next->wl_word;
                    delete wx->wl_next;
                    delete wx;
                    ca_findedge = true;
                }
                else if (*c == 'f') {
                    // keep points
                    ca_keepplot = true;
                }
                else if (*c == 'k') {
                    // multi-dimension output
                    ca_keepall = true;
                }
                else if (*c == 'm') {
                    // coerce Monte Carlo analysis
                    ca_monte = true;
                    ca_doall = true;
                }
                else if (*c == 'r') {
                    // use remote servers
                    ca_remote = true;
                }
                else if (*c == 's') {
                    // segment output
                    ca_segbase = true;
                }
                else if (*c == 'v') {
                    // verbose, print stuff on screen
                    ca_batchmode = false;
                }
                else {
                    // print help message
                    TTY.send(help_mesg);
                    wordlist::destroy(wl);
                    return (E_PAUSE);
                }
            }
            if (wl->wl_prev)
                wl->wl_prev->wl_next = wl->wl_next;
            if (wl->wl_next)
                wl->wl_next->wl_prev = wl->wl_prev;
            if (ww == wl)
                ww = wl->wl_next;
            delete [] wl->wl_word;
            delete wl;
        }
    }
    *pwl = ww;
    if (ca_findedge) {
        ca_monte = false;
        ca_doall = false;
        ca_remote = false;
    }
    return (OK);
}
// End of checkargs functions.


sCHECKprms::sCHECKprms()
{
    ch_op           = 0;
    ch_opname       = 0;
    ch_flags        = 0;
    ch_tmpout       = 0;
    ch_tmpoutname   = 0;
    ch_graphid      = 0;
    ch_batchmode    = false;
    ch_no_output    = false;
    ch_use_remote   = false;
    ch_monte        = false;
    ch_doall        = false;
    ch_pause        = false;
    ch_nogo         = false;
    ch_fail         = false;
    ch_cycles       = 0;
    ch_evalcnt      = 0;
    ch_index        = 0;
    ch_max          = 0;
    ch_points       = 0;
    ch_segbase      = 0;
    ch_cmdline      = 0;

    ch_val1         = 0.0;
    ch_val2         = 0.0;
    ch_delta1       = 0.0;
    ch_delta2       = 0.0;
    ch_names        = 0;
    ch_devs1        = 0;
    ch_devs2        = 0;
    ch_prms1        = 0;
    ch_prms2        = 0;
    ch_step1        = 0;
    ch_step2        = 0;
    ch_iterno       = 0;
    ch_gotval1      = false;
    ch_gotval2      = false;
    ch_gotdelta1    = false;
    ch_gotdelta2    = false;
    ch_gotstep1     = false;
    ch_gotstep2     = false;
}


sCHECKprms::~sCHECKprms()
{
    delete [] ch_opname;
    wordlist::destroy(ch_cmdline);
    delete ch_names;

    if (ch_tmpout) {
        TTY.ioOverride(0, ch_op, 0);
        fclose(ch_tmpout);
        if (ch_tmpoutname) {
            unlink(ch_tmpoutname);
            delete [] ch_tmpoutname;
        }
    }
    else {
        if (ch_op && ch_op != TTY.outfile()) {
            fclose(ch_op);
            ch_op = 0;
        }
    }
}


int
sCHECKprms::setup(checkargs &args, wordlist *wl)
{
    sFtCirc *curckt = Sp.CurCircuit();
    if (!curckt)
        return (E_NOCKT);

    // Create a new plot for the analysis.
    out_plot = new sPlot("range");
    out_plot->new_plot();
    Sp.SetCurPlot(out_plot->type_name());

    set_batchmode(args.batchmode());
    set_use_remote(args.remote());
    set_monte(args.monte());
    set_doall(args.doall());

    int err = parseRange(&wl);
    if (err != OK) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "syntax error in check command line.\n");
        wordlist::destroy(wl);
        out_plot->destroy();
        out_plot = 0;
        return (err);
    }

    err = curckt->setAnalysis(&wl);
    if (err != OK) {
        wordlist::destroy(wl);
        out_plot->destroy();
        out_plot = 0;
        return (err);
    }

    // Run the exec script.
    set_vec(checkINIT, 1.0);
    execblock(curckt->execs(), false);
    set_vec(checkINIT, 0.0);
    set_vec(checkFAIL, 0.0);

    initRange();
    initNames();
    initCheckPnts();

    if (!args.findedge()) {
        err = initOutFile();
        if (err != OK) {
            wordlist::destroy(wl);
            out_plot->destroy();
            out_plot = 0;
            return (err);
        }
        initOutMode(args.keepall(), args.segbase(), args.keepplot());
    }

    set_cmd(wl);  // takes ownership

    curckt->set_check(this);

    if (!args.batchmode() && !args.monte() && !args.findedge())
        check_print();

    return (OK);
}


namespace {
    // Unlink and free wl, return the next in list.
    //
    wordlist *unlink(wordlist *wl)
    {
        wordlist *wn = wl->wl_next;
        if (wl->wl_prev)
            wl->wl_prev->wl_next = wn;
        if (wn)
            wn->wl_prev = wl->wl_prev;
        delete [] wl->wl_word;
        delete wl;
        return (wn);
    }
}


// Parse [name1 val1 del1 stp1 name2 val2 del2 stp2], everything is
// optional.  The name1/2 are device/param lists as in the sweep
// command.  If these are not given, variables are used to pass trial
// values, otherwise specified parameters are altered directly.  If
// there are 2 dimensions, both or neither must have params specified. 
// The numbers are accepted as ordered, with params string or list end
// truncating the group, any missing numbers are expected to be set
// elsewhere.  E.g.,
//   R1,r 1k R2,r 2k 100 3
// is fine, but it assumes del1 and step1 are set through the vectors
// in the .exec block.
//
// All above applies for operating range analysis.  For Monte Carlo
// analysis, only two optional integers stp1 and stp2 are expected.
//
// Frees wordlist on error, frees elements read.
//
int
sCHECKprms::parseRange(wordlist **pwl)
{
    if (pwl == 0)
        return (OK);
    wordlist *wl = *pwl;
    if (wl == 0)
        return (OK);
    *pwl = 0;

    if (ch_monte) {
        // For Monte Carlo, we accept [stp1 [stp2]].

        const char *line = wl->wl_word;
        int error;
        double tmp = IP.getFloat(&line, &error, true);  // step1
        if (error == OK) {
            ch_step1 = rnd(tmp);
            ch_gotstep1 = true;
            wl = unlink(wl);
        }
        else {
            *pwl = wl;
            return (OK);
        }
        if (!wl)
            return (OK);

        line = wl->wl_word;
        tmp = IP.getFloat(&line, &error, true);  // step2
        if (error == OK) {
            ch_step2 = rnd(tmp);
            ch_gotstep2 = true;
            wl = unlink(wl);
        }
        *pwl = wl;
        return (OK);
    }

    const char *line = wl->wl_word;
    int error;
    double tmp = IP.getFloat(&line, &error, true);  // name1 or val1
    if (error == OK) {
        ch_val1 = tmp;
        ch_gotval1 = true;
        wl = unlink(wl);
    }
    else {
        char *dstr = lstring::getqtok(&line);
        sFtCirc::parseDevParams(dstr, &ch_devs1, &ch_prms1, false);
        delete [] dstr;
        if (!ch_devs1 || !ch_prms1) {
            wordlist::destroy(wl);
            return (E_SYNTAX);
        }
        wl = unlink(wl);
        if (!wl)
            return (OK);

        line = wl->wl_word;
        tmp = IP.getFloat(&line, &error, true);  // val1
        if (error == OK) {
            ch_val1 = tmp;
            ch_gotval1 = true;
            wl = unlink(wl);
        }
        else
            goto nextone;
    }
    if (!wl)
        return (OK);

    line = wl->wl_word;
    tmp = IP.getFloat(&line, &error, true);  // del1
    if (error == OK) {
        ch_delta1 = tmp;
        ch_gotdelta1 = true;
        wl = unlink(wl);
    }
    else
        goto nextone;

    if (!wl)
        return (OK);

    line = wl->wl_word;
    tmp = IP.getFloat(&line, &error, true);  // step1
    if (error == OK) {
        ch_step1 = rnd(tmp);
        ch_gotstep1 = true;
        wl = unlink(wl);
    }
    else
        goto nextone;

    if (!wl)
        return (OK);

    line = wl->wl_word;
    tmp = IP.getFloat(&line, &error, true);  // name2 or val2
    if (error == OK) {
        if (ch_devs1) {
            wordlist::destroy(wl);
            return (E_SYNTAX);
        }
        ch_val2 = tmp;
        ch_gotval2 = true;
        wl = unlink(wl);
    }
    else {
nextone:
        // It is non-numeric, it is either a parameter spec or the
        // start of the analysis command.

        if (sFtCirc::analysisType(line) >= 0) {
            *pwl = wl;
            return (OK);
        }

        if (!ch_devs1) {
            wordlist::destroy(wl);
            return (E_SYNTAX);
        }

        char *dstr = lstring::getqtok(&line);
        sFtCirc::parseDevParams(dstr, &ch_devs2, &ch_prms2, false);
        delete [] dstr;
        if (!ch_devs2 || !ch_prms2) {
            wordlist::destroy(wl);
            return (E_SYNTAX);
        }
        wl = unlink(wl);
        if (!wl)
            return (OK);

        line = wl->wl_word;
        tmp = IP.getFloat(&line, &error, true);  // val2
        if (error == OK) {
            ch_val2 = tmp;
            ch_gotval2 = true;
            wl = unlink(wl);
        }
        else {
            *pwl = wl;
            return (OK);
        }
    }
    if (!wl)
        return (OK);

    line = wl->wl_word;
    tmp = IP.getFloat(&line, &error, true);  // del2
    if (error == OK) {
        ch_delta2 = tmp;
        ch_gotdelta2 = true;
        wl = unlink(wl);
    }
    else {
        *pwl = wl;
        return (OK);
    }
    if (!wl)
        return (OK);

    line = wl->wl_word;
    tmp = IP.getFloat(&line, &error, true);  // step2
    if (error == OK) {
        ch_step2 = rnd(tmp);
        ch_gotstep2 = true;
        wl = unlink(wl);
    }
    *pwl = wl;
    return (OK);
}


// Initialize the unset range variables from the associated vectors.
//
void
sCHECKprms::initRange()
{
    if (!ch_gotval1) {
        sDataVec *d = out_plot->find_vec(checkVAL1);
        if (d) {
            ch_val1 = d->realval(0);
            ch_gotval1 = true;
        }
    }
    if (!ch_gotdelta1) {
        sDataVec *d = out_plot->find_vec(checkDEL1);
        if (d) {
            ch_delta1 = d->realval(0);
            ch_gotdelta1 = true;
        }
    }
    if (ch_delta1 < 0.0) {
        GRpkgIf()->ErrPrintf(ET_WARN,
            "negative delta1, absolute value used.\n");
        ch_delta1 = -ch_delta1;
    }
    if (!ch_gotstep1) {
        sDataVec *d = out_plot->find_vec(checkSTP1);
        if (d) {
            ch_step1 = rnd(d->realval(0));
            ch_gotstep1 = true;
        }
    }
    if (ch_step1 < 0) {
        GRpkgIf()->ErrPrintf(ET_WARN, "negative step1 value, set to 0.\n");
        ch_step1 = 0;
    }
    if (!ch_gotval2) {
        sDataVec *d = out_plot->find_vec(checkVAL2);
        if (d) {
            ch_val2 = d->realval(0);
            ch_gotval2 = true;
        }
    }
    if (!ch_gotdelta2) {
        sDataVec *d = out_plot->find_vec(checkDEL2);
        if (d) {
            ch_delta2 = d->realval(0);
            ch_gotdelta2 = true;
        }
    }
    if (ch_delta2 < 0.0) {
        GRpkgIf()->ErrPrintf(ET_WARN,
            "negative delta2, absolute value used.\n");
        ch_delta2 = -ch_delta2;
    }
    if (!ch_gotstep2) {
        sDataVec *d = out_plot->find_vec(checkSTP2);
        if (d) {
            ch_step2 = rnd(d->realval(0));
            ch_gotstep2 = true;
        }
    }
    if (ch_step2 < 0) {
        GRpkgIf()->ErrPrintf(ET_WARN, "negative step2 value, set to 0.\n");
        ch_step2 = 0;
    }
    if (ch_monte) {
        // default 49 points
        if (!ch_gotstep1 && !ch_gotstep2) {
            ch_step1 = 3;
            ch_step2 = 3;
            ch_gotstep1 = true;
            ch_gotstep2 = true;
        }
    }
}


// Allocate storage for the vector names table.
//
void
sCHECKprms::initNames()
{
    delete ch_names;
    ch_names = 0;
    if (!ch_devs1 && !ch_devs2)
        ch_names = sNames::set_names();
}


// Open the output file.
//
int
sCHECKprms::initOutFile()
{
    char code = ch_monte ? 'm' : 'd';
    ch_op = df_open(code, &ch_tmpoutname, &ch_tmpout, ch_names);
    if (!ch_op) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "Can't open data output file.\n");
        return (E_FAILED);
    }
    return (OK);
}


// Set up segmenting and data retention.
//
void
sCHECKprms::initOutMode(bool keepall, bool sgbase, bool keepplot)
{
    out_mode = OutcCheck;
    ch_cycles = 0;
    delete [] ch_segbase;
    ch_segbase = 0;

    if (keepall) {
        // Keep all data in a multi-dimensional plot.
        out_mode = OutcCheckMulti;
        ch_cycles = (2*ch_step1 + 1)*(2*ch_step2 + 1);
    }
    else if (sgbase) {
        // Keep all data for curent trial, dump a "segment" rawfile.
        VTvalue vv;
        if (Sp.GetVar(kw_mplot_cur, VTYP_STRING, &vv))
            ch_segbase = lstring::copy(vv.get_string());
        out_mode = OutcCheckSeg;
    }
    else if (keepplot || Sp.CurCircuit()->measures() || OP.isIplot(true)) {
        // Keep all data for curent trial.
        out_mode = OutcCheckSeg;
    }
}


// Set up the testing time points vector.
//
void
sCHECKprms::initCheckPnts()
{
//XXX copy this
    // implicitly set to last point
    ch_max = 0;
    ch_points = 0;
    sDataVec *d = out_plot->find_vec(checkPNTS);
    if (d) {
        // Make sure an "exact" point is recognized.
        for (int i = 0; i < d->length(); i++)
            d->set_realval(i, d->realval(i) * 0.9999);
        ch_max = d->length();
        ch_points = d->realvec();
    }
}


// Pass the operating point values for the next ineration.
//
void
sCHECKprms::initInput(double value1, double value2)
{
    if (ch_monte)
        return;
    if (ch_names)
        ch_names->set_input(out_cir, out_plot, value1, value2);
    else {
        char nbuf[32];
        if (ch_devs1) {
            sprintf(nbuf, "%.15e", value1);
            for (wordlist *wd = ch_devs1; wd; wd = wd->wl_next) {
                for (wordlist *wp = ch_prms1; wp; wp = wp->wl_next)
                    out_cir->addDeferred(wd->wl_word, wp->wl_word, nbuf);
            }
            if (ch_devs2) {
                sprintf(nbuf, "%.15e", value2);
                for (wordlist *wd = ch_devs2; wd; wd = wd->wl_next) {
                    for (wordlist *wp = ch_prms2; wp; wp = wp->wl_next)
                        out_cir->addDeferred(wd->wl_word, wp->wl_word, nbuf);
                }
            }
        }
    }
}


// Finish initializing and begin the analysis, doing the first run. 
// If true is returned, all was successful and caller can finish the
// analysis by calling the loop function.  Otherwise, an error ocurred
// and the analysis should be halted.
//
bool
sCHECKprms::initial()
{
    char buf[256];
    buf[0] = 0;
    if (ch_pause) {
        ch_pause = false;
        if (!ch_graphid) {
            ch_graphid = (GP.MpInit(2*ch_step1+1, 2*ch_step2+1, 
                ch_val1 - ch_step1*ch_delta1,
                ch_val1 + ch_step1*ch_delta1,
                ch_val2 - ch_step2*ch_delta2,
                ch_val2 + ch_step2*ch_delta2, ch_monte, out_plot));
            if (ch_graphid && ch_flags)
                plot();
        }
        if (ch_flags)
            return (true);
    }
    else {
        ch_iterno = 0;
        out_cir = Sp.CurCircuit();

        double value1=0.0, value2=0.0;
        if (!ch_monte) {
            value1 = ch_val1 - ch_step1 * ch_delta1;
            value2 = ch_val2 - ch_step2 * ch_delta2;
            initInput(value1, value2);
        }

        sDimen *dm = new sDimen(ch_names ? ch_names->value1() : "value1",
            ch_names ? ch_names->value2() : "value2");
        if (!ch_monte) {
            dm->set_start1(value1);
            dm->set_stop1(ch_val1 + ch_step1 * ch_delta1);
            dm->set_step1(ch_delta1);
            dm->set_start2(value2);
            dm->set_stop2(ch_val2 + ch_step2 * ch_delta2);
            dm->set_step2(ch_delta2);
        }
        out_plot->set_dimensions(dm);

        if (!Sp.GetFlag(FT_SERVERMODE)) {
            ch_graphid = (GP.MpInit(2*ch_step1+1, 2*ch_step2+1, 
                value1, ch_val1 + ch_step1*ch_delta1, 
                value2, ch_val2 + ch_step2*ch_delta2, ch_monte, out_plot));
        }

        if (!ch_graphid && !ch_batchmode) {
            if (ch_monte)
                TTY.printf_force("%3d %3d run %3d\n", -ch_step1, -ch_step2, 1);
            else {
                TTY.printf_force("%3d %3d %12g %12g\n",
                    -ch_step1, -ch_step2, value1, value2);
            }
        }
        if (ch_op) {
            if (ch_monte)
                sprintf(buf, "[DATA] %3d %3d run %3d",
                    -ch_step1, -ch_step2, 1);
            else
                sprintf(buf, "[DATA] %3d %3d %12g %12g", 
                    -ch_step1, -ch_step2, value1, value2);
        }
        set_opvec(-1, -1);  // clear oplo, ophi
        VTvalue vv;
        if (Sp.GetVar(kw_checkiterate, VTYP_NUM, &vv, out_cir))
            ch_iterno = vv.get_int();

        if (ch_iterno < 0 || ch_iterno > 10) {
            ch_iterno = 0;
            GRpkgIf()->ErrPrintf(ET_WARN,
                "bad value for checkiterate, ignored.\n");
        }
        if (ch_iterno)
            set_opvec(ch_step2, ch_step1);
    }

    if (ch_names || ch_monte)
        out_cir->rebuild(true);
    else
        out_cir->reset();
    GP.MpWhere(ch_graphid, -ch_step1, -ch_step2);

    out_cir->set_keep_deferred(true);
    bool flg = Sp.GetFlag(FT_DCOSILENT);
    Sp.SetFlag(FT_DCOSILENT, true);
    int error = out_cir->run(ch_cmdline->wl_word, ch_cmdline->wl_next);
    Sp.SetFlag(FT_DCOSILENT, flg);
    out_cir->set_keep_deferred(false);

    if (error < 0) { // user interrupt
        ch_pause = true;
        return (false);
    }
    if (error == E_ITERLIM) {
        // Failed to converge, take this as a fail point.
        // 
        ch_fail = true;
        error = OK;
    }
    if (error == OK) {
        if (!ch_no_output) {
            if (GP.MpMark(ch_graphid, !ch_fail) && !ch_batchmode)
                TTY.printf_force(ch_fail ? " FAIL\n\n" : " PASS\n\n");
            if (ch_op)
                fprintf(ch_op, "%s\t\t%s\n", buf, ch_fail ? "FAIL" : "PASS");
        }
        addpoint(-ch_step1, -ch_step2, ch_fail);
    }
    else {
        ch_nogo = true;
        return (false);
    }
    if (Sp.GetFlag(FT_SERVERMODE))
        // only run one point
        return (false);

    return (true);
}


// Control loop for operating range analysis.
//
bool
sCHECKprms::loop()
{
    if (ch_step1 == 0 && ch_step2 == 0) {
        if (ch_monte)
            return (OK);
        return (findRange());
    }

    int num1 = 2*ch_step1 + 1;
    int num2 = 2*ch_step2 + 1;
    if (num1*num2 > 10000) {
        GRpkgIf()->ErrPrintf(ET_WARN,
        "evaluating more than 10000 points, hope that was intended!\n");
    }
    if (!ch_flags) {
        ch_flags = new char[num1*num2];
        memset(ch_flags, 0, num1*num2*sizeof(char));
        // fill in the first point
        // flags:
        // 1 passed
        // 2 failed
        // 0 not checked
        //
        *ch_flags = 1 + ch_fail;
    }
    if (ch_use_remote) {
        registerJob();
        return (false);
    }

    int i, j;
    double value1, value2;
    char *rowflags;
    if (ch_doall) {
        for (j = -ch_step2; j <= ch_step2; j++) {
            value2 = ch_val2 + j*ch_delta2;
            rowflags = ch_flags + (j + ch_step2)*num1;
            for (i = -ch_step1; i <= ch_step1; i++, rowflags++) {
                if (*rowflags) continue;
                value1 = ch_val1 + i*ch_delta1;
                *rowflags = trial(i, j, value1, value2);
                if (ch_pause || ch_nogo) // user interrupt or error
                    goto quit;
            }
        }
        delete [] ch_flags;
        ch_flags = 0;
        return (false);
    }

    int last;
    // Find the range along the rows
    if (ch_step1 > 0 || ch_step2 == 0) {
        for (j = -ch_step2; j <= ch_step2; j++) {
            value2 = ch_val2 + j*ch_delta2;

            rowflags = ch_flags + (j + ch_step2)*num1;
            for (last = i = -ch_step1; i <= ch_step1; i++, rowflags++) {
                if (*rowflags == 2) continue;
                value1 = ch_val1 + i*ch_delta1;
                if (*rowflags == 1) break;
                *rowflags = trial(i, j, value1, value2);
                if (ch_pause || ch_nogo) // user interrupt or error
                    goto quit;
                if (*rowflags == 1) break;
            }
            last = i;
            if (*rowflags == 1 && ch_iterno) {
                if (i != -ch_step1) {
                    sDataVec *d = out_plot->find_vec(OPLO1);
                    if (d) {
                        if (findext1(ch_iterno, &value1, value2, ch_delta1))
                            goto quit;
                        d->set_realval(j+ch_step2, value1);
                    }
                }
                else {
                    if (findrange1(value1, j + ch_step2, true, false))
                        goto quit;
                }
            }

            rowflags = ch_flags + (j + ch_step2 + 1)*num1 - 1;
            for (i = ch_step1; i > last; i--, rowflags--) {
                if (*rowflags == 2) continue;
                value1 = ch_val1 + i*ch_delta1;
                if (*rowflags == 1) break;
                *rowflags = trial(i, j, value1, value2);
                if (ch_pause || ch_nogo) // user interrupt or error
                    goto quit;
                if (*rowflags == 1) break;
            }
            if (*rowflags == 1 && ch_iterno) {
                if (i != ch_step1) {
                    sDataVec *d = out_plot->find_vec(OPHI1);
                    if (d) {
                        if (findext1(ch_iterno, &value1, value2, -ch_delta1))
                            goto quit;
                        d->set_realval(j+ch_step2, value1);
                    }
                }
                else {
                    if (findrange1(value1, j + ch_step2, false, true))
                        goto quit;
                }
            }
        }
    }

    // Now check the columns, fill in any missing points.
    //
    if (ch_step2 > 0) {
        for (i = -ch_step1; i <= ch_step1; i++) {
            value1 = ch_val1 + i*ch_delta1;

            rowflags = ch_flags + i + ch_step1;
            for (last = j = -ch_step2; j <= ch_step2;
                    j++, rowflags += num1) {
                if (*rowflags == 2) continue;
                value2 = ch_val2 + j*ch_delta2;
                if (*rowflags == 1) break;
                *rowflags = trial(i, j, value1, value2);
                if (ch_pause || ch_nogo) // user interrupt or error
                    goto quit;
                if (*rowflags == 1) break;
            }
            if (*rowflags == 1 && ch_iterno) {
                if (j != -ch_step2) {
                    sDataVec *d = out_plot->find_vec(OPLO2);
                    if (d) {
                        if (findext2(ch_iterno, value1, &value2,
                                ch_delta2))
                            goto quit;
                        d->set_realval(i+ch_step1, value2);
                    }
                }
                else {
                    if (findrange2(value2, i + ch_step1, true, false))
                        goto quit;
                }
            }
            last = j;

            rowflags = ch_flags + i + ch_step1 + 2*ch_step2*num1;
            for (j = ch_step2; j > last; j--, rowflags -= num1) {
                if (*rowflags == 2) continue;
                value2 = ch_val2 + j*ch_delta2;
                if (*rowflags == 1) break;
                *rowflags = trial(i, j, value1, value2);
                if (ch_pause || ch_nogo) // user interrupt or error
                    goto quit;
                if (*rowflags == 1) break;
            }
            if (*rowflags == 1 && ch_iterno) {
                if (j != ch_step2) {
                    sDataVec *d = out_plot->find_vec(OPHI2);
                    if (d) {
                        if (findext2(ch_iterno, value1, &value2, -ch_delta2))
                            goto quit;
                        d->set_realval(i+ch_step1, value2);
                    }
                }
                else {
                    if (findrange2(value2, i + ch_step1, false, true))
                        goto quit;
                }
            }
        }
    }
    delete [] ch_flags;
    ch_flags = 0;
    return (false);
quit:
    if (ch_nogo) {
        delete [] ch_flags;
        ch_flags = 0;
    }
    return (true);
}


// Perform a simulation, for one condition.  Return 1 if pass, 2 if
// fail, 0 if error.
//
int
sCHECKprms::trial(int i, int j, double value1, double value2)
{
    if (!out_cir || !out_plot) {
        ch_nogo = true;
        return (0);
    }
    char buf[256];
    buf[0] = 0;

    sFtCirc *cir = Sp.CurCircuit();
    sPlot *plt = Sp.CurPlot();
    Sp.SetCurCircuit(out_cir);
    Sp.SetCurPlot(out_plot);
    if (ch_monte) {
        execblock(out_cir->execs(), true);
        sDataVec *d = out_plot->find_vec(checkPNTS);
        if (d) {
            // Needed since checkPNTS may be redefined in the header
            // block.
            //
            ch_max = d->length();
            ch_points = d->realvec();
        }
        if (!ch_no_output) {
            int num = (j + ch_step2)*(2*ch_step1 + 1) + i + ch_step1 + 1;
            if (GP.MpWhere(ch_graphid, i, j) && !ch_batchmode)
                TTY.printf_force("%3d %3d run %3d\n", i, j, num);
            if (ch_op)
                sprintf(buf, "[DATA] %3d %3d run %3d", i, j, num);
        }
    }
    else {
        initInput(value1, value2);
        if (!ch_no_output) {
            if (GP.MpWhere(ch_graphid, i, j) && !ch_batchmode)
                TTY.printf_force("%3d %3d %12g %12g\n", i, j, value1, value2);
            if (ch_op)
                sprintf(buf, "[DATA] %3d %3d %12g %12g",
                    i, j, value1, value2);
        }
    }

    out_cir->resetTrial(ch_monte || ch_names);
    ToolBar()->SuppressUpdate(true);
    out_cir->set_keep_deferred(true);

    int error = out_cir->runTrial();
    if (error == E_ITERLIM) {
        // Failed to converge, take this as a fail point.
        // 
        ch_fail = true;
        error = OK;
    }
    out_cir->set_keep_deferred(false);
    ToolBar()->SuppressUpdate(false);

    if (error < 0)
        ch_pause = true;
    else if (error == OK) {
        if (!ch_no_output) {
            if (GP.MpMark(ch_graphid, !ch_fail) && !ch_batchmode)
                TTY.printf_force(ch_fail ? " FAIL\n\n" : " PASS\n\n");
            if (ch_op)
                fprintf(ch_op, "%s\t\t%s\n", buf, ch_fail ? "FAIL" : "PASS");
        }
        addpoint(i, j, ch_fail);
        return (ch_fail + 1);
    }
    else
        ch_nogo = true;
    Sp.SetCurCircuit(cir);
    Sp.SetCurPlot(plt);

    return (0);
}


// Evaluate pass/fail of the circuit at the current operating point.
//
void
sCHECKprms::evaluate()
{
    bool fail = true;
    if (out_cir && out_plot) {
        sFtCirc *cir = Sp.CurCircuit();
        sPlot *plt = Sp.CurPlot();
        Sp.SetCurCircuit(out_cir);
        Sp.SetCurPlot(out_plot);
        sDataVec *d = out_plot->find_vec(checkFAIL);
        if (d && d->isreal()) {
            d->set_realval(0, 0.0);
            // update windows first call only
            execblock(out_cir->controls(), ch_evalcnt == 0 ? true : false);
            // checkFAIL is true if failed
            d = out_plot->find_vec(checkFAIL);
            // dummy user may have deleted it
            if (d && d->isreal())
                fail = (d->realval(0) != 0.0 ? true : false);
        }
        Sp.SetCurCircuit(cir);
        Sp.SetCurPlot(plt);
    }
    ch_evalcnt++;
    ch_fail = fail;
}


// Given two vectors of circuit parameters such that the circuit fails
// with one set and passes with the other, iterate to the operating
// boundary.
//
void
sCHECKprms::findEdge(const char *po, const char *pc)
{
    if (!ch_names) {
        GRpkgIf()->ErrPrintf(ET_ERROR,
            "find edge function requires a \"values\" vector.");
        return;
    }

    VTvalue vv;
    if (Sp.GetVar(kw_checkiterate, VTYP_NUM, &vv, out_cir))
        ch_iterno = vv.get_int();

    if (ch_iterno < DEF_checkiterate_MIN ||
            ch_iterno > DEF_checkiterate_MAX) {
        ch_iterno = DEF_checkiterate;
        GRpkgIf()->ErrPrintf(ET_WARN,
            "bad value for checkiterate, set to %d.\n", DEF_checkiterate);
    }
    sDataVec *vo = out_plot->find_vec(po);
    if (!vo) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "can't find vector %s.\n", po);
        return;
    }
    sDataVec *vc = out_plot->find_vec(pc);
    if (!vc) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "can't find vector %s.\n", pc);
        return;
    }
    sDataVec *value = out_plot->find_vec(ch_names->value());
    if (!value) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "can't find vector %s.\n",
            ch_names->value());
        return;
    }
    int len = value->length();
    if (len < vo->length() || len < vc->length()) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "vector lengths are incompatible.\n");
        return;
    }
    double *delta = new double[len];
    for (int i = 0; i < len; i++) {
        delta[i] = (vo->realval(i) - vc->realval(i))*0.5;
        value->set_realval(i, value->realval(i) - delta[i]);
    }

    bool pass1 = true;
    ch_no_output = true;
    int itno = ch_iterno;
    while (itno--) {
        int error;
        out_cir->resetTrial(ch_names != 0);
        ToolBar()->SuppressUpdate(true);
        if (pass1) {
            pass1 = false;
            out_cir = Sp.CurCircuit();
            out_cir->set_keep_deferred(true);
            bool flg = Sp.GetFlag(FT_DCOSILENT);
            Sp.SetFlag(FT_DCOSILENT, true);
            error = out_cir->run(ch_cmdline->wl_word, ch_cmdline->wl_next);
            Sp.SetFlag(FT_DCOSILENT, flg);
            out_cir->set_keep_deferred(false);
            if (error == 0)
                out_cir = Sp.CurCircuit();
        }
        else {
            out_cir->set_keep_deferred(true);
            error = out_cir->runTrial();
            out_cir->set_keep_deferred(false);
        }
        ToolBar()->SuppressUpdate(false);
        if (error < 0) {
            // User interrupt, can't resume.
            ch_nogo = true;
            ch_no_output = false;
            return;
        }
        else if (error != OK) {
            if (error == E_ITERLIM) {
                // Failed to converge, take this as a fail point.
                ch_fail = true;
                error = OK;
            }
            else {
                ch_nogo = true;
                ch_no_output = false;
                return;
            }
        }
        if (!ch_fail) {
            for (int i = 0; i < len; i++) {
                delta[i] *= 0.5;
                value->set_realval(i, value->realval(i) - delta[i]);
            }
        }
        else {
            for (int i = 0; i < len; i++) {
                delta[i] *= 0.5;
                value->set_realval(i, value->realval(i) + delta[i]);
            }
        }
    }
    ch_no_output = false;
}


// Find the operating range of value1 and value2.  The initial values
// must be nonzero, and the output vectors should exist, otherwise the
// search is skipped.  The search is also skipped if ch_iterno
// (the checkiterate variable) is zero.
//
bool
sCHECKprms::findRange()
{
    if (ch_fail) {
        // center point bad
        GRpkgIf()->ErrPrintf(ET_ERROR,
            "central point bad, range not found.\n");
        ch_nogo = true;
        return (true);
    }
    if (ch_doall) {
        // With the "all" flag set, find the range of each of the
        // value vector entries that are not masked by value_mask,
        // if value exists.
        //
        if (!ch_names) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
            "find range with \"all\" flag set requires a \"values\" vector.");
            return (true);
        }
        VTvalue vv;
        if (Sp.GetVar(kw_checkiterate, VTYP_NUM, &vv, out_cir))
            ch_iterno = vv.get_int();

        if (ch_iterno < 0 || ch_iterno > 10) {
            ch_iterno = 0;
            GRpkgIf()->ErrPrintf(ET_WARN,
                "bad value for checkiterate, ignored.\n");
        }
        if (ch_iterno == 0)
            return (false);
        sDataVec *d = out_plot->find_vec(ch_names->value());
        if (d && d->isreal()) {
            char maskbuf[64];
            sprintf(maskbuf, "%s_mask", ch_names->value());
            sDataVec *vm = out_plot->find_vec(maskbuf);
            set_vec(OPLO1, 0.0);
            set_vec(OPHI1, 0.0);
            sDataVec *tmpd = out_plot->find_vec(OPLO1);
            if (tmpd && tmpd->isreal()) {
                tmpd->set_realvec(new double[d->length()], true);
                tmpd->set_length(d->length());
            }
            else
                return (true);
            tmpd = out_plot->find_vec(OPHI1);
            if (tmpd && tmpd->isreal()) {
                tmpd->set_realvec(new double[d->length()], true);
                tmpd->set_length(d->length());
            }
            else
                return (true);
            for (int i = 0; i < d->length(); i++) {
                if (vm && vm->isreal() && i < vm->length() &&
                        vm->realval(i) != 0.0)
                    continue;
                double value1 = d->realval(i);
                set_vec(ch_names->n1(), (double)i);
                if (findrange1(value1, i, true, true))
                    continue;
            }
        }
        return (false);
    }
    if (ch_iterno == 0)
        return (false);
    if (findrange1(ch_val1, 0, true, true))
        return (true);
    if (findrange2(ch_val2, 0, true, true))
        return (true);
    return (false);
}


// Static function.
// Map the margin plot file name to a plot name in a hash table.  This
// is used when creating an mplot from the file.
//
void
sCHECKprms::setMfilePlotname(const char *fname, const char *tpname)
{
    if (!ch_plotnames)
        ch_plotnames = new sHtab(false);
    sHent *ent = sHtab::get_ent(ch_plotnames, fname);
    if (ent) {
        delete [] (char*)ent->data();
        ent->set_data(lstring::copy(tpname));
        return;
    }
    ch_plotnames->add(fname, lstring::copy(tpname));
}


// Static function.
// Return the plot name that corresponds to fname.
//
const char *
sCHECKprms::mfilePlotname(const char *fname)
{
    if (!ch_plotnames || !fname)
        return (0);
    return ((const char*)sHtab::get(ch_plotnames, fname));
}


// Set the named vector to val.  Create it if it does not exist.
//
void
sCHECKprms::set_vec(const char *name, double val)
{
    sDataVec *d = out_plot->find_vec(name);
    if (d && d->isreal()) {
        d->set_realval(0, val);
        return;
    }

    // Give the plot name explicitly to avoid use of the context plot.
    sLstr lstr;
    lstr.add(out_plot->type_name());
    lstr.add_c(Sp.PlotCatchar());
    lstr.add(name);
    char buf[64];
    sprintf(buf, "%.12g", val);
    Sp.VecSet(lstr.string(), buf);
}


// Create vectors for operating range extrema.
//
void
sCHECKprms::set_opvec(int n1, int n2)
{
    char buf[64];
    if (n1 < 0) {
        out_plot->remove_vec(OPLO1);
        out_plot->remove_vec(OPHI1);
    }
    else {
        n1 += n1;
        sprintf(buf, "%s[%d]", OPLO1, n1);
        set_vec(buf, 0.0);
        sprintf(buf, "%s[%d]", OPHI1, n1);
        set_vec(buf, 0.0);
    }
    if (n2 < 0) {
        out_plot->remove_vec(OPLO2);
        out_plot->remove_vec(OPHI2);
    }
    else {
        n2 += n2;
        sprintf(buf, "%s[%d]", OPLO2, n2);
        set_vec(buf, 0.0);
        sprintf(buf, "%s[%d]", OPHI2, n2);
        set_vec(buf, 0.0);
    }
}


void
sCHECKprms::check_print()
{
    double tval1 = 0.0;
    int tstep1 = 0;
    double tdelta1 = 0.0;
    double tval2 = 0.0;
    int tstep2 = 0;
    double tdelta2 = 0.0;

    sDataVec *d;
    if ((d = out_plot->find_vec(checkDEL1)) != 0)
        tdelta1 = d->realval(0);
    if ((d = out_plot->find_vec(checkVAL1)) != 0)
        tval1 = d->realval(0);
    if ((d = out_plot->find_vec(checkDEL2)) != 0)
        tdelta2 = d->realval(0);
    if ((d = out_plot->find_vec(checkVAL2)) != 0)
        tval2 = d->realval(0);
    if ((d = out_plot->find_vec(checkSTP1)) != 0)
        tstep1 = (int) d->realval(0);
    if ((d = out_plot->find_vec(checkSTP2)) != 0)
        tstep2 = (int) d->realval(0);
    int titerno = 0;
    VTvalue vv;
    if (Sp.GetVar(kw_checkiterate, VTYP_NUM, &vv, Sp.CurCircuit()))
        titerno = vv.get_int();

    TTY.printf(
    "Substitution for value1:\n"
    "    value: %g\n"
    "    delta: %g\n"
    "    steps: %d\n\n", tval1, tdelta1, tstep1);

    TTY.printf(
    "Substitution for value2:\n"
    "    value: %g\n"
    "    delta: %g\n"
    "    steps: %d\n\n", tval2, tdelta2, tstep2);

    TTY.printf("checkiterate is set to %d\n\n", titerno);
}


// Static function.
// Execute the codeblock.
//
void
sCHECKprms::execblock(sExBlk *exblk, bool suppress)
{
    if (exblk && (exblk->name() || exblk->tree())) {
        bool temp = CP.GetFlag(CP_INTERACTIVE);
        CP.SetFlag(CP_INTERACTIVE, false);
        TTY.ioPush();
        Sp.PushPlot();

        if (suppress)
            ToolBar()->SuppressUpdate(true);
        ToolBar()->UpdateVectors(2);

        if (exblk->name())
            CP.ExecBlock(exblk->name());
        else if (exblk->tree())
            CP.ExecControl(exblk->tree());
        ToolBar()->UpdateVectors(2);

        if (suppress)
            ToolBar()->SuppressUpdate(false);

        Sp.PopPlot();
        TTY.ioPop();
        CP.SetFlag(CP_INTERACTIVE, temp);
    }
}


// Open the data output file for writing.  name: xxxxx.cnn , where
// xxxxx is the base of the circuit file name, or "check" if no
// current file name.  nn is 00 - 99.
// If successful, set the variable "mplot_cur" to the file name.
//
FILE *
sCHECKprms::df_open(int c, char **rdname, FILE **rdfp, sNames *tnames)
{
    *rdname = 0;
    *rdfp = 0;
    if (Sp.GetFlag(FT_SERVERMODE)) {
        Sp.SetVar(kw_mplot_cur, "mplot");
        // If a non-psf filename was given on the command line, use it.
        if (Sp.GetOutDesc()->outFile() &&
                !cPSFout::is_psf(Sp.GetOutDesc()->outFile())) {
            FILE *fp = fopen(Sp.GetOutDesc()->outFile(), "w");
            if (!fp)
                fp = TTY.outfile();
            return (fp);
        }
        // If using TTY.outfile(), redirect TTY.outfile() to a
        // temp file, which will be dumped after the analysis
        // data.
        //
        *rdname = filestat::make_temp("so");
        *rdfp = fopen(*rdname, "w");
        if (!*rdfp) {
            delete [] *rdname;
            *rdname = 0;
            return (TTY.outfile());
        }
        FILE *oldcpout = TTY.outfile();
        TTY.ioOverride(0, *rdfp, 0);
        fputs("@\n", oldcpout);
        return (oldcpout);
    }
        
    char buf[128];
    if (Sp.CurCircuit()->filename())
        strcpy(buf, Sp.CurCircuit()->filename());
    else
        strcpy(buf, "check");
    char *s;
    if ((s = lstring::strrdirsep(buf)) != 0)
        s++;
    else
        s = buf;
    char buf1[128];
    strcpy(buf1, s);

    if ((s = strrchr(buf1, '.')) != 0)
        *s = '\0';
    char extn[8];
    sprintf(extn, ".%c00", c);
    strcat(buf1, extn);
    s = strchr(buf1, '.') + 2;
    int i;
    for (i = 1; ; i++) {
        if (access(buf1, 0)) break;
        sprintf(s, "%02d", i);
    }
    FILE *fp = fopen(buf1, "w");
    if (!fp) {
        GRpkgIf()->Perror(buf1);
        return (0);
    }
    const char *filename = Sp.CurCircuit()->filename();
    if (!filename)
        filename = "<unknown>";
    if (c == 'm')
        fprintf(fp, "Monte Carlo Analysis from %s\n", CP.Program());
    else
        fprintf(fp, "Operating Range Analysis from %s\n", CP.Program());
    fprintf(fp, "Date: %s\n", datestring());
    fprintf(fp, "File: %s\n", filename);
    if (!tnames) {
        fprintf(fp, "Parameter 1: %s\n", "value1");
        fprintf(fp, "Parameter 2: %s\n", "value2");
    }
    else {
        // Print the substituted parameter names.
        char param1[128], param2[128];
        *param1 = '\0';
        *param2 = '\0';
        sDataVec *d = out_plot->find_vec(tnames->value());
        if (d && d->isreal()) {
            int len = d->length();
            sDataVec *n1 = out_plot->find_vec(tnames->n1());
            sDataVec *n2 = out_plot->find_vec(tnames->n2());
            if (n2 && n2->isreal()) {
                int ii = (int)n2->realval(0);
                if (ii >= 0 && ii < len)
                    sprintf(param1, "%s[%d]", tnames->value(), ii);
            }
            // N1 has precedence if N1 = N2
            if (n1 && n1->isreal()) {
                int ii = (int)n1->realval(0);
                if (ii >= 0 && ii < len)
                    sprintf(param2, "%s[%d]", tnames->value(), ii);
            }
        }
        if (!*param1)
            strcpy(param1, tnames->value1());
        if (!*param2)
            strcpy(param2, tnames->value2());
        fprintf(fp, "Parameter 1: %s\n", param1);
        fprintf(fp, "Parameter 2: %s\n", param2);
    }

    // Map the file name to the current plot name.
    sCHECKprms::setMfilePlotname(buf1, Sp.CurPlot()->type_name());

    Sp.SetVar(kw_mplot_cur, buf1);
    return (fp);
}


// Create a vector and a scale for the plot which gives the computed
// boundary of the pass region.  This happens only when checkiterate
// is nonzero - when the min and max are found.
//
void
sCHECKprms::set_rangevec()
{
    int j, cnt = 0;
    int n = 2*((2*ch_step1+1) + (2*ch_step2+1));
    double *d = new double[n];
    double *s = new double[n];
    sDataVec *v = out_plot->find_vec(OPLO1);
    sDataVec *t = 0;
    if (v) {
        t = v;
        for (j = -ch_step2; j <= ch_step2; j++) {
            if (v->realval(j + ch_step2) != 0.0) {
                d[cnt] = ch_val2 + j*ch_delta2;
                s[cnt] = v->realval(j + ch_step2);
                cnt++;
            }
        }
    }
    v = out_plot->find_vec(OPHI2);
    if (v) {
        t = v;
        for (j = -ch_step1; j <= ch_step1; j++) {
            if (v->realval(j + ch_step1) != 0.0) {
                d[cnt] = v->realval(j + ch_step1);
                s[cnt] = ch_val1 + j*ch_delta1;
                cnt++;
            }
        }
    }
    v = out_plot->find_vec(OPHI1);
    if (v) {
        t = v;
        for (j = ch_step2; j >= -ch_step2; j--) {
            if (v->realval(j + ch_step2) != 0.0) {
                d[cnt] = ch_val2 + j*ch_delta2;
                s[cnt] = v->realval(j + ch_step2);
                cnt++;
            }
        }
    }
    v = out_plot->find_vec(OPLO2);
    if (v) {
        t = v;
        for (j = ch_step1; j >= -ch_step1; j--) {
            if (v->realval(j + ch_step1) != 0.0) {
                d[cnt] = v->realval(j + ch_step1);
                s[cnt] = ch_val1 + j*ch_delta1;
                cnt++;
            }
        }
    }

    if (t && cnt > 0) {
        sDataVec *scale = new sDataVec(lstring::copy(OPSCALE), t->flags(),
            cnt, t->units(), s);
        scale->set_defcolor(t->defcolor());
        scale->set_gridtype(t->gridtype());
        scale->set_plottype(t->plottype());
        scale->newperm();

        v = new sDataVec(lstring::copy(OPVEC), t->flags(), cnt, t->units(), d);
        v->set_defcolor(t->defcolor());
        v->set_gridtype(t->gridtype());
        v->set_plottype(t->plottype());
        v->set_scale(scale);
        v->newperm();
    }
    else {
        delete [] s;
        delete [] d;
    }
}


#define SPAN 10

// Find the operating range of value1, using val as the starting point.
// Results are recorded in OPLO1 and OPHI1.  Returns true if error.
//
bool
sCHECKprms::findrange1(double val, int offset, bool dolower, bool doupper)
{
    if (offset < 0)
        return (true);
    if (val != 0.0) {
        sDataVec *d = out_plot->find_vec(OPHI1);
        if (doupper && d && offset < d->length()) {
            if (!ch_batchmode)
                TTY.printf("Computing v1 upper limit...\n");
            double value2 = ch_val2;
            double value1 = val;
            double delta = fabs(.5*value1);
            int i;
            for (i = 0; i < SPAN; i++) {
                value1 += delta;
                ch_no_output = true;
                trial(0, 0, value1, value2);
                ch_no_output = false;
                if (ch_fail) {
                    value1 -= delta;
                    if (findext1(ch_iterno, &value1, value2, -delta))
                        return (true);
                    d->set_realval(offset, value1);
                    break;
                }
            }
            if (i == SPAN) {
                GRpkgIf()->ErrPrintf(ET_WARN,
                    "could not find upper v1 limit.\n");
                d->set_realval(offset, 0.0);
            }
        }
        d = out_plot->find_vec(OPLO1);
        if (dolower && d && offset < d->length()) {
            if (!ch_batchmode)
                TTY.printf("Computing v1 lower limit...\n");
            double value2 = ch_val2;
            double value1 = val;
            double delta = fabs(.5*value1);
            int i;
            for (i = 0; i < SPAN; i++) {
                value1 -= delta;
                ch_no_output = true;
                trial(0, 0, value1, value2);
                ch_no_output = false;
                if (ch_fail) {
                    value1 += delta;
                    if (findext1(ch_iterno, &value1, value2, delta))
                        return (true);
                    d->set_realval(offset, value1);
                    break;
                }
            }
            if (i == SPAN) {
                GRpkgIf()->ErrPrintf(ET_WARN,
                    "could not find lower v1 limit.\n");
                d->set_realval(offset, 0.0);
            }
        }
    }
    return (false);
}


// Find the operating range of value2, using val as the starting point.
// Results are recorded in OPLO2 and OPHI2.  Returns true if error.
//
bool
sCHECKprms::findrange2(double val, int offset, bool dolower, bool doupper)
{
    if (offset < 0)
        return (true);
    if (val != 0.0) {
        sDataVec *d = out_plot->find_vec(OPHI2);
        if (doupper && d && offset < d->length()) {
            if (!ch_batchmode)
                TTY.printf("Computing v2 upper limit...\n");
            double value2 = val;
            double value1 = ch_val1;
            double delta = fabs(.5*value2);
            int i;
            for (i = 0; i < SPAN; i++) {
                value2 += delta;
                ch_no_output = true;
                trial(0, 0, value1, value2);
                ch_no_output = false;
                if (ch_fail) {
                    value2 -= delta;
                    if (findext2(ch_iterno, value1, &value2, -delta))
                        return (true);
                    d->set_realval(offset, value2);
                    break;
                }
            }
            if (i == SPAN) {
                GRpkgIf()->ErrPrintf(ET_WARN,
                    "could not find upper v2 limit.\n");
                d->set_realval(offset, 0.0);
            }
        }
        d = out_plot->find_vec(OPLO2);
        if (dolower && d && offset < d->length()) {
            if (!ch_batchmode)
                TTY.printf("Computing v2 lower limit...\n");
            double value2 = val;
            double value1 = ch_val1;
            double delta = fabs(.5*value2);
            int i;
            for (i = 0; i < SPAN; i++) {
                value2 -= delta;
                ch_no_output = true;
                trial(0, 0, value1, value2);
                ch_no_output = false;
                if (ch_fail) {
                    value2 += delta;
                    if (findext2(ch_iterno, value1, &value2, delta))
                        return (true);
                    d->set_realval(offset, value2);
                    break;
                }
            }
            if (i == SPAN) {
                GRpkgIf()->ErrPrintf(ET_WARN,
                    "could not find lower v2 limit.\n");
                d->set_realval(offset, 0.0);
            }
        }
    }
    return (false);
}


// Iterate to the edge of the operating region along row.
// If delta > 0, find lower boundary, else find upper.
// Return true if error or pause.
//
bool
sCHECKprms::findext1(int itno, double *value1, double value2, double delta)
{
    delta *= .5;
    *value1 -= delta;
    ch_no_output = true;
    while (itno--) {
        initInput(*value1, value2);
        out_cir->resetTrial(ch_names != 0);
        ToolBar()->SuppressUpdate(true);
        out_cir->set_keep_deferred(true);
        int error = out_cir->runTrial();
        out_cir->set_keep_deferred(false);
        ToolBar()->SuppressUpdate(false);
        if (error < 0) {
            ch_pause = true;
            ch_no_output = false;
            return (true);
        }
        if (error == E_ITERLIM) {
            // Failed to converge, take this as a fail point.
            // 
            ch_fail = true;
            error = OK;
        }
        else if (error != OK) {
            ch_nogo = true;
            ch_no_output = false;
            return (true);
        }
        delta *= .5;
        if (!ch_fail)
            *value1 -= delta;
        else
            *value1 += delta;
    }
    ch_no_output = false;
    return (false);
}


// Iterate to the edge of the operating region along column.
// If delta > 0, find lower boundary, else find upper.
// Return true if error or pause.
//
bool
sCHECKprms::findext2(int itno, double value1, double *value2, double delta)
{
    delta *= .5;
    *value2 -= delta;
    ch_no_output = true;
    while (itno--) {
        initInput(value1, *value2);
        out_cir->resetTrial(ch_names != 0);
        ToolBar()->SuppressUpdate(true);
        out_cir->set_keep_deferred(true);
        int error = out_cir->runTrial();
        out_cir->set_keep_deferred(false);
        ToolBar()->SuppressUpdate(false);
        if (error < 0) {
            ch_pause = true;
            ch_no_output = false;
            return (true);
        }
        if (error == E_ITERLIM) {
            // Failed to converge, take this as a fail point.
            // 
            ch_fail = true;
            error = OK;
        }
        else if (error != OK) {
            ch_nogo = true;
            ch_no_output = false;
            return (true);
        }
        delta *= .5;
        if (!ch_fail)
            *value2 -= delta;
        else
            *value2 += delta;
    }
    ch_no_output = false;
    return (false);
}


void
sCHECKprms::addpoint(int i, int j, bool fail)
{
    sDimen *dm = out_plot->dimensions();
    dm->add_point(i, j, fail, (2*ch_step1 + 1), (2*ch_step2 + 1));
}


// If we turned on mplot during a pause, plot the existing data.
//
void
sCHECKprms::plot()
{
    int num1 = 2*ch_step1 + 1;
    for (int j = -ch_step2; j <= ch_step2; j++) {
        char *rowflags = ch_flags + (j + ch_step2)*num1;
        for (int i = -ch_step1; i <= ch_step1; i++, rowflags++) {
            if (!*rowflags)
                continue;
            GP.MpWhere(ch_graphid, i, j);
            GP.MpMark(ch_graphid, (*rowflags - 2));
        }
    }
}
// End of sCHECKprms functions.


sOUTcontrol::~sOUTcontrol()
{
    if (out_cir)
        out_cir->clearDeferred();
}

