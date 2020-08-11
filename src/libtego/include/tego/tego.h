#ifndef TEGO_H
#define TEGO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define TEGO_TRUE  1
#define TEGO_FALSE 0

#define TEGO_ED25519_SIGNATURE_LENGTH 64

typedef struct tego_error* tego_error_t;

/*
 * Get error message form tego_error
 *
 * @param error : the error object to get the message from
 * @return : null terminated string with error message whose
 *  lifetime is tied to the source tego_error_t
 */
const char* tego_error_get_message(const tego_error_t error);

// library init/uninit
void tego_initialize(tego_error_t* error);
void tego_uninitialize(tego_error_t* error);

/*
 * v3 onion/ed25519 functionality
 */

typedef struct tego_ed25519_private_key* tego_ed25519_private_key_t;
typedef struct tego_ed25519_public_key* tego_ed25519_public_key_t;
typedef struct tego_ed25519_signature* tego_ed25519_signature_t;


/*
 * Conversion method for converting the KeyBlob returned by
 * ADD_ONION command into an ed25519_private_key_t
 *
 * @param out_privateKey : returned ed25519 private key
 * @param keyBlob : a null terminated ED25519 KeyBlob in the form
 *  "ED25519-V3:abcd1234..."
 * @param error : filled with a tego_error_t on error
 */
void tego_ed25519_private_key_from_ed25519_keyblob(
    tego_ed25519_private_key_t* out_privateKey,
    const char* keyBlob,
    tego_error_t* error);

/*
 * Calculate ed25519 public key from ed25519 private key
 *
 * @param out_publicKey : returned ed25519 public key
 * @param privateKey : input ed25519 private key
 * @param error : filled with a tego_error_t on error
 */
void tego_ed25519_public_key_from_ed25519_private_key(
    tego_ed25519_public_key_t* out_publicKey,
    const tego_ed25519_private_key_t privateKey,
    tego_error_t* error);

/*
 * Extract public key from v3 onion domain per
 * https://gitweb.torproject.org/torspec.git/tree/rend-spec-v3.txt
 *
 * @param out_publicKey : returned ed25519 public key
 * @param v3onionDomain : null terminated v3 onion domain including ".onion" suffix
 * @param error : filled with a tego_error_t on error
 */
void tego_ed25519_public_key_from_v3_onion_domain(
    tego_ed25519_public_key_t* out_publicKey,
    const char* v3OnionDomain,
    tego_error_t* error);

/*
 * Read in signature from length 64 byte buffer
 *
 * @param out_signature : returned ed25519 signature
 * @param data : 64 byte memory buffer holding signature
 * @param error : filled with a tego_error_t on error
 */
void tego_ed25519_signature_from_data(
    tego_ed25519_signature_t* out_signature,
    const uint8_t data[TEGO_ED25519_SIGNATURE_LENGTH],
    tego_error_t* error);

/*
 * Sign a message with ed25519 key-pair
 *
 * @param message: binary blob to sign
 * @param messageLength : length of blob in bytes
 * @param privateKey: the ed25519 private key
 * @param publicKey : the ed25519 public key
 * @param out_signature : the output signature
 * @param error : filled with a tego_error_t on error
 */
void tego_message_ed25519_sign(
    const uint8_t* message,
    size_t messageLength,
    const tego_ed25519_public_key_t privateKey,
    const tego_ed25519_public_key_t publicKey,
    tego_ed25519_signature_t* out_signature,
    tego_error_t* error);

/*
 * Get the signature and place it in length 64 byte buffer
 *
 * @param signature : a calculated message signature
 * @param out_data : output buffer to write signature to
 */
void tego_ed25519_signature_data(
    const tego_ed25519_signature_t signature,
    uint8_t out_data[TEGO_ED25519_SIGNATURE_LENGTH],
    tego_error_t* error);

/*
 * Verify a message's signature given a public key
 *
 * @param signature : the signature to verify
 * @param message : the message that was signed
 * @param messageLength : length of the message
 * @param publicKey : the public key to very the the signature against
 * @param error : filled with a tego_error_t on error
 * @return : TEGO_TRUE if signature is verified, TEGO_FALSE if it is not
 *  verified or if an error occurs
 */
int tego_ed25519_signature_verify(
    const tego_ed25519_signature_t signature,
    const uint8_t* message,
    size_t messageLength,
    const tego_ed25519_public_key_t publicKey,
    tego_error_t* error);

/*
 Destructors for various tego types
 */

void tego_ed25519_private_key_delete(tego_ed25519_private_key_t);
void tego_ed25519_public_key_delete(tego_ed25519_public_key_t);
void tego_ed25519_signature_delete(tego_ed25519_signature_t);
void tego_error_delete(tego_error_t);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // TEGO_H