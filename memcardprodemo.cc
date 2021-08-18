// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "memcardpro.h"

void main() {
    if( MCPro::IsConnected( 0 ) ) {
        MCPro::SendFilename( "Demo0", 0 );
    } else if( MCPro::IsConnected( 1 ) ) {
        MCPro::SendFilename( "Demo1", 1 );
    }
}
