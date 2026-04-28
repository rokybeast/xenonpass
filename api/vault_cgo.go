package api

/*
#cgo CFLAGS: -I${SRCDIR}/../src/vault/include
#cgo LDFLAGS: -L${SRCDIR}/../build -lxpass -lsodium -Wl,-rpath=${SRCDIR}/../build
#include <stdlib.h>
#include "xpass.h"
*/
import "C"
import (
	"errors"
	"unsafe"
)

func init() {
	if C.xpass_init() < 0 {
		panic("failed to initialize libsodium")
	}
}

func LoadVault(path string, password string) ([]byte, error) {
	cPath := C.CString(path)
	defer C.free(unsafe.Pointer(cPath))

	var cCiphertext *C.uint8_t
	var cCiphertextLen C.uint32_t
	var cSalt [C.XPASS_SALT_LEN]C.uint8_t
	var cNonce [C.XPASS_NONCE_LEN]C.uint8_t

	if C.xpass_vault_load(cPath, &cCiphertext, &cCiphertextLen, &cSalt[0], &cNonce[0]) != 0 {
		return nil, errors.New("failed to load vault")
	}
	defer C.free(unsafe.Pointer(cCiphertext))

	var cKey [C.XPASS_KEY_LEN]C.uint8_t
	cPassword := C.CString(password)
	defer C.free(unsafe.Pointer(cPassword))

	if C.xpass_derive_key(&cKey[0], cPassword, C.size_t(len(password)), &cSalt[0]) != 0 {
		return nil, errors.New("failed to derive key")
	}
	defer C.xpass_secure_zero(unsafe.Pointer(&cKey[0]), C.XPASS_KEY_LEN)

	plaintext := make([]byte, int(cCiphertextLen))
	var cPlaintextLen C.ulonglong

	if C.xpass_decrypt((*C.uint8_t)(&plaintext[0]), &cPlaintextLen, cCiphertext, C.size_t(cCiphertextLen), &cNonce[0], &cKey[0]) != 0 {
		return nil, errors.New("failed to decrypt vault")
	}

	return plaintext[:cPlaintextLen], nil
}

func SaveVault(path string, password string, plaintext []byte) error {
	cPath := C.CString(path)
	defer C.free(unsafe.Pointer(cPath))

	var cSalt [C.XPASS_SALT_LEN]C.uint8_t
	var cNonce [C.XPASS_NONCE_LEN]C.uint8_t
	var cKey [C.XPASS_KEY_LEN]C.uint8_t

	C.xpass_generate_salt(&cSalt[0])

	cPassword := C.CString(password)
	defer C.free(unsafe.Pointer(cPassword))

	if C.xpass_derive_key(&cKey[0], cPassword, C.size_t(len(password)), &cSalt[0]) != 0 {
		return errors.New("failed to derive key")
	}
	defer C.xpass_secure_zero(unsafe.Pointer(&cKey[0]), C.XPASS_KEY_LEN)

	if len(plaintext) == 0 {
		plaintext = []byte("{}")
	}

	ctLen := len(plaintext) + C.XPASS_TAG_LEN + 1
	ciphertext := make([]byte, ctLen)
	var cCiphertextLen C.ulonglong

	if C.xpass_encrypt((*C.uint8_t)(&ciphertext[0]), &cCiphertextLen, (*C.uint8_t)(&plaintext[0]), C.size_t(len(plaintext)), &cNonce[0], &cKey[0]) != 0 {
		return errors.New("failed to encrypt vault")
	}

	if C.xpass_vault_save(cPath, (*C.uint8_t)(&ciphertext[0]), C.uint32_t(cCiphertextLen), &cSalt[0], &cNonce[0]) != 0 {
		return errors.New("failed to save vault")
	}

	return nil
}
