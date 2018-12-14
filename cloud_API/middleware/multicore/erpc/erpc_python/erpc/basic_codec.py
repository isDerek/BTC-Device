#!/usr/bin/env python

# The Clear BSD License
# Copyright (c) 2015 Freescale Semiconductor, Inc.
# Copyright 2016-2017 NXP
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted (subject to the limitations in the disclaimer below) provided
# that the following conditions are met:
#
# o Redistributions of source code must retain the above copyright notice, this list
#   of conditions and the following disclaimer.
#
# o Redistributions in binary form must reproduce the above copyright notice, this
#   list of conditions and the following disclaimer in the documentation and/or
#   other materials provided with the distribution.
#
# o Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import struct
from .codec import (MessageType, MessageInfo, Codec, CodecError)

class BasicCodec(Codec):
    ## Version of this codec.
    BASIC_CODEC_VERSION = 1

    def start_write_message(self, msgInfo):
        header = (self.BASIC_CODEC_VERSION << 24) \
                        | ((msgInfo.service & 0xff) << 16) \
                        | ((msgInfo.request & 0xff) << 8) \
                        | (msgInfo.type.value & 0xff)
        self.write_uint32(header)
        self.write_uint32(msgInfo.sequence)

    def _write(self, fmt, value):
        self._buffer += struct.pack(fmt, value)
        self._cursor += struct.calcsize(fmt)

    def write_bool(self, value):
        self._write('<?', value)

    def write_int8(self, value):
        self._write('<b', value)

    def write_int16(self, value):
        self._write('<h', value)

    def write_int32(self, value):
        self._write('<i', value)

    def write_int64(self, value):
        self._write('<q', value)

    def write_uint8(self, value):
        self._write('<B', value)

    def write_uint16(self, value):
        self._write('<H', value)

    def write_uint32(self, value):
        self._write('<I', value)

    def write_uint64(self, value):
        self._write('<Q', value)

    def write_float(self, value):
        self._write('<f', value)

    def write_double(self, value):
        self._write('<d', value)

    def write_string(self, value):
        self.write_binary(value.encode())

    def write_binary(self, value):
        self.write_uint32(len(value))
        self._buffer += value

    def start_write_list(self, length):
        self.write_uint32(length)

    def start_write_union(self, discriminator):
        self.write_uint32(discriminator)

    def write_null_flag(self, flag):
        self.write_uint8(1 if flag else 0)

    ##
    # @return 4-tuple of msgType, service, request, sequence.
    def start_read_message(self):
        header = self.read_uint32()
        sequence = self.read_uint32()
        version = header >> 24
        if version != self.BASIC_CODEC_VERSION:
            raise CodecError("unsupported codec version %d" % version)
        service = (header >> 16) & 0xff
        request = (header >> 8) & 0xff
        msgType = MessageType(header & 0xff)
        return MessageInfo(type=msgType, service=service, request=request, sequence=sequence)

    def _read(self, fmt):
        result = struct.unpack_from(fmt, self._buffer, self._cursor)
        self._cursor += struct.calcsize(fmt)
        return result[0]

    def read_bool(self):
        return self._read('<?')

    def read_int8(self):
        return self._read('<b')

    def read_int16(self):
        return self._read('<h')

    def read_int32(self):
        return self._read('<i')

    def read_int64(self):
        return self._read('<q')

    def read_uint8(self):
        return self._read('<B')

    def read_uint16(self):
        return self._read('<H')

    def read_uint32(self):
        return self._read('<I')

    def read_uint64(self):
        return self._read('<Q')

    def read_float(self):
        return self._read('<f')

    def read_double(self):
        return self._read('<d')

    def read_string(self):
        return self.read_binary().decode()

    def read_binary(self):
        length = self.read_uint32()
        data = self._buffer[self._cursor:self._cursor+length]
        self._cursor += length
        return data

    ##
    # @return Int of list length.
    def start_read_list(self):
        return self.read_uint32()

    ##
    # @return Int of union discriminator.
    def start_read_union(self):
        return self.read_int32()

    def read_null_flag(self):
        return self.read_uint8()



