// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <string_view>

#include "littlelibc.h"
#include "spi.h"

namespace MCPro {

bool IsConnected( ulong which );
void SendFilename( std::string_view inName, ulong which);

}

