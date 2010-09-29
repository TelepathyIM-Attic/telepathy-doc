/*
 * mcp-account-manager-example.h
 *
 * McpAccountManagerExample - an example Mission Control plugin
 *
 * Copyright (C) 2010 Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *    Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

#include <mission-control-plugins/mission-control-plugins.h>

#ifndef __MCP_ACCOUNT_MANAGER_EXAMPLE_H__
#define __MCP_ACCOUNT_MANAGER_EXAMPLE_H__

G_BEGIN_DECLS

#define MCP_TYPE_ACCOUNT_MANAGER_EXAMPLE	(mcp_account_manager_example_get_type ())
#define MCP_ACCOUNT_MANAGER_EXAMPLE(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), MCP_TYPE_ACCOUNT_MANAGER_EXAMPLE, McpAccountManagerExample))
#define MCP_ACCOUNT_MANAGER_EXAMPLE_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), MCP_TYPE_ACCOUNT_MANAGER_EXAMPLE, McpAccountManagerExampleClass))
#define MCP_IS_ACCOUNT_MANAGER_EXAMPLE(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MCP_TYPE_ACCOUNT_MANAGER_EXAMPLE))
#define MCP_IS_ACCOUNT_MANAGER_EXAMPLE_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), MCP_TYPE_ACCOUNT_MANAGER_EXAMPLE))
#define MCP_ACCOUNT_MANAGER_EXAMPLE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MCP_TYPE_ACCOUNT_MANAGER_EXAMPLE, McpAccountManagerExampleClass))

typedef struct _McpAccountManagerExample McpAccountManagerExample;
typedef struct _McpAccountManagerExampleClass McpAccountManagerExampleClass;

struct _McpAccountManagerExample
{
  GObject parent;
};

struct _McpAccountManagerExampleClass
{
  GObjectClass parent_class;
};

GType mcp_account_manager_example_get_type (void);

G_END_DECLS

#endif
