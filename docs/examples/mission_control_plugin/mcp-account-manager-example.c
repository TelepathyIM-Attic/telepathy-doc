/*
 * mcp-account-manager-example.c
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

#include <telepathy-glib/telepathy-glib.h>

#include "mcp-account-manager-example.h"

#define DEBUG g_debug

#define PLUGIN_NAME "ExamplePlugin"
#define PLUGIN_PRIORITY (MCP_ACCOUNT_STORAGE_PLUGIN_PRIO_KEYRING + 10)
#define PLUGIN_DESCRIPTION "Provide an example Telepathy Account"
#define PLUGIN_PROVIDER "au.id.madeley.danielle.ExamplePlugin"

#define ACCOUNT_NAME "gabble/jabber/example0"

#define GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), MCP_TYPE_ACCOUNT_MANAGER_EXAMPLE, McpAccountManagerExamplePrivate))

static void account_storage_iface_init (McpAccountStorageIface *iface);

G_DEFINE_TYPE_WITH_CODE (McpAccountManagerExample,
    mcp_account_manager_example,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (MCP_TYPE_ACCOUNT_STORAGE,
      account_storage_iface_init));

typedef struct _McpAccountManagerExamplePrivate McpAccountManagerExamplePrivate;
struct _McpAccountManagerExamplePrivate
{
  gboolean ready;
};


static void
mcp_account_manager_example_dispose (GObject *self)
{
  McpAccountManagerExamplePrivate *priv = GET_PRIVATE (self);

  G_OBJECT_CLASS (mcp_account_manager_example_parent_class)->dispose (self);
}


static void
mcp_account_manager_example_class_init (McpAccountManagerExampleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = mcp_account_manager_example_dispose;

  g_type_class_add_private (gobject_class,
      sizeof (McpAccountManagerExamplePrivate));
}


static void
mcp_account_manager_example_init (McpAccountManagerExample *self)
{
}


static GList *
mcp_account_manager_example_list (const McpAccountStorage *self,
    const McpAccountManager *am)
{
  McpAccountManagerExamplePrivate *priv = GET_PRIVATE (self);
  GList *accounts = NULL;

  DEBUG (G_STRFUNC);

  accounts = g_list_prepend (accounts, g_strdup (ACCOUNT_NAME));

  return accounts;
}


typedef struct
{
  char *key;
  char *value;
} Parameter;

static const Parameter parameters[] = {
      { "manager", "gabble" },
      { "protocol", "jabber" },
      { "Icon", "im-xmpp" },
      { "ConnectAutomatically", "true" },
      { "Enabled", "false" },
      { "DisplayName", "My Example Account" },

      { "param-account", "escher@tuxedo.cat" },
};


static gboolean
mcp_account_manager_example_get (const McpAccountStorage *self,
    const McpAccountManager *am,
    const gchar *acct,
    const gchar *key)
{
  DEBUG ("%s: %s, %s", G_STRFUNC, acct, key);

  if (!tp_strdiff (acct, ACCOUNT_NAME))
    {
      if (key == NULL)
        {
          guint i;

          /* load all keys */
          for (i = 0; i < G_N_ELEMENTS (parameters); i++)
            {
              const Parameter *param = &parameters[i];

              DEBUG ("Loading %s = %s", param->key, param->value);
              mcp_account_manager_set_value (am, acct,
                  param->key, param->value);
            }

          return TRUE;
        }
      else
        {
          guint i;

          /* get a specific key */
          for (i = 0; i < G_N_ELEMENTS (parameters); i++)
            {
              const Parameter *param = &parameters[i];

              if (!tp_strdiff (param->key, key))
                {
                  DEBUG ("Loading %s = %s", param->key, param->value);
                  mcp_account_manager_set_value (am, acct,
                      param->key, param->value);

                  return TRUE;
                }
            }

          return FALSE;
        }
    }

  return FALSE;
}


static gboolean
mcp_account_manager_example_set (const McpAccountStorage *self,
    const McpAccountManager *am,
    const gchar *acct,
    const gchar *key,
    const gchar *val)
{
  McpAccountManagerExamplePrivate *priv = GET_PRIVATE (self);
  GError *error = NULL;

  DEBUG ("%s: (%s, %s, %s)", G_STRFUNC, acct, key, val);

  if (!tp_strdiff (acct, ACCOUNT_NAME))
    /* pretend we saved everything */
    return TRUE;
  else
    return FALSE;
}


static gboolean
mcp_account_manager_example_delete (const McpAccountStorage *self,
    const McpAccountManager *am,
    const gchar *acct,
    const gchar *key)
{
  DEBUG ("%s: (%s, %s)", G_STRFUNC, acct, key);

  if (!tp_strdiff (acct, ACCOUNT_NAME))
    return TRUE;
  else
    return FALSE;
}


static gboolean
mcp_account_manager_example_commit (const McpAccountStorage *self,
    const McpAccountManager *am)
{
  DEBUG ("%s", G_STRFUNC);

  return TRUE;
}


static void
mcp_account_manager_example_ready (const McpAccountStorage *self,
    const McpAccountManager *am)
{
  McpAccountManagerExamplePrivate *priv = GET_PRIVATE (self);

  DEBUG (G_STRFUNC);

  priv->ready = TRUE;
}


static guint
mcp_account_manager_example_get_restrictions (const McpAccountStorage *self,
    const gchar *account)
{
  return TP_STORAGE_RESTRICTION_FLAG_CANNOT_SET_PARAMETERS |
         TP_STORAGE_RESTRICTION_FLAG_CANNOT_SET_SERVICE;
}


static void
account_storage_iface_init (McpAccountStorageIface *iface)
{
  mcp_account_storage_iface_set_name (iface, PLUGIN_NAME);
  mcp_account_storage_iface_set_desc (iface, PLUGIN_DESCRIPTION);
  mcp_account_storage_iface_set_priority (iface, PLUGIN_PRIORITY);
  mcp_account_storage_iface_set_provider (iface, PLUGIN_PROVIDER);

#define IMPLEMENT(x) mcp_account_storage_iface_implement_##x(iface, \
    mcp_account_manager_example_##x)
  IMPLEMENT (get);
  IMPLEMENT (list);
  IMPLEMENT (set);
  IMPLEMENT (delete);
  IMPLEMENT (commit);
  IMPLEMENT (ready);
  IMPLEMENT (get_restrictions);
#undef IMPLEMENT
}
