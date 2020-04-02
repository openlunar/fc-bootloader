#!/usr/bin/env python

# Copyright 2016 NXP
# All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause

from .codec import MessageType, ReponseType

ResponseErrors = [
    ReponseType.E_NO_SERVER,
    ReponseType.E_NO_METHOD,
    ReponseType.E_SYNTAX,
    ReponseType.E_UNAVAIL,
]

class RequestError(RuntimeError):
    pass

class ClientManager(object):
    def __init__(self, transport=None, codecClass=None):
        self._transport = transport
        self._arbitrator = None
        self._codecClass = codecClass
        self._sequence = 0
        self._info = None

    @property
    def transport(self):
        return self._transport

    @transport.setter
    def transport(self, value):
        self._transport = value

    @property
    def arbitrator(self):
        return self._arbitrator

    @arbitrator.setter
    def arbitrator(self, arb):
        self._arbitrator = arb

    @property
    def codec_class(self):
        return self._codecClass

    @codec_class.setter
    def codec_class(self, value):
        self._codecClass = value

    @property
    def sequence(self):
        self._sequence += 1
        # LOGAN: Sequence is limited to 5 bits
        self._sequence &= 0x1F
        return self._sequence

    # NOTE: Removed support for "oneWay" messages
    def create_request(self):
        msg = bytearray()
        codec = self.codec_class()
        codec.buffer = msg
        return RequestContext(self.sequence, msg, codec)

    def perform_request(self, request):
        # Arbitrate requests.
        token = None
        if self._arbitrator is not None and not request.is_oneway:
            token = self._arbitrator.prepare_client_receive(request)

        # Send serialized request to server.
        self.transport.send(request.codec.buffer)

        if token is not None:
            msg = self._arbitrator.client_receive(token)
        else:
            msg = self.transport.receive()
        request.codec.buffer = msg

        self._info = request.codec.start_read_message()
        print(self._info)

        # Presently this client implementation only supports message type of "single normal"
        if self._info.type != MessageType.kSingleNormal:
            raise RequestError("invalid reply message type")

        # The sequence in the response should match the sequence that was sent
        if self._info.sequence != request.sequence:
            raise RequestError("unexpected sequence number in reply (was %d, expected %d)"
                        % (self._info.sequence, request.sequence))

        # Check the protocol response for an error (e.g. E_NO_SERVICE)
        r = ReponseType(self._info.protocol)
        if r in ResponseErrors:
            raise RequestError("Response failed: {0}".format(r))

class RequestContext(object):
    def __init__(self, sequence, message, codec):
        self._sequence = sequence
        self._message = message
        self._codec = codec

    @property
    def sequence(self):
        return self._sequence

    @property
    def message(self):
        return self._message

    @property
    def codec(self):
        return self._codec


