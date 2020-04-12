/*
 * qnap-finder
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"


LIST * create_list(void)
{
        LIST *list;

        list = (LIST *)malloc(sizeof(LIST));
        if (list) {
                list->first = NULL;
                list->last = NULL;
                list->num_entries = 0;
        }

        return list;
}

void free_list(LIST *list)
{
        if (list) {
                NODE *tmp, *prev;

                tmp = list->first;
                while (tmp != NULL) {
                        prev = tmp;
                        tmp = tmp->next;

                        if (prev->hostip)
                                free(prev->hostip);
                        if (prev->msg)
                                free(prev->msg);
                        free(prev);
                }
                free(tmp);
                list->first = NULL;
                list->last = NULL;

                free(list);
        }
}

NODE * create_node(void)
{
        NODE *node;

        node = (NODE *)malloc(sizeof(NODE));
        if (node) {
                node->len = 0;
                node->hostip = NULL;
                node->msg = NULL;
                node->next = NULL;
        }

        return node;
}

int add_node(LIST *list, NODE *new)
{
        int ret;

        if (!list || !new) {
                ret = -1;
                goto end;
        }

        if (list->first == list->last &&
            list->last == NULL) {
                list->first = list->last = new;
                list->first->next = NULL;
                list->last->next = NULL;
        } else {
                list->last->next = new;
                list->last = new;
                list->last->next = NULL;
        }

        list->num_entries += 1;
        ret = 0;

end:
        return ret;
}
