/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ARC4.h"
#include "Errors.h"

Acore::Crypto::ARC4::ARC4() : _ctx(EVP_CIPHER_CTX_new())
{
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    _cipher = EVP_CIPHER_fetch(nullptr, "RC4", nullptr);
#else
    EVP_CIPHER const* _cipher = EVP_rc4();
#endif

    EVP_CIPHER_CTX_init(_ctx);
    int result = EVP_EncryptInit_ex(_ctx, _cipher, nullptr, nullptr, nullptr);
    ASSERT(result == 1);
}

Acore::Crypto::ARC4::~ARC4()
{
    EVP_CIPHER_CTX_free(_ctx);

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    EVP_CIPHER_free(_cipher);
#endif
}

void Acore::Crypto::ARC4::Init(uint8 const* seed, std::size_t len)
{
    int result1 = EVP_CIPHER_CTX_set_key_length(_ctx, len);
    ASSERT(result1 == 1);
    int result2 = EVP_EncryptInit_ex(_ctx, nullptr, nullptr, seed, nullptr);
    ASSERT(result2 == 1);
}

void Acore::Crypto::ARC4::UpdateData(uint8* data, std::size_t len)
{
    int outlen = 0;
    int result1 = EVP_EncryptUpdate(_ctx, data, &outlen, data, len);
    ASSERT(result1 == 1);
    int result2 = EVP_EncryptFinal_ex(_ctx, data, &outlen);
    ASSERT(result2 == 1);
}
