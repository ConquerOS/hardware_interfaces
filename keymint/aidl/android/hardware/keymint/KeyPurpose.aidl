/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.keymint;


/**
 * Possible purposes of a key (or pair).
 */
@VintfStability
@Backing(type = "int")
enum KeyPurpose {
    /* Usable with RSA, EC and AES keys. */
    ENCRYPT = 0,

    /* Usable with RSA, EC and AES keys. */
    DECRYPT = 1,

    /* Usable with RSA, EC and HMAC keys. */
    SIGN = 2,

    /* Usable with RSA, EC and HMAC keys. */
    VERIFY = 3,

    /* 4 is reserved */
    /* Usable with wrapping keys. */
    WRAP_KEY = 5,

    /* TODO(seleneh) add AGREE_KEY and ATTEST_KEY and their corresponding codes and tests later*/
}
