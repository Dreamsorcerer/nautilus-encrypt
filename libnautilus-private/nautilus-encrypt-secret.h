/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

#include <libsecret/secret.h>


const SecretSchema *get_encryption_schema(void);

#define ENCRYPTION_SCHEMA  get_encryption_schema ()
