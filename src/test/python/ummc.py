#!/usr/bin/env python2.6
# -*- coding: utf-8 -*-
#
#  ummd-client.py ( Micro MultiMedia Client )
#
#  Copyright (C) 2010 Kevin Roy <kiniou@gmail.com)
#

#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

# <pep8 compliant>

import sys, os, os.path

from twisted.internet.protocol import DatagramProtocol
from twisted.internet import reactor
from twisted.internet import task
from twisted.application.internet import MulticastServer

class MulticastClientUDP(DatagramProtocol):

    
    def startProtocol(self):
        self.transport.joinGroup('224.0.0.1')
        self.loop = task.LoopingCall(self.datagramSend, ("224.0.0.1",8005))
        self.loop.start(2.0,now=False)

    def datagramReceived(self, datagram, (host,port)):
        if "Pong!" in datagram :
            print("[ummd-client] Received:" + repr(datagram) + " from %s:%s" % (host,port))

    def datagramSend(self,(host,port)):
        print("[ummd-client] Sending \"Ping?\" to %s:%s" % (host,port))
        self.transport.write("Ping?",(host,port))


if __name__ == "__main__":

    reactor.listenMulticast(8005,MulticastClientUDP(),listenMultiple=True)
    reactor.run()


