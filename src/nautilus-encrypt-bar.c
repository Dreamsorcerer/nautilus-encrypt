/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006 Paolo Borelli <pborelli@katamail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Paolo Borelli <pborelli@katamail.com>
 *
 */

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "nautilus-encrypt-bar.h"

#include "nautilus-view.h"

#define NAUTILUS_ENCRYPT_BAR_GET_PRIVATE(o)\
	(G_TYPE_INSTANCE_GET_PRIVATE ((o), NAUTILUS_TYPE_ENCRYPT_BAR, NautilusEncryptBarPrivate))

enum {
	PROP_VIEW = 1,
	PROP_SLOT,
	NUM_PROPERTIES
};

enum {
	ENCRYPT_BAR_RESPONSE_UNLOCK = 1,
};

struct NautilusEncryptBarPrivate
{
	NautilusView *view;
	NautilusWindowSlot *slot;
	gulong selection_handler_id;
};

G_DEFINE_TYPE (NautilusEncryptBar, nautilus_encrypt_bar, GTK_TYPE_INFO_BAR);

static void
nautilus_encrypt_bar_set_property (GObject      *object,
				 guint         prop_id,
				 const GValue *value,
				 GParamSpec   *pspec)
{
	NautilusEncryptBar *bar;

	bar = NAUTILUS_ENCRYPT_BAR (object);

	switch (prop_id) {
	case PROP_VIEW:
		bar->priv->view = g_value_get_object (value);
		break;
	case PROP_SLOT:
		bar->priv->slot = g_value_get_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
nautilus_encrypt_bar_dispose (GObject *obj)
{
	NautilusEncryptBar *bar;

	bar = NAUTILUS_ENCRYPT_BAR (obj);

	if (bar->priv->selection_handler_id) {
		g_signal_handler_disconnect (bar->priv->view, bar->priv->selection_handler_id);
		bar->priv->selection_handler_id = 0;
	}

	G_OBJECT_CLASS (nautilus_encrypt_bar_parent_class)->dispose (obj);
}

static void
nautilus_encrypt_bar_class_init (NautilusEncryptBarClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = nautilus_encrypt_bar_set_property;
	object_class->dispose = nautilus_encrypt_bar_dispose;

	g_object_class_install_property (object_class,
					 PROP_VIEW,
					 g_param_spec_object ("view",
							      "view",
							      "the NautilusView",
							      NAUTILUS_TYPE_VIEW,
							      G_PARAM_WRITABLE |
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
					 PROP_SLOT,
					 g_param_spec_object ("slot",
							      "slot",
							      "the NautilusWindowSlot",
							      NAUTILUS_TYPE_WINDOW_SLOT,
							      G_PARAM_WRITABLE |
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (klass, sizeof (NautilusEncryptBarPrivate));
}

static void
encrypt_bar_response_cb (GtkInfoBar *infobar,
		       gint response_id,
		       gpointer user_data)
{
	NautilusEncryptBar *bar;
	NautilusFile *dir;
	gchar *this_dir;
	int exit_code;

	bar = NAUTILUS_ENCRYPT_BAR (infobar);

	if (response_id == ENCRYPT_BAR_RESPONSE_UNLOCK) {
		dir = nautilus_view_get_directory_as_file (bar->priv->view);
		this_dir = g_file_get_path (nautilus_file_get_location (dir));

		// Get encrypted name for selected directory.
		char *parent_name = g_file_get_path (nautilus_file_get_parent_location (dir));
		char *dir_name = nautilus_file_get_name (dir);
		char enc_dir[strlen (parent_name) + strlen (dir_name) + 7];
		sprintf (enc_dir, "%s/.%s-enc", parent_name, dir_name);

		gchar *args[] = {"encfs", enc_dir, this_dir,
				 "--standard", "--extpass=/usr/bin/ssh-askpass", NULL};
		if (g_spawn_sync (NULL, args, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
				  NULL, NULL, &exit_code, NULL) == FALSE) {
			// error
		}
		if (exit_code != 0) {
			// error
		}

		g_free (this_dir);
		g_object_unref (dir);

		nautilus_window_slot_queue_reload (bar->priv->slot);
	}
}

static void
nautilus_encrypt_bar_init (NautilusEncryptBar *bar)
{
	GtkWidget *content_area, *action_area, *w;
	GtkWidget *label;
	PangoAttrList *attrs;

	bar->priv = NAUTILUS_ENCRYPT_BAR_GET_PRIVATE (bar);
	content_area = gtk_info_bar_get_content_area (GTK_INFO_BAR (bar));
	action_area = gtk_info_bar_get_action_area (GTK_INFO_BAR (bar));

	gtk_orientable_set_orientation (GTK_ORIENTABLE (action_area),
					GTK_ORIENTATION_HORIZONTAL);

	attrs = pango_attr_list_new ();
	pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
	label = gtk_label_new (_("This folder is protected"));
	gtk_label_set_attributes (GTK_LABEL (label), attrs);
	pango_attr_list_unref (attrs);

	gtk_widget_show (label);
	gtk_container_add (GTK_CONTAINER (content_area), label);

	w = gtk_info_bar_add_button (GTK_INFO_BAR (bar),
				     _("Unlock"),
				     ENCRYPT_BAR_RESPONSE_UNLOCK);
	gtk_widget_set_tooltip_text (w,
				     _("Unlock the folder and view its contents"));

	g_signal_connect (bar, "response",
			  G_CALLBACK (encrypt_bar_response_cb), bar);
}

GtkWidget *
nautilus_encrypt_bar_new (NautilusWindowSlot *slot)
{
	NautilusView *view;

	view = nautilus_window_slot_get_current_view (slot);

	return g_object_new (NAUTILUS_TYPE_ENCRYPT_BAR,
			     "view", view,
			     "slot", slot,
			     "message-type", GTK_MESSAGE_QUESTION,
			     NULL);
}
