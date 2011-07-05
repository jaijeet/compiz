/*
 * Copyright © 2011 Canonical Ltd.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Canonical Ltd. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Canonical Ltd. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * CANONICAL, LTD. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL CANONICAL, LTD. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Authored by: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#ifndef _COMPIZ_TEST_TIMER_H
#define _COMPIZ_TEST_TIMER_H

#include <glibmm/main.h>
#include <core/timer.h>
#include <privatetimeouthandler.h>
#include <privatetimeoutsource.h>
#include <iostream>
#include <boost/bind.hpp>

class CompTimerTest
{
public:

    CompTimerTest ();
    virtual ~CompTimerTest ();

    Glib::RefPtr <Glib::MainContext> mc;
    Glib::RefPtr <Glib::MainLoop> ml;
    Glib::RefPtr <CompTimeoutSource> ts;
    std::list <CompTimer *> timers;

    virtual void precallback () = 0;

    int lastTimerTriggered;
};

class CompTimerTestCallbacks :
    public CompTimerTest
{
public:

    void precallback ();
    bool cb (int timernum);
};

class CompTimerTestDiffs :
    public CompTimerTest
{
public:

    void precallback ();
    bool cb (int timernum, CompTimer *t1, CompTimer *t2, CompTimer *t3);
};

class CompTimerTestSetValues :
    public CompTimerTest
{
public:

    void precallback ();
    bool cb (int timernum);
};

class CompTimerTestSetCalling :
    public CompTimerTest
{
public:

    void precallback ();
    bool cb (int timernum, CompTimer *t1, CompTimer *t2, CompTimer *t3);
};

#endif
