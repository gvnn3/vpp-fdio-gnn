/*
 *------------------------------------------------------------------
 * nhrp_api.c - nhrp api
 *
 * Copyright (c) 2016 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *------------------------------------------------------------------
 */

#include <vnet/vnet.h>
#include <vlibmemory/api.h>

#include <vnet/api_errno.h>
#include <vnet/nhrp/nhrp.h>
#include <vnet/ip/ip_types_api.h>
#include <vnet/fib/fib_table.h>

/* define message IDs */
#include <vnet/format_fns.h>
#include <vnet/nhrp/nhrp.api_enum.h>
#include <vnet/nhrp/nhrp.api_types.h>

static u32 nhrp_base_msg_id;
#define REPLY_MSG_ID_BASE nhrp_base_msg_id

#include <vlibapi/api_helper_macros.h>

static void
vl_api_nhrp_entry_add_del_t_handler (vl_api_nhrp_entry_add_del_t * mp)
{
  vl_api_nhrp_entry_add_del_reply_t *rmp;
  ip46_address_t peer, nh;
  int rv;

  VALIDATE_SW_IF_INDEX ((&mp->entry));

  ip_address_decode (&mp->entry.peer, &peer);
  ip_address_decode (&mp->entry.nh, &nh);

  if (mp->is_add)
    rv = nhrp_entry_add (ntohl (mp->entry.sw_if_index), &peer,
			 ntohl (mp->entry.nh_table_id), &nh);
  else
    rv = nhrp_entry_del (ntohl (mp->entry.sw_if_index), &peer);

  BAD_SW_IF_INDEX_LABEL;

  REPLY_MACRO (VL_API_NHRP_ENTRY_ADD_DEL_REPLY);
}

typedef struct vl_api_nhrp_send_t_
{
  vl_api_registration_t *reg;
  u32 context;
} vl_api_nhrp_send_t;

static walk_rc_t
vl_api_nhrp_send_one (index_t nei, void *arg)
{
  vl_api_nhrp_details_t *mp;
  vl_api_nhrp_send_t *ctx = arg;
  const nhrp_entry_t *ne;

  mp = vl_msg_api_alloc (sizeof (*mp));
  clib_memset (mp, 0, sizeof (*mp));
  mp->_vl_msg_id = ntohs (VL_API_NHRP_DETAILS + REPLY_MSG_ID_BASE);
  mp->context = ctx->context;

  ne = nhrp_entry_get (nei);

  ip_address_encode (&ne->ne_key->nk_peer, IP46_TYPE_ANY, &mp->entry.peer);
  ip_address_encode (&ne->ne_nh.fp_addr, IP46_TYPE_ANY, &mp->entry.nh);
  mp->entry.nh_table_id =
    htonl (fib_table_get_table_id (ne->ne_fib_index, ne->ne_nh.fp_proto));
  mp->entry.sw_if_index = htonl (ne->ne_key->nk_sw_if_index);

  vl_api_send_msg (ctx->reg, (u8 *) mp);

  return (WALK_CONTINUE);
}

static void
vl_api_nhrp_dump_t_handler (vl_api_nhrp_dump_t * mp)
{
  vl_api_registration_t *reg;

  reg = vl_api_client_index_to_registration (mp->client_index);
  if (!reg)
    return;

  vl_api_nhrp_send_t ctx = {
    .reg = reg,
    .context = mp->context,
  };

  nhrp_walk (vl_api_nhrp_send_one, &ctx);
}

/*
 * nhrp_api_hookup
 * Add vpe's API message handlers to the table.
 * vlib has already mapped shared memory and
 * added the client registration handlers.
 * See .../vlib-api/vlibmemory/memclnt_vlib.c:memclnt_process()
 */
#include <vnet/nhrp/nhrp.api.c>

static clib_error_t *
nhrp_api_hookup (vlib_main_t * vm)
{
  nhrp_base_msg_id = setup_message_id_table ();

  return (NULL);
}

VLIB_API_INIT_FUNCTION (nhrp_api_hookup);

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */
