/*
 * DBus encapsulation for VLAN interfaces
 *
 * Copyright (C) 2011 Olaf Kirch <okir@suse.de>
 */

#include <sys/poll.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <errno.h>

#include <wicked/netinfo.h>
#include <wicked/logging.h>
#include "model.h"

/*
 * Create a new VLAN interface
 */
ni_dbus_object_t *
ni_objectmodel_new_vlan(ni_dbus_server_t *server, const ni_dbus_object_t *config)
{
	ni_interface_t *cfg_ifp = ni_dbus_object_get_handle(config);
	ni_interface_t *new_ifp;
	const ni_vlan_t *vlan = cfg_ifp->vlan;
	ni_handle_t *nih = ni_global_state_handle();

	if (!vlan
	 || !vlan->tag
	 || !vlan->interface_name) {
		/* FIXME: report error */
		return NULL;
	}

	cfg_ifp->type = NI_IFTYPE_VLAN;
	if (cfg_ifp->name == NULL) {
		static char namebuf[64];
		unsigned int num;

		for (num = 0; num < 65536; ++num) {
			snprintf(namebuf, sizeof(namebuf), "vlan%u", num);
			if (!ni_interface_by_name(nih, namebuf)) {
				ni_string_dup(&cfg_ifp->name, namebuf);
				break;
			}
		}

		if (cfg_ifp->name == NULL) {
			/* FIXME: report error */
			return NULL;
		}
	}

	if (ni_interface_create_vlan(nih, cfg_ifp->name, vlan, &new_ifp) < 0) {
		/* FIXME: report error */
		return NULL;
	}

	if (new_ifp->type != NI_IFTYPE_VLAN) {
		/* FIXME: report error */
		return NULL;
	}

	return ni_objectmodel_register_interface(server, new_ifp);
}

static ni_vlan_t *
__wicked_dbus_vlan_handle(const ni_dbus_object_t *object, DBusError *error)
{
	ni_interface_t *ifp = ni_dbus_object_get_handle(object);

	return ni_interface_get_vlan(ifp);
}

static dbus_bool_t
__wicked_dbus_vlan_get_tag(const ni_dbus_object_t *object,
				const ni_dbus_property_t *property,
				ni_dbus_variant_t *result,
				DBusError *error)
{
	ni_vlan_t *vlan;

	if (!(vlan = __wicked_dbus_vlan_handle(object, error)))
		return FALSE;

	ni_dbus_variant_set_uint16(result, vlan->tag);
	return TRUE;
}

static dbus_bool_t
__wicked_dbus_vlan_set_tag(ni_dbus_object_t *object,
				const ni_dbus_property_t *property,
				const ni_dbus_variant_t *result,
				DBusError *error)
{
	ni_vlan_t *vlan;
	uint16_t value;

	if (!(vlan = __wicked_dbus_vlan_handle(object, error)))
		return FALSE;

	if (!ni_dbus_variant_get_uint16(result, &value)) {
		dbus_set_error(error, DBUS_ERROR_INVALID_ARGS,
				"tag property must be of type uint16");
		return FALSE;
	}
	if (value == 0) {
		dbus_set_error(error, DBUS_ERROR_INVALID_ARGS,
				"tag property must not be 0");
		return FALSE;
	}
	vlan->tag = value;
	return TRUE;
}

static dbus_bool_t
__wicked_dbus_vlan_get_interface_name(const ni_dbus_object_t *object,
				const ni_dbus_property_t *property,
				ni_dbus_variant_t *result,
				DBusError *error)
{
	ni_vlan_t *vlan;

	if (!(vlan = __wicked_dbus_vlan_handle(object, error)))
		return FALSE;

	ni_dbus_variant_set_string(result, vlan->interface_name);
	return TRUE;
}

static dbus_bool_t
__wicked_dbus_vlan_set_interface_name(ni_dbus_object_t *object,
				const ni_dbus_property_t *property,
				const ni_dbus_variant_t *result,
				DBusError *error)
{
	ni_vlan_t *vlan;
	const char *interface_name;

	if (!(vlan = __wicked_dbus_vlan_handle(object, error)))
		return FALSE;

	if (!ni_dbus_variant_get_string(result, &interface_name))
		return FALSE;
	ni_string_dup(&vlan->interface_name, interface_name);
	return TRUE;
}

#define WICKED_VLAN_PROPERTY(type, __name, rw) \
	NI_DBUS_PROPERTY(type, __name, offsetof(ni_vlan_t, __name), __wicked_dbus_vlan, rw)
#define WICKED_VLAN_PROPERTY_SIGNATURE(signature, __name, rw) \
	__NI_DBUS_PROPERTY(signature, __name, offsetof(ni_vlan_t, __name), __wicked_dbus_ethernet, rw)

static ni_dbus_property_t	wicked_dbus_vlan_properties[] = {
	WICKED_VLAN_PROPERTY(STRING, interface_name, RO),
	WICKED_VLAN_PROPERTY(UINT16, tag, RO),
	{ NULL }
};


static ni_dbus_method_t		wicked_dbus_vlan_methods[] = {
	{ NULL }
};

ni_dbus_service_t	wicked_dbus_vlan_service = {
	.object_interface = WICKED_DBUS_INTERFACE ".VLAN",
	.methods = wicked_dbus_vlan_methods,
	.properties = wicked_dbus_vlan_properties,
};

