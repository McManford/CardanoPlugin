/**
 * \file update_drep_cert.c
 *
 * \author angel.castillo
 * \date   Jul 31, 2024
 *
 * Copyright 2024 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* INCLUDES ******************************************************************/

#include <cardano/certs/cert_type.h>
#include <cardano/certs/update_drep_cert.h>
#include <cardano/common/anchor.h>
#include <cardano/common/credential.h>
#include <cardano/error.h>
#include <cardano/object.h>

#include "../allocators.h"
#include "../cbor/cbor_validation.h"

#include <assert.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

static const int64_t EMBEDDED_GROUP_SIZE = 3;

/* STRUCTURES ****************************************************************/

/**
 * \brief Updates the DRep anchored metadata.
 */
typedef struct cardano_update_drep_cert_t
{
    cardano_object_t      base;
    cardano_credential_t* credential;
    cardano_anchor_t*     anchor;
} cardano_update_drep_cert_t;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Deallocates a update_drep_cert object.
 *
 * This function is responsible for properly deallocating a update_drep_cert object (`cardano_update_drep_cert_t`)
 * and its associated resources.
 *
 * \param object A void pointer to the update_drep_cert object to be deallocated. The function casts this
 *               pointer to the appropriate type (`cardano_update_drep_cert_t*`).
 *
 * \note It is assumed that this function is called only when the reference count of the update_drep_cert
 *       object reaches zero, as part of the reference counting mechanism implemented for managing the
 *       lifecycle of these objects.
 */
static void
cardano_update_drep_cert_deallocate(void* object)
{
  assert(object != NULL);

  cardano_update_drep_cert_t* data = (cardano_update_drep_cert_t*)object;

  cardano_credential_unref(&data->credential);
  cardano_anchor_unref(&data->anchor);

  _cardano_free(data);
}

/* DEFINITIONS ****************************************************************/

cardano_error_t
cardano_update_drep_cert_new(
  cardano_credential_t*        drep_credential,
  cardano_anchor_t*            anchor,
  cardano_update_drep_cert_t** update_drep_cert)
{
  if (drep_credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (update_drep_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_update_drep_cert_t* data = _cardano_malloc(sizeof(cardano_update_drep_cert_t));

  if (data == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  data->base.ref_count     = 1;
  data->base.last_error[0] = '\0';
  data->base.deallocator   = cardano_update_drep_cert_deallocate;

  cardano_credential_ref(drep_credential);
  data->credential = drep_credential;

  if (anchor != NULL)
  {
    cardano_anchor_ref(anchor);
  }

  data->anchor = anchor;

  *update_drep_cert = data;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_update_drep_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_update_drep_cert_t** update_drep_cert)
{
  if (reader == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (update_drep_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  static const char* validator_name = "update_drep_cert";

  cardano_error_t expect_array_result = cardano_cbor_validate_array_of_n_elements(validator_name, reader, (uint32_t)EMBEDDED_GROUP_SIZE);

  if (expect_array_result != CARDANO_SUCCESS)
  {
    return expect_array_result;
  }

  uint64_t              type             = 0U;
  const cardano_error_t read_uint_result = cardano_cbor_validate_enum_value(
    validator_name,
    "type",
    reader,
    CARDANO_CERT_TYPE_UPDATE_DREP,
    (enum_to_string_callback_t)((void*)&cardano_cert_type_to_string),
    &type);

  if (read_uint_result != CARDANO_SUCCESS)
  {
    return read_uint_result;
  }

  cardano_credential_t* credential = NULL;

  cardano_error_t read_credential_result = cardano_credential_from_cbor(reader, &credential);

  if (read_credential_result != CARDANO_SUCCESS)
  {
    return read_credential_result;
  }

  cardano_anchor_t* anchor = NULL;

  cardano_cbor_reader_state_t state;

  cardano_error_t read_state = cardano_cbor_reader_peek_state(reader, &state);

  if (read_state != CARDANO_SUCCESS)
  {
    cardano_credential_unref(&credential);

    return read_state;
  }

  if (state == CARDANO_CBOR_READER_STATE_NULL)
  {
    cardano_error_t read_null = cardano_cbor_reader_read_null(reader);
    CARDANO_UNUSED(read_null);
  }
  else
  {
    cardano_error_t read_anchor_result = cardano_anchor_from_cbor(reader, &anchor);

    if (read_anchor_result != CARDANO_SUCCESS)
    {
      cardano_credential_unref(&credential);

      return read_anchor_result;
    }
  }

  cardano_error_t new_result = cardano_update_drep_cert_new(credential, anchor, update_drep_cert);

  cardano_credential_unref(&credential);
  cardano_anchor_unref(&anchor);

  if (new_result != CARDANO_SUCCESS)
  {
    return new_result;
  }

  return cardano_cbor_validate_end_array(validator_name, reader);
}

cardano_error_t
cardano_update_drep_cert_to_cbor(
  const cardano_update_drep_cert_t* update_drep_cert,
  cardano_cbor_writer_t*            writer)
{
  if (update_drep_cert == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (writer == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_error_t write_array_result = cardano_cbor_writer_write_start_array(writer, EMBEDDED_GROUP_SIZE);

  if (write_array_result != CARDANO_SUCCESS)
  {
    return write_array_result;
  }

  cardano_error_t write_type_result = cardano_cbor_writer_write_uint(writer, CARDANO_CERT_TYPE_UPDATE_DREP);

  if (write_type_result != CARDANO_SUCCESS)
  {
    return write_type_result;
  }

  cardano_error_t write_credential_result = cardano_credential_to_cbor(update_drep_cert->credential, writer);

  if (write_credential_result != CARDANO_SUCCESS)
  {
    return write_credential_result;
  }

  if (update_drep_cert->anchor != NULL)
  {
    cardano_error_t write_anchor_result = cardano_anchor_to_cbor(update_drep_cert->anchor, writer);

    if (write_anchor_result != CARDANO_SUCCESS)
    {
      return write_anchor_result;
    }
  }
  else
  {
    cardano_error_t write_null_result = cardano_cbor_writer_write_null(writer);

    if (write_null_result != CARDANO_SUCCESS)
    {
      return write_null_result;
    }
  }

  return CARDANO_SUCCESS;
}

cardano_credential_t*
cardano_update_drep_cert_get_credential(cardano_update_drep_cert_t* certificate)
{
  if (certificate == NULL)
  {
    return NULL;
  }

  cardano_credential_ref(certificate->credential);

  return certificate->credential;
}

cardano_error_t
cardano_update_drep_cert_set_credential(
  cardano_update_drep_cert_t* certificate,
  cardano_credential_t*       credential)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (credential == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_credential_ref(credential);
  cardano_credential_unref(&certificate->credential);
  certificate->credential = credential;

  return CARDANO_SUCCESS;
}

cardano_anchor_t*
cardano_update_drep_cert_get_anchor(cardano_update_drep_cert_t* certificate)
{
  if (certificate == NULL)
  {
    return NULL;
  }

  cardano_anchor_ref(certificate->anchor);

  return certificate->anchor;
}

cardano_error_t
cardano_update_drep_cert_set_anchor(
  cardano_update_drep_cert_t* certificate,
  cardano_anchor_t*           anchor)
{
  if (certificate == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (anchor == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_anchor_ref(anchor);
  cardano_anchor_unref(&certificate->anchor);
  certificate->anchor = anchor;

  return CARDANO_SUCCESS;
}

void
cardano_update_drep_cert_unref(cardano_update_drep_cert_t** update_drep_cert)
{
  if ((update_drep_cert == NULL) || (*update_drep_cert == NULL))
  {
    return;
  }

  cardano_object_t* object = &(*update_drep_cert)->base;
  cardano_object_unref(&object);

  if (object == NULL)
  {
    *update_drep_cert = NULL;
    return;
  }
}

void
cardano_update_drep_cert_ref(cardano_update_drep_cert_t* update_drep_cert)
{
  if (update_drep_cert == NULL)
  {
    return;
  }

  cardano_object_ref(&update_drep_cert->base);
}

size_t
cardano_update_drep_cert_refcount(const cardano_update_drep_cert_t* update_drep_cert)
{
  if (update_drep_cert == NULL)
  {
    return 0;
  }

  return cardano_object_refcount(&update_drep_cert->base);
}

void
cardano_update_drep_cert_set_last_error(cardano_update_drep_cert_t* update_drep_cert, const char* message)
{
  cardano_object_set_last_error(&update_drep_cert->base, message);
}

const char*
cardano_update_drep_cert_get_last_error(const cardano_update_drep_cert_t* update_drep_cert)
{
  return cardano_object_get_last_error(&update_drep_cert->base);
}
