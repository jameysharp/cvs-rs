/*
 * Copyright (C) 2004-2005 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 2004-2005 Derek Price, Ximbiot <http://ximbiot.com>,
 *                                  and others.
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS source distribution.
 *
 * This module uses the hash.c module to implement a stack.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "cvs.h"
#include <assert.h>



static void
do_push (List *stack, void *elem, int isstring)
{
    Node *p = getnode();

    if (isstring)
	p->key = elem;
    else
	p->data = elem;

    addnode(stack, p);
}



void
push (List *stack, void *elem)
{
    do_push (stack, elem, 0);
}



void
push_string (List *stack, char *elem)
{
    do_push (stack, elem, 1);
}



static void *
do_pop (List *stack, int isstring)
{
    void *elem;

    if (isempty (stack)) return NULL;

    if (isstring)
    {
	elem = stack->list->prev->key;
	stack->list->prev->key = NULL;
    }
    else
    {
	elem = stack->list->prev->data;
	stack->list->prev->data = NULL;
    }

    delnode (stack->list->prev);
    return elem;
}



void *
pop (List *stack)
{
    return do_pop (stack, 0);
}



char *
pop_string (List *stack)
{
    return do_pop (stack, 1);
}



static void
do_unshift (List *stack, void *elem, int isstring)
{
    Node *p = getnode();

    if (isstring)
	p->key = elem;
    else
	p->data = elem;

    addnode_at_front(stack, p);
}



void
unshift (List *stack, void *elem)
{
    do_unshift (stack, elem, 0);
}



void
unshift_string (List *stack, char *elem)
{
    do_unshift (stack, elem, 1);
}



static void *
do_shift (List *stack, int isstring)
{
    void *elem;

    if (isempty (stack)) return NULL;

    if (isstring)
    {
	elem = stack->list->next->key;
	stack->list->next->key = NULL;
    }
    else
    {
	elem = stack->list->next->data;
	stack->list->next->data = NULL;
    }
    delnode (stack->list->next);
    return elem;
}



void *
shift (List *stack)
{
    return do_shift (stack, 0);
}



char *
shift_string (List *stack)
{
    return do_shift (stack, 1);
}



int
isempty (List *stack)
{
    if (stack->list == stack->list->next)
	return 1;
    return 0;
}



/* Copy the elements of ARGV to the beginning of a List.  For example,
 * With an array containing "a", "b", "c", and a List containing "e", "f", "g",
 * in those orders, calling this function would cause the list to contain "a",
 * "b", "c", "d", "e", "f".
 *
 * INPUT
 *   list		The List to modify.
 *   argv		The array to insert the contents of.
 *   argc		The number of elements in ARGV.
 */
void
unshift_string_array (List *list, char **argv, int argc)
{
    while (argc-- > 0)
	unshift_string (list, xstrdup (argv[argc]));
}



List *
init_string_list (char **argv, int argc)
{
    List *newlist = getlist();
    unshift_string_array (newlist, argv, argc);
    return newlist;
}
