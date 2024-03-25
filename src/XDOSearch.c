/* xdo search implementation
 *
 * Lets you search windows by a query
 */
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/extensions/XTest.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#include "xdo.h"
#include "XDOSearch.h"

/** *********************************************************************
 ** This method consumed by app :
 **     largest_window_with_name().
 **/
int doXDOWindowSearch(const xdo_t* xdo, const xdo_search_t* search,
    Window** windowlist_ret, unsigned int* nwindows_ret) {

    unsigned int windowlist_size = 100;
    *nwindows_ret = 0;
    *windowlist_ret = (Window *)calloc(sizeof(Window), windowlist_size); // wv

    /* TODO(sissel): Support multiple screens */
    if (search->searchmask & SEARCH_SCREEN) {
        Window root = RootWindow(xdo->xdpy, search->screen);
        if (checkXDOWindowMatch(xdo, root, search)) {
            (*windowlist_ret)[*nwindows_ret] = root;
            (*nwindows_ret)++;
            /* Don't have to check for size bounds here because
             * we start with array size 100 */
        }

        /* Start with depth=1 since we already covered the root windows */
        findXDOWindowMatch(xdo, root, search, windowlist_ret, nwindows_ret,
            &windowlist_size, 1);
    } else {
        const int screencount = ScreenCount(xdo->xdpy);
        for (int i = 0; i < screencount; i++) {
            Window root = RootWindow(xdo->xdpy, i);
            if (checkXDOWindowMatch(xdo, root, search)) {
                (*windowlist_ret)[*nwindows_ret] = root;
                (*nwindows_ret)++;
                /* Don't have to check for size bounds here because
                 * we start with array size 100 */
            }

            /* Start with depth=1 since we already covered the root windows */
            findXDOWindowMatch(xdo, root, search, windowlist_ret,
                nwindows_ret, &windowlist_size, 1);
        }
    }

    return XDO_SUCCESS;
}

/** *********************************************************************
 ** This internal method ...
 **/
int checkXDOWindowMatch(const xdo_t* xdo, Window wid,
    const xdo_search_t* search) {

    // Get regex compilers for string matching.
    // No cpp std::string.
    regex_t windowTitleRegex;
    regex_t windowClassRegex;
    regex_t windowClassNameRegex;
    regex_t windowNameRegex;
    if (!initXDOSearchRegex(search->title, &windowTitleRegex) ||
        !initXDOSearchRegex(search->winclass, &windowClassRegex) ||
        !initXDOSearchRegex(search->winclassname, &windowClassNameRegex) ||
        !initXDOSearchRegex(search->winname, &windowNameRegex)) {
        regfree(&windowTitleRegex);
        regfree(&windowClassRegex);
        regfree(&windowClassNameRegex);
        regfree(&windowNameRegex);
        return False;
    }

    bool pid_ok = true;
    bool title_ok = true, class_ok = true, classname_ok = true, name_ok = true;

    int pid_want, title_want, name_want, class_want,
        classname_want, desktop_want;

    pid_want = search->searchmask & SEARCH_PID;
    desktop_want = search->searchmask & SEARCH_DESKTOP;

    title_want = search->searchmask & SEARCH_TITLE;
    class_want = search->searchmask & SEARCH_CLASS;
    classname_want = search->searchmask & SEARCH_CLASSNAME;
    name_want = search->searchmask & SEARCH_NAME;

    regfree(&windowTitleRegex);
    regfree(&windowClassRegex);
    regfree(&windowClassNameRegex);
    regfree(&windowNameRegex);

    switch (search->require) {
        case SEARCH_ALL:
            return pid_ok && title_ok && name_ok && class_ok &&
                   classname_ok;

        case SEARCH_ANY:
            return ((pid_want && pid_ok) || (title_want && title_ok) ||
                       (name_want && name_ok) || (class_want && class_ok) ||
                       (classname_want && classname_ok));
    }
}

/** *********************************************************************
 ** This internal method ...
 **/
int initXDOSearchRegex(const char *pattern, regex_t *re) {
    int ret;
    if (pattern == NULL) {
        regcomp(re, "^$", REG_EXTENDED | REG_ICASE);
        return True;
    }

    ret = regcomp(re, pattern, REG_EXTENDED | REG_ICASE);
    if (ret != 0) {
        fprintf(stderr, "Failed to compile regex (return code %d):"
            " '%s'\n", ret, pattern);
        return False;
    }
    return True;
}

/** *********************************************************************
 ** Query for children of 'wid'. For each child, check match.
 ** We want to do a breadth-first search.
 **
 ** If match, add to list.
 ** If over limit, break.
 ** Recurse.
 **/
void findXDOWindowMatch(const xdo_t* xdo, Window window,
    const xdo_search_t* search, Window** windowlist_ret,
    unsigned int* nwindows_ret, unsigned int* windowlist_size,
    int current_depth) {

    /* Break early, if we have enough windows already. */
    if (search->limit > 0 &&
        *nwindows_ret >= search->limit) {
        return;
    }

    /* Break if too deep */
    if (search->max_depth != -1 &&
        current_depth > search->max_depth) {
        return;
    }

    /* Break if XQueryTree fails.
     * TODO(sissel): report an error? */
    Window dummy;
    Window* children;
    unsigned int i, nchildren;
    Status success = XQueryTree(xdo->xdpy,
        window, &dummy, &dummy, &children, &nchildren);

    if (!success) {
        if (children != NULL) {
            XFree(children);
        }
        return;
    }

    /* Breadth first, check all children for matches */
    for (int i = 0; i < nchildren; i++) {
        Window child = children[i];
        if (!checkXDOWindowMatch(xdo, child, search)) {
            continue;
        }

        (*windowlist_ret)[*nwindows_ret] = child;
        (*nwindows_ret)++;

        if (search->limit > 0 &&
            *nwindows_ret >= search->limit) {
            break;
        }

        if (*windowlist_size == *nwindows_ret) {
            *windowlist_size *= 2;
            *windowlist_ret = (Window *) realloc(*windowlist_ret,
                *windowlist_size * sizeof(Window));
        }
    }

    /* Now check children-children */
    if (search->max_depth == -1 ||
        (current_depth + 1) <= search->max_depth) {
        for (int i = 0; i < nchildren; i++) {
            findXDOWindowMatch(xdo, children[i],
                search, windowlist_ret, nwindows_ret,
                windowlist_size, current_depth + 1);
        }
    }

    if (children != NULL) {
        XFree(children);
    }
}
