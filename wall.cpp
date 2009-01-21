/**
 *
 * Compiz wall plugin
 *
 * wall.cpp
 *
 * Copyright (c) 2006 Robert Carr <racarr@beryl-project.org>
 *
 * Authors:
 * Robert Carr <racarr@beryl-project.org>
 * Dennis Kasprzyk <onestone@opencompositing.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <GL/glu.h>

#include <core/atoms.h>

#include "wall.h"

#define PI 3.14159265359f
#define VIEWPORT_SWITCHER_SIZE 100
#define ARROW_SIZE 33

#define WIN_X(w) ((w)->attrib.x - (w)->input.left)
#define WIN_Y(w) ((w)->attrib.y - (w)->input.top)
#define WIN_W(w) ((w)->width + (w)->input.left + (w)->input.right)
#define WIN_H(w) ((w)->height + (w)->input.top + (w)->input.bottom)

#define getColorRGBA(name) \
    r = optionGet##name##Red() / 65535.0f;\
    g = optionGet##name##Green() / 65535.0f; \
    b = optionGet##name##Blue() / 65535.0f; \
    a = optionGet##name##Alpha() / 65535.0f

#define sigmoid(x) (1.0f / (1.0f + exp (-5.5f * 2 * ((x) - 0.5))))
#define sigmoidProgress(x) ((sigmoid (x) - sigmoid (0)) / \
			    (sigmoid (1) - sigmoid (0)))

COMPIZ_PLUGIN_20081216 (wall, WallPluginVTable);

void
WallScreen::clearCairoLayer (cairo_t *cr)
{
    cairo_save (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);
    cairo_restore (cr);
}

void
WallScreen::drawSwitcherBackground ()
{
    cairo_t         *cr;
    cairo_pattern_t *pattern;
    float           outline = 2.0f;
    int             width, height, radius;
    float           r, g, b, a;
    unsigned int    i, j;

    cr = switcherContext.cr;
    clearCairoLayer (cr);

    width = switcherContext.width - outline;
    height = switcherContext.height - outline;

    cairo_save (cr);
    cairo_translate (cr, outline / 2.0f, outline / 2.0f);

    /* set the pattern for the switcher's background */
    pattern = cairo_pattern_create_linear (0, 0, width, height);
    getColorRGBA (BackgroundGradientBaseColor);
    cairo_pattern_add_color_stop_rgba (pattern, 0.00f, r, g, b, a);
    getColorRGBA (BackgroundGradientHighlightColor);
    cairo_pattern_add_color_stop_rgba (pattern, 0.65f, r, g, b, a);
    getColorRGBA (BackgroundGradientShadowColor);
    cairo_pattern_add_color_stop_rgba (pattern, 0.85f, r, g, b, a);
    cairo_set_source (cr, pattern);

    /* draw the border's shape */
    radius = optionGetEdgeRadius ();
    if (radius)
    {
        cairo_arc (cr, radius, radius, radius, PI, 1.5f * PI);
        cairo_arc (cr, radius + width - 2 * radius,
                   radius, radius, 1.5f * PI, 2.0 * PI);
        cairo_arc (cr, width - radius, height - radius, radius, 0,  PI / 2.0f);
        cairo_arc (cr, radius, height - radius, radius,  PI / 2.0f, PI);
    }
    else
        cairo_rectangle (cr, 0, 0, width, height);

    cairo_close_path (cr);

    /* apply pattern to background... */
    cairo_fill_preserve (cr);

    /* ... and draw an outline */
    cairo_set_line_width (cr, outline);
    getColorRGBA (OutlineColor);
    cairo_set_source_rgba (cr, r, g, b, a);
    cairo_stroke (cr);

    cairo_pattern_destroy (pattern);
    cairo_restore (cr);

    cairo_save (cr);
    for (i = 0; i < screen->vpSize ().height (); i++)
    {
        cairo_translate (cr, 0.0, viewportBorder);
        cairo_save (cr);
        for (j = 0; j < screen->vpSize ().width (); j++)
        {
            cairo_translate (cr, viewportBorder, 0.0);

            /* this cuts a hole into our background */
            cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
            cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
            cairo_rectangle (cr, 0, 0, viewportWidth, viewportHeight);

            cairo_fill_preserve (cr);
            cairo_set_operator (cr, CAIRO_OPERATOR_XOR);
            cairo_fill (cr);

            cairo_translate (cr, viewportWidth, 0.0);
        }
        cairo_restore(cr);

        cairo_translate (cr, 0.0, viewportHeight);
    }
    cairo_restore (cr);
}

void
WallScreen::drawThumb ()
{
    cairo_t         *cr;
    cairo_pattern_t *pattern;
    float           r, g, b, a;
    float           outline = 2.0f;
    int             width, height;

    cr = thumbContext.cr;
    clearCairoLayer (cr);

    width  = thumbContext.width - outline;
    height = thumbContext.height - outline;

    cairo_translate (cr, outline / 2.0f, outline / 2.0f);

    pattern = cairo_pattern_create_linear (0, 0, width, height);
    getColorRGBA (ThumbGradientBaseColor);
    cairo_pattern_add_color_stop_rgba (pattern, 0.0f, r, g, b, a);
    getColorRGBA (ThumbGradientHighlightColor);
    cairo_pattern_add_color_stop_rgba (pattern, 1.0f, r, g, b, a);

    /* apply the pattern for thumb background */
    cairo_set_source (cr, pattern);
    cairo_rectangle (cr, 0, 0, width, height);
    cairo_fill_preserve (cr);

    cairo_set_line_width (cr, outline);
    getColorRGBA (OutlineColor);
    cairo_set_source_rgba (cr, r, g, b, a);
    cairo_stroke (cr);

    cairo_pattern_destroy (pattern);

    cairo_restore (cr);
}

void
WallScreen::drawHighlight ()
{
    cairo_t         *cr;
    cairo_pattern_t *pattern;
    int             width, height;
    float           r, g, b, a;
    float           outline = 2.0f;

    cr = highlightContext.cr;
    clearCairoLayer (cr);

    width  = highlightContext.width - outline;
    height = highlightContext.height - outline;

    cairo_translate (cr, outline / 2.0f, outline / 2.0f);

    pattern = cairo_pattern_create_linear (0, 0, width, height);
    getColorRGBA (ThumbHighlightGradientBaseColor);
    cairo_pattern_add_color_stop_rgba (pattern, 0.0f, r, g, b, a);
    getColorRGBA (ThumbHighlightGradientShadowColor);
    cairo_pattern_add_color_stop_rgba (pattern, 1.0f, r, g, b, a);

    /* apply the pattern for thumb background */
    cairo_set_source (cr, pattern);
    cairo_rectangle (cr, 0, 0, width, height);
    cairo_fill_preserve (cr);

    cairo_set_line_width (cr, outline);
    getColorRGBA (OutlineColor);
    cairo_set_source_rgba (cr, r, g, b, a);
    cairo_stroke (cr);

    cairo_pattern_destroy (pattern);

    cairo_restore (cr);
}

void
WallScreen::drawArrow ()
{
    cairo_t *cr;
    float   outline = 2.0f;
    float   r, g, b, a;

    cr = arrowContext.cr;
    clearCairoLayer (cr);

    cairo_translate (cr, outline / 2.0f, outline / 2.0f);

    /* apply the pattern for thumb background */
    cairo_set_line_width (cr, outline);

    /* draw top part of the arrow */
    getColorRGBA (ArrowBaseColor);
    cairo_set_source_rgba (cr, r, g, b, a);
    cairo_move_to (cr, 15, 0);
    cairo_line_to (cr, 30, 30);
    cairo_line_to (cr, 15, 24.5);
    cairo_line_to (cr, 15, 0);
    cairo_fill (cr);

    /* draw bottom part of the arrow */
    getColorRGBA (ArrowShadowColor);
    cairo_set_source_rgba (cr, r, g, b, a);
    cairo_move_to (cr, 15, 0);
    cairo_line_to (cr, 0, 30);
    cairo_line_to (cr, 15, 24.5);
    cairo_line_to (cr, 15, 0);
    cairo_fill (cr);

    /* draw the arrow outline */
    getColorRGBA (OutlineColor);
    cairo_set_source_rgba (cr, r, g, b, a);
    cairo_move_to (cr, 15, 0);
    cairo_line_to (cr, 30, 30);
    cairo_line_to (cr, 15, 24.5);
    cairo_line_to (cr, 0, 30);
    cairo_line_to (cr, 15, 0);
    cairo_stroke (cr);

    cairo_restore (cr);
}

void
WallScreen::setupCairoContext (WallCairoContext *context)
{
    XRenderPictFormat *format;
    Screen            *xScreen;
    int               width, height;

    xScreen = ScreenOfDisplay (screen->dpy (), screen->screenNum ());

    width = context->width;
    height = context->height;

    format = XRenderFindStandardFormat (screen->dpy (), PictStandardARGB32);

    context->pixmap = XCreatePixmap (screen->dpy (), screen->root (),
                                     width, height, 32);

    context->texture = GLTexture::bindPixmapToTexture (context->pixmap,
                                                       width, height, 32);
    if (context->texture.empty ())
    {
        screen->logMessage ("wall", CompLogLevelError,
                            "Couldn't create cairo context for switcher");
    }

    context->surface =
        cairo_xlib_surface_create_with_xrender_format (screen->dpy (),
                                                       context->pixmap,
                                                       xScreen, format,
                                                       width, height);

    context->cr = cairo_create (context->surface);
    clearCairoLayer (context->cr);
}

void
WallScreen::destroyCairoContext (WallCairoContext *context)
{
    if (context->cr)
        cairo_destroy (context->cr);

    if (context->surface)
        cairo_surface_destroy (context->surface);

	context->texture.clear ();

    if (context->pixmap)
        XFreePixmap (screen->dpy (), context->pixmap);
}

bool
WallScreen::checkDestination (unsigned int destX,
                              unsigned int destY)
{
    CompPoint point;
    CompSize  size;

    point = screen->vp ();
    size = screen->vpSize ();

    if (point.x () - destX < 0)
        return false;

    if (point.x () - destX >= size.width ())
        return false;

    if (point.y () - destY >= size.height ())
        return false;

    if (point.y () - destY < 0)
        return false;

    return true;
}

void
WallScreen::releaseMoveWindow ()
{
    CompWindow *window;

    window = screen->findWindow (moveWindow);
    if (window)
        window->syncPosition ();

    moveWindow = 0;
}

void
WallScreen::computeTranslation (float *x,
                                float *y)
{
    float dx, dy, elapsed, duration;

    duration = optionGetSlideDuration () * 1000.0;
    if (duration != 0.0)
        elapsed = 1.0 - (timer / duration);
    else
        elapsed = 1.0;

    if (elapsed < 0.0)
        elapsed = 0.0;
    if (elapsed > 1.0)
        elapsed = 1.0;

    /* Use temporary variables to you can pass in &ps->cur_x */
    dx = (gotoX - curPosX) * elapsed + curPosX;
    dy = (gotoY - curPosY) * elapsed + curPosY;

    *x = dx;
    *y = dy;
}

/* movement remainder that gets ignored for direction calculation */
#define IGNORE_REMAINDER 0.05

void
WallScreen::determineMovementAngle ()
{
    int angle;
    float dx, dy;

    dx = gotoX - curPosX;
    dy = gotoY - curPosY;

    if (dy > IGNORE_REMAINDER)
        angle = (dx > IGNORE_REMAINDER) ? 135 :
                (dx < -IGNORE_REMAINDER) ? 225 : 180;
    else if (dy < -IGNORE_REMAINDER)
        angle = (dx > IGNORE_REMAINDER) ? 45 :
                (dx < -IGNORE_REMAINDER) ? 315 : 0;
    else
        angle = (dx > IGNORE_REMAINDER) ? 90 :
                (dx < -IGNORE_REMAINDER) ? 270 : -1;

    direction = angle;
}

bool
WallScreen::moveViewport (int    x,
                          int    y,
                          Window moveWin)
{
    if (!x && !y)
        return false;

    if (screen->otherGrabExist ("move", "switcher", "group-drag", "wall", 0))
        return false;

    if (!checkDestination (x, y))
        return false;

    if (moveWindow != moveWin)
    {
        CompWindow *w;

        releaseMoveWindow ();
        w = screen->findWindow (moveWin);
        if (w)
        {
            if (!(w->type () & (CompWindowTypeDesktopMask |
                             CompWindowTypeDockMask)))
            {
                if (!(w->state () & CompWindowStateStickyMask))
                {
                    moveWindow = w->id ();
                    moveWindowX = w->x ();
                    moveWindowY = w->y ();
                    w->raise ();
                }
            }
        }
    }

    if (!moving)
    {
        curPosX = screen->vp ().x ();
        curPosY = screen->vp ().y ();
    }
    gotoX = screen->vp ().x () - x;
    gotoY = screen->vp ().y () - y;

    determineMovementAngle ();

    if (!grabIndex)
        grabIndex = screen->pushGrab (screen->invisibleCursor (), "wall");

    screen->moveViewport (x, y, true);

    moving          = true;
    focusDefault    = true;
    boxOutputDevice = screen->outputDeviceForPoint (pointerX, pointerY);

    if (optionGetShowSwitcher ())
        boxTimeout = optionGetPreviewTimeout () * 1000;
    else
        boxTimeout = 0;

    timer = optionGetSlideDuration () * 1000;

    cScreen->damageScreen ();

    return true;
}

void
WallScreen::handleEvent (XEvent *event)
{
    switch (event->type) {
        case ClientMessage:
        if (event->xclient.message_type == Atoms::desktopViewport)
        {
            int        dx, dy;

            if (screen->otherGrabExist ("switcher", "wall", 0))
                break;

            dx = event->xclient.data.l[0] / screen->width() - screen->vp().x();
            dy = event->xclient.data.l[1] / screen->height() - screen->vp().y();

            if (!dx && !dy)
                break;

            moveViewport (-dx, -dy, None);
        }
        break;
    }
    screen->handleEvent (event);
}

void
WallWindow::activate ()
{
    WALL_SCREEN (screen);

    if (window->placed () && !screen->otherGrabExist ("wall", "switcher", 0))
    {
        int dx, dy;
		CompPoint viewport;

		viewport = window->defaultViewport ();
		dx = viewport.x ();
		dy = viewport.y ();

        dx -= screen->vp ().x ();
        dy -= screen->vp ().y ();
	
        if (dx || dy)
        {
            ws->moveViewport (-dx, -dy, None);
            ws->focusDefault = false;
        }
    }

    window->activate ();
}

void
WallScreen::checkAmount (unsigned int  dx,
                         unsigned int  dy,
                         int *amountX,
                         int *amountY)
{
    CompPoint point;
    CompSize  size;

    point = screen->vp ();
    size = screen->vpSize ();

    *amountX = -dx;
    *amountY = -dy;

    if (optionGetAllowWraparound ())
    {
        if ((point.x () + dx) < 0)
            *amountX = -(size.width () + dx);
        else if ((point.x () + dx) >= size.width ())
            *amountX = size.width () - dx;

        if ((point.y () + dy) < 0)
            *amountY = -(size.height () + dy);
        else if ((point.y () + dy) >= size.height ())
            *amountY = size.height () - dy;
    }
}

bool
WallScreen::initiate (CompAction         *action,
                      CompAction::State  state,
		      CompOption::Vector &options,
		      Direction          dir,
		      bool               withWin)
{
    int dx = 0, dy = 0, amountX, amountY;

    CompPoint point;
    CompSize size;
    Window win = None;
	
    point = screen->vp ();
    size = screen->vpSize ();

    switch (dir) {
	case Up:
	    dy = -1;
	    checkAmount (dx, dy, &amountX, &amountY);
	    break;
	case Down:
	    dy = 1;
	    checkAmount (dx, dy, &amountX, &amountY);
	    break;
	case Left:
	    dx = -1;
	    checkAmount (dx, dy, &amountX, &amountY);
	    break;
	case Right:
	    dx = 1;
	    checkAmount (dx, dy, &amountX, &amountY);
	    break;
	case Next:
	    if ((point.x () == size.width () - 1) && (point.y () == size.height () - 1))
	    {
		amountX = -(size.width () - 1);
		amountY = -(size.height () - 1);
	    }
	    else if (point.x () == size.width () - 1)
	    {
		amountX = -(size.width () - 1);
		amountY = 1;
	    }
	    else
	    {
		amountX = 1;
		amountY = 0;
	    }

	    break;
	case Prev:
	    if ((point.x () == 0) && (point.y () == 0))
	    {
		amountX = size.width () - 1;
		amountY = size.height () - 1;
	    }
	    else if (point.x () == 0)
	    {
		amountX = size.width () - 1;
		amountY = -1;
	    }
	    else
	    {
		amountX = -1;
		amountY = 0;
	    }
	    break;
    }

    if (withWin)
	win = CompOption::getIntOptionNamed (options, "window", 0);

    if (!moveViewport (amountX, amountY, win))
        return true;

    if (state & CompAction::StateInitKey)
        action->setState (action->state () | CompAction::StateTermKey);

    if (state & CompAction::StateInitButton)
        action->setState (action->state () | CompAction::StateTermButton);

    showPreview = optionGetShowSwitcher ();

    return true;
}

bool
WallScreen::terminate (CompAction         *action,
                       CompAction::State   state,
                       CompOption::Vector &options)
{
    if (showPreview)
    {
        showPreview = FALSE;
	    cScreen->damageScreen ();
    }

    if (action)
        action->setState (action->state () & ~(CompAction::StateTermKey | CompAction::StateTermButton));

    return false;
}

bool
WallScreen::initiateFlip (Direction         direction,
                          CompAction::State state)
{
    int dx, dy;
    int amountX, amountY;

    if (screen->otherGrabExist ("wall", "move", "group-drag", 0))
        return false;

    if (state & CompAction::StateInitEdgeDnd)
    {
        if (!optionGetEdgeflipDnd ())
            return false;

        if (screen->otherGrabExist ("wall", 0))
            return false;
    }
    else if (screen->otherGrabExist ("wall", "group-drag", 0))
    {
        /* not wall or group means move */
        if (!optionGetEdgeflipMove ())
            return false;
    }
    else if (screen->otherGrabExist ("wall", 0))
    {
        /* move was ruled out before, so we have group */
        if (!optionGetEdgeflipDnd ())
            return false;
    }
    else if (!optionGetEdgeflipPointer ())
        return false;

    switch (direction) {
    case Left:
        dx = -1; dy = 0;
        break;
    case Right:
        dx = 1; dy = 0;
        break;
    case Up:
        dx = 0; dy = -1;
        break;
    case Down:
        dx = 0; dy = 1;
        break;
    default:
        dx = 0; dy = 0;
        break;
    }

    checkAmount (dx, dy, &amountX, &amountY);
    if (moveViewport (amountX, amountY, None))
    {
        int offsetX, offsetY;
        int warpX, warpY;

        if (dx < 0)
        {
            offsetX = screen->width () - 10;
            warpX = pointerX + screen->width ();
        }
        else if (dx > 0)
        {
            offsetX = 1- screen->width ();
            warpX = pointerX - screen->width ();
        }
        else
        {
            offsetX = 0;
            warpX = lastPointerX;
        }

        if (dy < 0)
        {
            offsetY = screen->height () - 10;
            warpY = pointerY + screen->height ();
        }
        else if (dy > 0)
        {
            offsetY = 1- screen->height ();
            warpY = pointerY - screen->height ();
        }
        else
        {
            offsetY = 0;
            warpY = lastPointerY;
        }

        screen->warpPointer (offsetX, offsetY);
        lastPointerX = warpX;
        lastPointerY = warpY;
    }

    return true;
}

inline void
wallDrawQuad (GLTexture::Matrix *matrix, BOX *box)
{
    glTexCoord2f (COMP_TEX_COORD_X (*matrix, box->x1),
		  COMP_TEX_COORD_Y (*matrix, box->y2));
    glVertex2i (box->x1, box->y2);
    glTexCoord2f (COMP_TEX_COORD_X (*matrix, box->x2),
		  COMP_TEX_COORD_Y (*matrix, box->y2));
    glVertex2i (box->x2, box->y2);
    glTexCoord2f (COMP_TEX_COORD_X (*matrix, box->x2),
		  COMP_TEX_COORD_Y (*matrix, box->y1));
    glVertex2i (box->x2, box->y1);
    glTexCoord2f (COMP_TEX_COORD_X (*matrix, box->x1),
		  COMP_TEX_COORD_Y (*matrix, box->y1));
    glVertex2i (box->x1, box->y1);
}

void
WallScreen::drawCairoTextureOnScreen ()
{
    float             centerX, centerY;
    float             width, height;
    float             topLeftX, topLeftY;
    float             border;
    unsigned int      i, j;
    GLTexture::Matrix matrix;
    BOX               box;

    CompOutput::vector &outputDevs = screen->outputDevs ();
    CompOutput output = outputDevs[boxOutputDevice];

    glDisableClientState (GL_TEXTURE_COORD_ARRAY);
    glEnable (GL_BLEND);

    centerX = output.x1 () + (output.width () / 2.0f);
    centerY = output.y1 () + (output.height () / 2.0f);

    border = (float) viewportBorder;
    width  = (float) switcherContext.width;
    height = (float) switcherContext.height;

    topLeftX = centerX - floor (width / 2.0f);
    topLeftY = centerY - floor (height / 2.0f);

    firstViewportX = topLeftX + border;
    firstViewportY = topLeftY + border;

    if (!moving)
    {
        double left, timeout;

        timeout = optionGetPreviewTimeout () * 1000.0f;
        left    = (timeout > 0) ? (float) boxTimeout / timeout : 1.0f;

        if (left < 0)
            left = 0.0f;
        else if (left > 0.5)
            left = 1.0f;
        else
            left = 2 * left;

        glScreen->setTexEnvMode (GL_MODULATE);

        glColor4f (left, left, left, left);
        glTranslatef (0.0f,0.0f, -(1 - left));

        mSzCamera = -(1 - left);
    }
    else
        mSzCamera = 0.0f;

    /* draw background */

    matrix = switcherContext.texture[0]->matrix ();
    matrix.x0 -= topLeftX * matrix.xx;
    matrix.y0 -= topLeftY * matrix.yy;

    box.x1 = topLeftX;
    box.x2 = box.x1 + width;
    box.y1 = topLeftY;
    box.y2 = box.y1 + height;

    switcherContext.texture[0]->enable (GLTexture::Fast);
    glBegin (GL_QUADS);
    wallDrawQuad (&matrix, &box);
    glEnd ();
    switcherContext.texture[0]->disable ();

    /* draw thumb */
    width = (float) thumbContext.width;
    height = (float) thumbContext.height;

    thumbContext.texture[0]->enable (GLTexture::Fast);
    glBegin (GL_QUADS);
    for (i = 0; i < screen->vpSize ().width (); i++)
    {
        for (j = 0; j < screen->vpSize ().height (); j++)
        {
            if (i == gotoX && j == gotoY && moving)
                continue;

            box.x1 = i * (width + border);
            box.x1 += topLeftX + border;
            box.x2 = box.x1 + width;
            box.y1 = j * (height + border);
            box.y1 += topLeftY + border;
            box.y2 = box.y1 + height;

            matrix = thumbContext.texture[0]->matrix ();
            matrix.x0 -= box.x1 * matrix.xx;
            matrix.y0 -= box.y1 * matrix.yy;

            wallDrawQuad (&matrix, &box);
        }
    }
    glEnd ();
    thumbContext.texture[0]->disable ();

    if (moving || showPreview)
    {
        /* draw highlight */
        int   aW, aH;

        box.x1 = screen->vp ().x () * (width + border) + topLeftX + border;
        box.x2 = box.x1 + width;
        box.y1 = screen->vp ().y () * (height + border) + topLeftY + border;
        box.y2 = box.y1 + height;

        matrix = highlightContext.texture[0]->matrix ();
        matrix.x0 -= box.x1 * matrix.xx;
        matrix.y0 -= box.y1 * matrix.yy;

        highlightContext.texture[0]->enable (GLTexture::Fast);
        glBegin (GL_QUADS);
        wallDrawQuad (&matrix, &box);
        glEnd ();
        highlightContext.texture[0]->disable ();

        /* draw arrow */
        if (direction >= 0)
        {
            arrowContext.texture[0]->enable (GLTexture::Fast);
            aW = arrowContext.width;
            aH = arrowContext.height;

            /* if we have a viewport preview we just paint the
               arrow outside the switcher */
            if (optionGetMiniscreen ())
            {
                width  = (float) switcherContext.width;
                height = (float) switcherContext.height;

                switch (direction)
                {
                    /* top left */
                    case 315:
                        box.x1 = topLeftX - aW - border;
                        box.y1 = topLeftY - aH - border;
                        break;
                    /* up */
                    case 0:
                        box.x1 = topLeftX + width / 2.0f - aW / 2.0f;
                        box.y1 = topLeftY - aH - border;
                        break;
                    /* top right */
                    case 45:
                        box.x1 = topLeftX + width + border;
                        box.y1 = topLeftY - aH - border;
                        break;
                    /* right */
                    case 90:
                        box.x1 = topLeftX + width + border;
                        box.y1 = topLeftY + height / 2.0f - aH / 2.0f;
                        break;
                    /* bottom right */
                    case 135:
                        box.x1 = topLeftX + width + border;
                        box.y1 = topLeftY + height + border;
                        break;
                    /* down */
                    case 180:
                        box.x1 = topLeftX + width / 2.0f - aW / 2.0f;
                        box.y1 = topLeftY + height + border;
                        break;
                    /* bottom left */
                    case 225:
                        box.x1 = topLeftX - aW - border;
                        box.y1 = topLeftY + height + border;
                        break;
                    /* left */
                    case 270:
                        box.x1 = topLeftX - aW - border;
                        box.y1 = topLeftY + height / 2.0f - aH / 2.0f;
                        break;
                    default:
                        break;
                }
            }
            else
            {
                /* arrow is visible (no preview is painted over it) */
                box.x1 = screen->vp().x() * (width + border) + topLeftX + border;
                box.x1 += width / 2 - aW / 2;
                box.y1 = screen->vp().y() * (height + border) + topLeftY + border;
                box.y1 += height / 2 - aH / 2;
            }

            box.x2 = box.x1 + aW;
            box.y2 = box.y1 + aH;

            glTranslatef (box.x1 + aW / 2, box.y1 + aH / 2, 0.0f);
            glRotatef (direction, 0.0f, 0.0f, 1.0f);
            glTranslatef (-box.x1 - aW / 2, -box.y1 - aH / 2, 0.0f);

            matrix = arrowContext.texture[0]->matrix ();
            matrix.x0 -= box.x1 * matrix.xx;
            matrix.y0 -= box.y1 * matrix.yy;

            glBegin (GL_QUADS);
            wallDrawQuad (&matrix, &box);
            glEnd ();

            arrowContext.texture[0]->disable ();
        }
    }

    glDisable (GL_BLEND);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    glScreen->setTexEnvMode (GL_REPLACE);
    glColor4usv (defaultColor);
}

bool
WallScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
                           const GLMatrix            &matrix,
                           const CompRegion          &region,
                           CompOutput                *output,
                           unsigned int               mask)
{
    bool status;

    transform = NoTransformation;
    if (moving)
        mask |= PAINT_SCREEN_TRANSFORMED_MASK |
                PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    status = glScreen->glPaintOutput (attrib, matrix, region, output, mask);

    if (optionGetShowSwitcher () &&
        (moving || showPreview || boxTimeout) &&
        (output->id () == boxOutputDevice ||
	 output == &screen->fullscreenOutput ()))
    {
        GLMatrix sMatrix = matrix;

        sMatrix.toScreenSpace (output, -DEFAULT_Z_CAMERA);

        glPushMatrix ();
        glLoadMatrixf (sMatrix.getMatrix ());

        drawCairoTextureOnScreen ();

        glPopMatrix ();

        if (optionGetMiniscreen ())
        {
            int  i, j;
            float mw, mh;

            mw = viewportWidth;
            mh = viewportHeight;

            transform = MiniScreen;
            mSAttribs.xScale = mw / screen->width ();
            mSAttribs.yScale = mh / screen->height ();
            mSAttribs.opacity = OPAQUE * (1.0 + mSzCamera);
            mSAttribs.saturation = COLOR;

            for (j = 0; j < screen->vpSize ().height (); j++)
            {
                for (i = 0; i < screen->vpSize ().width (); i++)
                {
                    float        mx, my;
                    unsigned int msMask;

                    mx = firstViewportX +
                         (i * (viewportWidth + viewportBorder));
                    my = firstViewportY + 
                         (j * (viewportHeight + viewportBorder));

                    mSAttribs.xTranslate = mx / output->width ();
                    mSAttribs.yTranslate = -my / output->height ();

                    mSAttribs.brightness = 0.4f * BRIGHT;

                    if (i == screen->vp ().x () && j == screen->vp ().y ()
                        && moving)
                    {
                        mSAttribs.brightness = BRIGHT;
                    }

                    if ((boxTimeout || showPreview) &&
                        !moving && i == screen->vp ().x () &&
                        j == screen->vp ().y ())
                    {
                        mSAttribs.brightness = BRIGHT;
                    }

                    cScreen->setWindowPaintOffset ((screen->vp ().x () - i) *
                                                   screen->vpSize ().width (),
                                                   (screen->vp ().y () - j) *
                                                   screen->vpSize ().height ());

                    msMask = mask | PAINT_SCREEN_TRANSFORMED_MASK;
                    glScreen->glPaintTransformedOutput (attrib, matrix,
                                                       region, output, msMask);

                }
            }
            transform = NoTransformation;
            cScreen->setWindowPaintOffset (0, 0);
        }
    }

    return status;
}

void
WallScreen::preparePaint (int ms)
{
    if (!moving && !showPreview && boxTimeout)
        boxTimeout -= ms;

    if (timer)
        timer -= ms;

    if (moving)
    {
        computeTranslation (&curPosX, &curPosY);

        if (moveWindow)
        {
            CompWindow *window;

            window = screen->findWindow (moveWindow);
            if (window)
            {
                float dx, dy;

                dx = gotoX - curPosX;
                dy = gotoY - curPosY;

                window->moveToViewportPosition (moveWindowX - screen->vpSize ().width () * dx,
                                                moveWindowY - screen->vpSize ().height () * dy,
                                                true);
            }
        }
    }

    if (moving && curPosX == gotoX && curPosY == gotoY)
    {
        moving = false;
        timer  = 0;

        if (moveWindow)
            releaseMoveWindow ();
        else if (focusDefault)
        {
            /* only focus default window if switcher is not active */
            if (!screen->grabExist ("switcher"))
                screen->focusDefaultWindow ();
        }
    }

    cScreen->preparePaint (ms);
}

void
WallScreen::glPaintTransformedOutput (const GLScreenPaintAttrib &attrib,
                                      const GLMatrix            &matrix,
                                      const CompRegion          &region,
                                      CompOutput                *output,
                                      unsigned int               mask)
{
    bool clear = (mask & PAINT_SCREEN_CLEAR_MASK);
    CompPoint point;
    CompSize  size;

    point = screen->vp ();
    size = screen->vpSize ();

    if (transform == MiniScreen)
    {
        GLMatrix sMatrix = matrix;

        mask &= ~PAINT_SCREEN_CLEAR_MASK;

        /* move each screen to the correct output position */

	    sMatrix.translate (-(float) output->x1 () /
                           (float) output->width (),
                           (float) output->y1 () /
                           (float) output->height (), 0.0f);
        sMatrix.translate (0.0f, 0.0f, -DEFAULT_Z_CAMERA);

        sMatrix.translate (mSAttribs.xTranslate,
                           mSAttribs.yTranslate,
                           mSzCamera);

        /* move origin to top left */
        sMatrix.translate (-0.5f, 0.5f, 0.0f);
        sMatrix.scale (mSAttribs.xScale, mSAttribs.yScale, 1.0);

        /* revert prepareXCoords region shift.
           Now all screens display the same */
        sMatrix.translate (0.5f, 0.5f, DEFAULT_Z_CAMERA);
        sMatrix.translate ((float) output->x1 () / (float) output->width (),
                           -(float) output->y2 () /
                           (float) output->height (), 0.0f);

        glScreen->glPaintTransformedOutput (attrib, sMatrix,
                                            screen->region (), output, mask);
        return;
    }

    if (!moving)
        glScreen->glPaintTransformedOutput (attrib, matrix,
                                            region, output, mask);

    mask &= ~PAINT_SCREEN_CLEAR_MASK;

    if (moving)
    {
        ScreenTransformation oldTransform = transform;
        GLMatrix             sMatrix = matrix;
        float                xTranslate, yTranslate;
        float                px, py;
        int                  tx, ty;
        Bool                 movingX, movingY;

        if (clear)
            glScreen->clearTargetOutput (GL_COLOR_BUFFER_BIT);

        transform  = Sliding;
        currOutput = output;

        px = curPosX;
        py = curPosY;

        movingX = ((int) floor (px)) != ((int) ceil (px));
        movingY = ((int) floor (py)) != ((int) ceil (py));

        if (movingY)
        {
            ty = ceil (py) - point.y ();
            yTranslate = fmod (py, 1) - 1;

            sMatrix.translate (0.0f, yTranslate, 0.0f);

            if (movingX)
            {
                tx = ceil (px) - point.x ();
                xTranslate = 1 - fmod (px, 1);

                cScreen->setWindowPaintOffset ((point.x () - ceil(px)) *
                                               size.width (),
                                               (point.y () - ceil(py)) *
                                               size.height ());
		
                sMatrix.translate (xTranslate, 0.0f, 0.0f);

                glScreen->glPaintTransformedOutput (attrib, sMatrix,
                                             screen->region (), output, mask);

                sMatrix.translate (-xTranslate, 0.0f, 0.0f);
            }

            tx = floor (px) - point.x ();
            xTranslate = -fmod (px, 1);

            cScreen->setWindowPaintOffset ((point.x () - floor(px)) *
                                           size.width (),
                                           (point.y () - ceil(py)) *
                                           size.height ());

            sMatrix.translate (xTranslate, 0.0f, 0.0f);

            glScreen->glPaintTransformedOutput (attrib, sMatrix,
                                             screen->region (), output, mask);
            sMatrix.translate (-xTranslate, -yTranslate, 0.0f);
        }

        ty = floor (py) - point.y ();
        yTranslate = fmod (py, 1);

        sMatrix.translate (0.0f, yTranslate, 0.0f);

        if (movingX)
        {
            tx = ceil (px) - point.x ();
            xTranslate = 1 - fmod (px, 1);

            cScreen->setWindowPaintOffset ((point.x () - ceil(px)) *
                                           size.width (),
                                           (point.y () - floor(py)) *
                                           size.height ());

            sMatrix.translate (xTranslate, 0.0f, 0.0f);

            glScreen->glPaintTransformedOutput (attrib, sMatrix,
                                             screen->region (), output, mask);

            sMatrix.translate (-xTranslate, 0.0f, 0.0f);
        }

        tx = floor (px) - point.x ();
        xTranslate = -fmod (px, 1);

        cScreen->setWindowPaintOffset ((point.x () - floor(px)) *
                                       size.width (),
                                       (point.y () - floor(py)) *
                                       size.height ());

        sMatrix.translate (xTranslate, 0.0f, 0.0f);
        glScreen->glPaintTransformedOutput (attrib, sMatrix,
                                             screen->region (), output, mask);

        cScreen->setWindowPaintOffset (0, 0);
        transform = oldTransform;
    }
}

bool
WallWindow::glPaint (const GLWindowPaintAttrib &attrib,
                     const GLMatrix            &matrix,
                     const CompRegion          &region,
                     unsigned int               mask)
{
    bool       status;

    WALL_SCREEN (screen);

    if (ws->transform == MiniScreen)
    {
        GLWindowPaintAttrib pA = attrib;

        pA.opacity    = attrib.opacity *
                        ((float) ws->mSAttribs.opacity / OPAQUE);
        pA.brightness = attrib.brightness *
                        ((float) ws->mSAttribs.brightness / BRIGHT);
        pA.saturation = attrib.saturation *
                        ((float) ws->mSAttribs.saturation / COLOR);

        if (!pA.opacity || !pA.brightness)
            mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;

        status = glWindow->glPaint (pA, matrix, region, mask);
    }
    else if (ws->transform == Sliding)
    {
        GLMatrix wMatrix;

        if (!isSliding)
        {
            wMatrix.translate (-0.5f, -0.5f, -DEFAULT_Z_CAMERA);
            wMatrix.scale (1.0f / ws->currOutput->width (),
                           -1.0f / ws->currOutput->height (),
                           1.0f);
            wMatrix.translate (-ws->currOutput->x1 (), -ws->currOutput->y1 (),
                               0.0f);
            mask |= PAINT_WINDOW_TRANSFORMED_MASK;
        }
        else
        {
            wMatrix = matrix;
        }

        status = glWindow->glPaint (attrib, wMatrix, region, mask);
    }
    else
    {
        status = glWindow->glPaint (attrib, matrix, region, mask);
    }

    return status;
}

void
WallScreen::donePaint ()
{
    if (moving || showPreview || boxTimeout)
    {
		boxTimeout = MAX (0, boxTimeout);
		cScreen->damageScreen ();
    }

    if (!moving && !showPreview && grabIndex)
    {
		screen->removeGrab (grabIndex, NULL);
		grabIndex = 0;
    }

	cScreen->donePaint ();
}

void
WallScreen::createCairoContexts (bool initial)
{
    int width, height;

    viewportWidth = VIEWPORT_SWITCHER_SIZE *
                    (float) optionGetPreviewScale () / 100.0f;
    viewportHeight = viewportWidth * (float) screen->height () /
		     (float) screen->width ();
    viewportBorder = optionGetBorderWidth ();

    width  = screen->vpSize ().width () * (viewportWidth + viewportBorder) +
             viewportBorder;
    height = screen->vpSize ().height () * (viewportHeight + viewportBorder) +
             viewportBorder;

    destroyCairoContext (&switcherContext);
    switcherContext.width = width;
    switcherContext.height = height;
    setupCairoContext (&switcherContext);
    drawSwitcherBackground ();

    destroyCairoContext (&thumbContext);
    thumbContext.width = viewportWidth;
    thumbContext.height = viewportHeight;
    setupCairoContext (&thumbContext);
    drawThumb ();

    destroyCairoContext (&highlightContext);
    highlightContext.width = viewportWidth;
    highlightContext.height = viewportHeight;
    setupCairoContext (&highlightContext);
    drawHighlight ();

    if (initial)
    {
        arrowContext.width = ARROW_SIZE;
        arrowContext.height = ARROW_SIZE;
        setupCairoContext (&arrowContext);
        drawArrow ();
    }
}


void
WallScreen::optionChanged (CompOption *opt, WallOptions::Options num)
{

    switch(num)
    {
	case WallOptions::OutlineColor:
	    drawSwitcherBackground ();
	    drawHighlight ();
	    drawThumb ();
	    break;

	case WallOptions::EdgeRadius:
	case WallOptions::BackgroundGradientBaseColor:
	case WallOptions::BackgroundGradientHighlightColor:
	case WallOptions::BackgroundGradientShadowColor:
	    drawSwitcherBackground ();
	    break;

	case WallOptions::BorderWidth:
	case WallOptions::PreviewScale:
	    createCairoContexts (false);
	    break;

	case WallOptions::ThumbGradientBaseColor:
	case WallOptions::ThumbGradientHighlightColor:
	    drawThumb ();
	    break;

	case WallOptions::ThumbHighlightGradientBaseColor:
	case WallOptions::ThumbHighlightGradientShadowColor:
	    drawHighlight ();
	    break;

	case WallOptions::ArrowBaseColor:
	case WallOptions::ArrowShadowColor:
	    drawArrow ();
	    break;

	case WallOptions::NoSlideMatch:

	    foreach (CompWindow *w, screen->windows ())
	    {
		WALL_WINDOW (w);
		ww->isSliding = !optionGetNoSlideMatch ().evaluate (w);
	    }
	    break;

	default:
	    break;
    }
}

bool
WallScreen::setOptionForPlugin (const char        *plugin,
                                const char        *name,
                                CompOption::Value &value)
{
    bool status = screen->setOptionForPlugin (plugin, name, value);

    if (strcmp (plugin, "core") == 0)
    {
        if (strcmp (name, "hsize") == 0 || strcmp (name, "vsize") == 0)
        {
            createCairoContexts (false);
        }
    }

    return status;
}

void
WallScreen::matchExpHandlerChanged ()
{
    screen->matchExpHandlerChanged ();

    foreach (CompWindow *w, screen->windows ())
    {
	WALL_WINDOW (w);
	ww->isSliding = !optionGetNoSlideMatch ().evaluate (w);
    }
}

void
WallScreen::matchPropertyChanged (CompWindow *window)
{
    WALL_WINDOW (window);

    screen->matchPropertyChanged (window);

    ww->isSliding = !optionGetNoSlideMatch ().evaluate (window);
}

WallScreen::WallScreen (CompScreen *screen) :
    PrivateHandler <WallScreen, CompScreen> (screen),
    WallOptions (wallVTable->getMetadata ()),
    cScreen (CompositeScreen::get (screen)),
    glScreen (GLScreen::get (screen)),
    moving (false),
    showPreview (false),
    direction (-1),
    boxTimeout (0),
    grabIndex (0),
    timer (0),
    moveWindow (None),
    focusDefault (true),
    transform (NoTransformation)
{
    ScreenInterface::setHandler (screen);
    CompositeScreenInterface::setHandler (cScreen);
    GLScreenInterface::setHandler (glScreen);

    memset (&switcherContext, 0, sizeof (WallCairoContext));
    memset (&thumbContext, 0, sizeof (WallCairoContext));
    memset (&highlightContext, 0, sizeof (WallCairoContext));
    memset (&arrowContext, 0, sizeof (WallCairoContext));
    createCairoContexts (true);

#define setAction(action, dir, win) \
    optionSet##action##Initiate (boost::bind (&WallScreen::initiate, this, _1, _2, _3, dir, win)); \
    optionSet##action##Terminate (boost::bind (&WallScreen::terminate, this, _1, _2, _3))

#define setFlipAction(action, dir) \
    optionSet##action##Initiate (boost::bind (&WallScreen::initiateFlip, this, dir, _2))
    
    setAction (LeftKey, Left, false);
    setAction (RightKey, Right, false);
    setAction (UpKey, Up, false);
    setAction (DownKey, Down, false);
    setAction (NextKey, Next, false);
    setAction (PrevKey, Prev, false);
    setAction (LeftButton, Left, false);
    setAction (RightButton, Right, false);
    setAction (UpButton, Up, false);
    setAction (DownButton, Down, false);
    setAction (NextButton, Next, false);
    setAction (PrevButton, Prev, false);
    setAction (LeftWindowKey, Left, true);
    setAction (RightWindowKey, Right, true);
    setAction (UpWindowKey, Up, true);
    setAction (DownWindowKey, Down, true);


    setFlipAction (FlipLeftEdge, Left);
    setFlipAction (FlipRightEdge, Right);
    setFlipAction (FlipUpEdge, Up);
    setFlipAction (FlipDownEdge, Down);

#define setNotify(func) \
    optionSet##func##Notify (boost::bind (&WallScreen::optionChanged, this, _1, _2))

    setNotify (EdgeRadius);
    setNotify (BorderWidth);
    setNotify (PreviewScale);
    setNotify (OutlineColor);
    setNotify (BackgroundGradientBaseColor);
    setNotify (BackgroundGradientHighlightColor);
    setNotify (BackgroundGradientShadowColor);
    setNotify (ThumbGradientBaseColor);
    setNotify (ThumbGradientHighlightColor);
    setNotify (ThumbHighlightGradientBaseColor);
    setNotify (ThumbHighlightGradientShadowColor);
    setNotify (ArrowBaseColor);
    setNotify (ArrowShadowColor);
    setNotify (NoSlideMatch);
}

WallScreen::~WallScreen ()
{
    destroyCairoContext (&switcherContext);
    destroyCairoContext (&thumbContext);
    destroyCairoContext (&highlightContext);
    destroyCairoContext (&arrowContext);
}

WallWindow::WallWindow (CompWindow *window) :
    PrivateHandler <WallWindow, CompWindow> (window),
    window (window),
    glWindow (GLWindow::get (window)),
    isSliding (!WallScreen::get (screen)->optionGetNoSlideMatch ().evaluate (window))
{
}

bool
WallPluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
        return false;

    return true;
}

