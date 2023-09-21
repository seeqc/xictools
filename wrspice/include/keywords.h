
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

#ifndef KEYWORDS_H
#define KEYWORDS_H

#include "spnumber/hash.h"


//
// Data types for internal variables.
//

// Basic keyword.
struct Kword
{
    Kword() { }
    Kword(const char *w, const char *d)
        { word = w; descr = d; type = VTYP_NONE; }
    virtual ~Kword () { }
    virtual void print(sLstr*);

    const char *word;
    const char *descr;
    VTYPenum type;
};

struct userEnt
{
    virtual ~userEnt() { }

    virtual void callback(bool, variable*) = 0;
};

struct KWent : public Kword
{
    KWent(const char *w=0, VTYPenum t=VTYP_NONE, const char *d=0)
        { set(w, t, 0.0, 0.0, d); lastv1 = lastv2 = 0; ent = 0; }
    void set(const char *w, VTYPenum t, double mi, double mx, const char *d)
        { word = w; type = t; min = mi; max = mx; descr = d; }
    void init()
        { Sp.Options()->add(word, this);
        CP.AddKeyword(CT_VARIABLES, word); }
    virtual void callback(bool isset, variable *v)
        { if (ent) ent->callback(isset, v); }

    double min, max;     // for numeric variables
    const char *lastv1;  // previous value set, for graphics
    const char *lastv2;  // previous value set, for graphics
    userEnt *ent;        // used in graphics subsystem
};

class cKeyWords
{
public:
    void initDatabase();

    Kword *pstyles(int i)     { return (KWpstyles[i]); }
    Kword *gstyles(int i)     { return (KWgstyles[i]); }
    Kword *scale(int i)       { return (KWscale[i]); }
    Kword *plot(int i)        { return (KWplot[i]); }
    Kword *color(int i)       { return (KWcolor[i]); }
    Kword *dbargs(int i)      { return (KWdbargs[i]); }
    Kword *debug(int i)       { return (KWdebug[i]); }
    Kword *ft(int i)          { return (KWft[i]); }
    Kword *level(int i)       { return (KWlevel[i]); }
    Kword *units(int i)       { return (KWunits[i]); }
    Kword *spec(int i)        { return (KWspec[i]); }
    Kword *cmds(int i)        { return (KWcmds[i]); }
    Kword *shell(int i)       { return (KWshell[i]); }
    Kword *step(int i)        { return (KWstep[i]); }
    Kword *method(int i)      { return (KWmethod[i]); }
    Kword *optmerge(int i)    { return (KWoptmerge[i]); }
    Kword *parhier(int i)     { return (KWparhier[i]); }
    Kword *sim(int i)         { return (KWsim[i]); }

    static Kword *KWpstyles[];
    static Kword *KWgstyles[];
    static Kword *KWscale[];
    static Kword *KWplot[];
    static Kword *KWcolor[];
    static Kword *KWdbargs[];
    static Kword *KWdebug[];
    static Kword *KWft[];
    static Kword *KWlevel[];
    static Kword *KWunits[];
    static Kword *KWspec[];
    static Kword *KWcmds[];
    static Kword *KWshell[];
    static Kword *KWstep[];
    static Kword *KWmethod[];
    static Kword *KWoptmerge[];
    static Kword *KWparhier[];
    static Kword *KWsim[];
};

extern cKeyWords KW;

#endif // KEYWORDS_H

