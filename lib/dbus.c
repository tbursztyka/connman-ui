/*
 *  Connection Manager UI
 *
 *  Copyright (C) 2012  Intel Corporation. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License version 2.1,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <cui-dbus.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char *map_basic_to_signature(int dbus_type)
{
	switch (dbus_type) {
	case DBUS_TYPE_BOOLEAN:
		return DBUS_TYPE_BOOLEAN_AS_STRING;
	case DBUS_TYPE_INT16:
		return DBUS_TYPE_INT16_AS_STRING;
	case DBUS_TYPE_UINT16:
		return DBUS_TYPE_UINT16_AS_STRING;
	case DBUS_TYPE_INT32:
		return DBUS_TYPE_INT32_AS_STRING;
	case DBUS_TYPE_UINT32:
		return DBUS_TYPE_UINT32_AS_STRING;
	case DBUS_TYPE_DOUBLE:
		return DBUS_TYPE_DOUBLE_AS_STRING;
	case DBUS_TYPE_STRING:
		return DBUS_TYPE_STRING_AS_STRING;
	case DBUS_TYPE_OBJECT_PATH:
		return DBUS_TYPE_OBJECT_PATH_AS_STRING;
	case DBUS_TYPE_SIGNATURE:
		return DBUS_TYPE_SIGNATURE_AS_STRING;
	case DBUS_TYPE_DICT_ENTRY:
		return DBUS_TYPE_DICT_ENTRY_AS_STRING;
	case DBUS_TYPE_BYTE:
		return DBUS_TYPE_BYTE_AS_STRING;
	default:
		break;
	}

	return DBUS_TYPE_VARIANT_AS_STRING;
}

static const char *map_array_to_signature(int dbus_type)
{
	switch (dbus_type) {
	case DBUS_TYPE_BOOLEAN:
		return DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_BOOLEAN_AS_STRING;
	case DBUS_TYPE_INT16:
		return DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_INT16_AS_STRING;
	case DBUS_TYPE_UINT16:
		return DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_UINT16_AS_STRING;
	case DBUS_TYPE_INT32:
		return DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_INT32_AS_STRING;
	case DBUS_TYPE_UINT32:
		return DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_UINT32_AS_STRING;
	case DBUS_TYPE_DOUBLE:
		return DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_DOUBLE_AS_STRING;
	case DBUS_TYPE_STRING:
		return DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING;
	case DBUS_TYPE_OBJECT_PATH:
		return DBUS_TYPE_ARRAY_AS_STRING
			DBUS_TYPE_OBJECT_PATH_AS_STRING;
	case DBUS_TYPE_SIGNATURE:
		return DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_SIGNATURE_AS_STRING;
	case DBUS_TYPE_DICT_ENTRY:
		return DBUS_TYPE_ARRAY_AS_STRING
			DBUS_TYPE_DICT_ENTRY_AS_STRING;
	case DBUS_TYPE_BYTE:
		return DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_ARRAY_AS_STRING;
	default:
		break;
	}

	return DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_VARIANT_AS_STRING;
}

void cui_dbus_append_dict(DBusMessageIter *iter,
				const char *key_name,
				cui_dbus_property_f property_function,
				void *user_data)
{
	DBusMessageIter dict, variant, *vdict;

	if (key_name != NULL) {
		dbus_message_iter_append_basic(iter,
					DBUS_TYPE_STRING, &key_name);

		dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
			DBUS_TYPE_ARRAY_AS_STRING
			DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
			DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING
			DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &variant);

		vdict = &variant;
	} else
		vdict = iter;

	dbus_message_iter_open_container(vdict, DBUS_TYPE_ARRAY,
			DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
			DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING
			DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dict);

	if (property_function != NULL)
		property_function(&dict, user_data);

	dbus_message_iter_close_container(vdict, &dict);

	if (key_name != NULL)
		dbus_message_iter_close_container(iter, &variant);
}

void cui_dbus_append_basic(DBusMessageIter *iter,
				const char *key_name,
				int dbus_type,
				void *value)
{
	if (key_name != NULL) {
		DBusMessageIter basic;

		dbus_message_iter_append_basic(iter,
					DBUS_TYPE_STRING, &key_name);

		dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
				map_basic_to_signature(dbus_type), &basic);

		dbus_message_iter_append_basic(&basic, dbus_type, value);

		dbus_message_iter_close_container(iter, &basic);
	} else
		dbus_message_iter_append_basic(iter, dbus_type, value);
}

void cui_dbus_append_array(DBusMessageIter *iter,
				const char *key_name,
				int dbus_type,
				cui_dbus_property_f property_function,
				void *user_data)
{
	DBusMessageIter variant, array, *varray;

	if (key_name != NULL) {
		dbus_message_iter_append_basic(iter,
					DBUS_TYPE_STRING, &key_name);

		dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
				map_array_to_signature(dbus_type), &variant);

		varray = &variant;
	} else
		varray = iter;

	dbus_message_iter_open_container(varray, DBUS_TYPE_ARRAY,
			map_basic_to_signature(dbus_type), &array);

	if (property_function != NULL)
		property_function(&array, user_data);

	dbus_message_iter_close_container(varray, &array);

	if (key_name != NULL)
		dbus_message_iter_close_container(iter, &variant);
}

void cui_dbus_append_fixed_array(DBusMessageIter *iter,
					const char *key_name,
					int dbus_type,
					int length,
					void *value)
{
	DBusMessageIter variant, array, *varray;

	if (key_name != NULL) {
		dbus_message_iter_append_basic(iter,
					DBUS_TYPE_STRING, &key_name);

		dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
			map_array_to_signature(DBUS_TYPE_BYTE), &variant);

		varray = &variant;
	} else
		varray = iter;

	dbus_message_iter_open_container(varray, DBUS_TYPE_ARRAY,
			map_basic_to_signature(DBUS_TYPE_BYTE), &array);

	dbus_message_iter_append_fixed_array(&array, dbus_type, value, length);

	dbus_message_iter_close_container(varray, &array);

	if (key_name != NULL)
		dbus_message_iter_close_container(iter, &variant);
}

void cui_dbus_append_dict_entry(DBusMessageIter *dict,
				const char *key_name,
				enum cui_dbus_entry entry_type,
				int dbus_type,
				int length,
				cui_dbus_property_f property_function,
				void *user_data)
{
	DBusMessageIter entry;

	dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY,
								NULL, &entry);

	switch (entry_type) {
	case CUI_DBUS_ENTRY_BASIC:
		cui_dbus_append_basic(&entry, key_name,
						dbus_type, user_data);
		break;
	case CUI_DBUS_ENTRY_ARRAY:
		cui_dbus_append_array(&entry, key_name, dbus_type,
					property_function, user_data);
		break;
	case CUI_DBUS_ENTRY_FIXED_ARRAY:
		cui_dbus_append_fixed_array(&entry, key_name,
					dbus_type, length, user_data);
		break;
	case CUI_DBUS_ENTRY_DICT:
		cui_dbus_append_dict(&entry, key_name,
					property_function, user_data);
		break;
	default:
		break;
	}

	dbus_message_iter_close_container(dict, &entry);
}

int cui_dbus_get_basic(DBusMessageIter *iter,
					int dbus_type,
					void *destination)
{
	if (destination == NULL)
		return -EINVAL;

	if (dbus_message_iter_get_arg_type(iter) != dbus_type)
		return -EINVAL;

	dbus_message_iter_get_basic(iter, destination);

	return 0;
}

int cui_dbus_get_basic_variant(DBusMessageIter *iter,
						int dbus_type,
						void *destination)
{
	DBusMessageIter variant;

	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_VARIANT)
		return -EINVAL;

	dbus_message_iter_recurse(iter, &variant);

	if (dbus_message_iter_get_arg_type(&variant) != dbus_type)
		return -EINVAL;

	dbus_message_iter_get_basic(&variant, destination);

	return 0;
}

int cui_dbus_get_array(DBusMessageIter *iter,
					int dbus_type,
					int *length,
					void *destination)
{
	DBusMessageIter variant, array;
	void **value_array = NULL;
	int iteration;
	int arg_type;
	size_t size;

	if (length == NULL || destination == NULL)
		return -EINVAL;

	switch (dbus_type) {
	case DBUS_TYPE_BOOLEAN:
	case DBUS_TYPE_INT16:
	case DBUS_TYPE_INT32:
	case DBUS_TYPE_UINT16:
	case DBUS_TYPE_UINT32:
		size = sizeof(int);
		break;
	case DBUS_TYPE_DOUBLE:
		size = sizeof(double);
		break;
	case DBUS_TYPE_STRING:
	case DBUS_TYPE_OBJECT_PATH:
	case DBUS_TYPE_SIGNATURE:
		size = sizeof(char *);
		break;
	default:
		return -EINVAL;
	}

	*length = 0;
	iteration = 0;
	*((void **) destination) = NULL;

	arg_type = dbus_message_iter_get_arg_type(iter);
	if (arg_type == DBUS_TYPE_VARIANT) {
		dbus_message_iter_recurse(iter, &variant);
		dbus_message_iter_recurse(&variant, &array);
	} else
		dbus_message_iter_recurse(iter, &array);

	arg_type = dbus_message_iter_get_arg_type(&array);
	while (arg_type != DBUS_TYPE_INVALID) {
		void **new_array;

		iteration++;

		new_array = realloc(value_array, size * (iteration + 1));
		if (new_array == NULL)
			goto error;

		value_array = new_array;

		if (arg_type == DBUS_TYPE_VARIANT) {
			if (cui_dbus_get_basic_variant(&array, dbus_type,
						&value_array[iteration-1]) != 0)
				goto error;
		} else
			dbus_message_iter_get_basic(&array,
						&value_array[iteration-1]);

		dbus_message_iter_next(&array);

		arg_type = dbus_message_iter_get_arg_type(&array);
	}

	if (iteration > 0) {
		value_array[iteration] = NULL;
		*length = iteration - 1;
	}

	*((void **) destination) = value_array;

	return 0;

error:
	if (value_array != NULL)
		free(value_array);

	return -ENOMEM;
}

int cui_dbus_get_fixed_array(DBusMessageIter *iter,
					int *length,
					void *destination)
{
	DBusMessageIter farray;

	if (length == NULL || destination == NULL)
		return -EINVAL;

	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_ARRAY)
		return -EINVAL;

	if (dbus_message_iter_get_element_type(iter) != DBUS_TYPE_BYTE)
		return -EINVAL;

	dbus_message_iter_recurse(iter, &farray);

	dbus_message_iter_get_fixed_array(&farray, destination, length);

	return 0;
}

static int cui_dbus_get(DBusMessageIter *iter,
					enum cui_dbus_entry entry_type,
					int dbus_type,
					int *length,
					void *destination)
{
	switch (entry_type) {
		case CUI_DBUS_ENTRY_BASIC:
			return cui_dbus_get_basic(iter,
						dbus_type, destination);
		case CUI_DBUS_ENTRY_ARRAY:
			return cui_dbus_get_array(iter,
					dbus_type, length, destination);
		case CUI_DBUS_ENTRY_FIXED_ARRAY:
			return cui_dbus_get_fixed_array(iter,
							length, destination);
		case CUI_DBUS_ENTRY_DICT:
			if (destination == NULL)
				return -EINVAL;

			dbus_message_iter_recurse(iter,
					(DBusMessageIter *)destination);
			return 0;
		default:
			break;
	}

	return -EINVAL;
}

int cui_dbus_foreach_dict_entry(DBusMessageIter *iter,
				cui_dbus_foreach_callback_f callback,
				void *user_data)
{
	DBusMessageIter array, dict_entry;
	int arg_type;

	arg_type = dbus_message_iter_get_arg_type(iter);
	if (arg_type != DBUS_TYPE_ARRAY)
		return -EINVAL;

	if (dbus_message_iter_get_element_type(iter) == DBUS_TYPE_BYTE)
		return -EINVAL;

	dbus_message_iter_recurse(iter, &array);

	arg_type = dbus_message_iter_get_arg_type(&array);
	while (arg_type != DBUS_TYPE_INVALID) {
		if (arg_type != DBUS_TYPE_DICT_ENTRY)
			return -EINVAL;

		dbus_message_iter_recurse(&array, &dict_entry);

		if (callback(&dict_entry, user_data) == TRUE)
			return 0;

		dbus_message_iter_next(&array);
		arg_type = dbus_message_iter_get_arg_type(&array);
	}

	return -EINVAL;
}

struct dict_entry_parameters {
	const char *key_name;
	enum cui_dbus_entry entry_type;
	int dbus_type;
	int *length;
	void *destination;
};

static bool get_dict_entry_cb(DBusMessageIter *iter, void *user_data)
{
	struct dict_entry_parameters *param = user_data;
	DBusMessageIter dict_value;
	char *name;

	dbus_message_iter_get_basic(iter, &name);

	if (strncmp(param->key_name, name, strlen(param->key_name)) == 0) {
		dbus_message_iter_next(iter);
		dbus_message_iter_recurse(iter, &dict_value);

		cui_dbus_get(&dict_value, param->entry_type,
			param->dbus_type, param->length, param->destination);

		return TRUE;
	}

	return FALSE;
}

int cui_dbus_get_dict_entry(DBusMessageIter *iter,
					const char *key_name,
					enum cui_dbus_entry entry_type,
					int dbus_type,
					int *length,
					void *destination)
{
	struct dict_entry_parameters param;

	if (key_name == NULL)
		return -EINVAL;

	param.key_name = key_name;
	param.entry_type = entry_type;
	param.dbus_type = dbus_type;
	param.length = length;
	param.destination = destination;

	return cui_dbus_foreach_dict_entry(iter, get_dict_entry_cb, &param);
}

int cui_dbus_get_struct_entry(DBusMessageIter *iter,
					unsigned int position,
					enum cui_dbus_entry entry_type,
					int dbus_type,
					int *length,
					void *destination)
{
	DBusMessageIter struct_entry;
	unsigned int current = 1;
	int arg_type;

	arg_type = dbus_message_iter_get_arg_type(iter);
	if (arg_type != DBUS_TYPE_STRUCT)
		return -EINVAL;

	dbus_message_iter_recurse(iter, &struct_entry);

	arg_type = dbus_message_iter_get_arg_type(&struct_entry);

	while (arg_type != DBUS_TYPE_INVALID) {
		if (position != current) {
			dbus_message_iter_next(&struct_entry);
			arg_type = dbus_message_iter_get_arg_type(&struct_entry);

			current++;

			continue;
		}

		return cui_dbus_get(&struct_entry, entry_type,
					dbus_type, length, destination);
	}

	return -EINVAL;
}
