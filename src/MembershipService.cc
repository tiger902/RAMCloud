/* Copyright (c) 2011-2012 Stanford University
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR(S) DISCLAIM ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL AUTHORS BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * \file
 * This file implements the MembershipService class. It is responsible for
 * maintaining the global ServerList object describing other servers in the
 * system.
 */

#include "Common.h"
#include "MembershipService.h"
#include "ProtoBuf.h"
#include "ServerId.h"
#include "ServerConfig.pb.h"
#include "ServerList.pb.h"
#include "ShortMacros.h"

namespace RAMCloud {

/**
 * Construct a new MembershipService object. There should really only be one
 * per server.
 */
MembershipService::MembershipService(ServerId ourServerId,
                                     ServerList* serverList,
                                     const ServerConfig* serverConfig)
    : serverList(serverList),
      serverConfig(serverConfig)
{
    // The coordinator will push the server list to us once we've
    // enlisted.
    serverId = ourServerId;
}

/**
 * Dispatch an RPC to the right handler based on its opcode.
 */
void
MembershipService::dispatch(WireFormat::Opcode opcode, Rpc* rpc)
{
    switch (opcode) {
    case WireFormat::GetServerConfig::opcode:
        callHandler<WireFormat::GetServerConfig, MembershipService,
            &MembershipService::getServerConfig>(rpc);
        break;
    case WireFormat::UpdateServerList::opcode:
        callHandler<WireFormat::UpdateServerList, MembershipService,
            &MembershipService::updateServerList>(rpc);
        break;
    default:
        throw UnimplementedRequestError(HERE);
    }
}

/**
 * Top-level service method to handle the GET_SERVER_CONFIG request.
 *
 * Yes, this is out of place in the membership service, but it needs to be
 * handled by a service that will always be present and it seems silly to
 * introduce one for a single RPC. If there are others, perhaps we will
 * want a generic server information service.
 *
 * \copydetails Service::ping
 */
void
MembershipService::getServerConfig(
    const WireFormat::GetServerConfig::Request* reqHdr,
    WireFormat::GetServerConfig::Response* respHdr,
    Rpc* rpc)
{
    ProtoBuf::ServerConfig serverConfigBuf;
    serverConfig->serialize(serverConfigBuf);
    respHdr->serverConfigLength = ProtoBuf::serializeToResponse(
        rpc->replyPayload, &serverConfigBuf);
}

/**
 * Top-level service method to handle the GET_SERVER_ID request.
 *
 * \copydetails Service::ping
 */
void
MembershipService::updateServerList(
    const WireFormat::UpdateServerList::Request* reqHdr,
    WireFormat::UpdateServerList::Response* respHdr,
    Rpc* rpc)
{
    ProtoBuf::ServerList list;
    ProtoBuf::parseFromRequest(rpc->requestPayload, sizeof(*reqHdr),
                               reqHdr->serverListLength, &list);

    serverList->applyServerList(list);
}

} // namespace RAMCloud
