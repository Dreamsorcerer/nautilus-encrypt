/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

#include "nautilus-encrypt-secret.h"

const SecretSchema *
get_encryption_schema (void)
{
	static const SecretSchema the_schema = {
		"org.nautilus.EncryptionPassword", SECRET_SCHEMA_NONE,
		{
			{  "encfs-key", SECRET_SCHEMA_ATTRIBUTE_STRING },
			{  "location", SECRET_SCHEMA_ATTRIBUTE_STRING },
			{  "NULL", 0 },
		}
	};
	return &the_schema;
}
