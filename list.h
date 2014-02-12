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

/* Message type*/
typedef enum {
	NODE_TYPE_NONE,
	NODE_TYPE_BRIEF,
	NODE_TYPE_DETAIL
} node_type_t;

typedef struct _QnapResponseNode {
	int len;
	in_addr_t addr;
	node_type_t ntype;
	char *hostip;
	unsigned char *msg;
	struct _QnapResponseNode *next;
} NODE;

typedef struct _NASBoxList {
	NODE *first;
	NODE *last;
	int num_entries;
} LIST;


LIST * create_list(void);
void free_list(LIST *list);
NODE * create_node(void);
int add_node(LIST *list, NODE *new);
