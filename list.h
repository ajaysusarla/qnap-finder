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

#include <sys/types.h>
#include <netinet/in.h>

typedef struct _QnapResponseNode {
	int len;
	in_addr_t addr;
	char *hostip;
	char *msg;
	struct _QnapResponseNode *next;
} NODE;

typedef struct _NASBoxList {
	NODE *first;
	NODE *last;
} LIST;


LIST * create_list(void);
void free_list(LIST *list);
NODE * create_node(void);
int add_node(LIST *list, NODE *new);
