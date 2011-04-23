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

import sys, os, os.path, socket

from twisted.internet.protocol import DatagramProtocol
from twisted.internet import reactor
from twisted.internet import task
from twisted.application.internet import MulticastServer

from pprint import pprint, pformat

class MulticastServerUDP(DatagramProtocol):

    def __init__(self,x_name):
        self.name = x_name
        self.multicastgroup = "224.0.0.1"
        self.multicastport = 8005
        print("[ummd-node] Starting node %s" % (self.name))

    def startProtocol(self):
        print("[ummd-node] Joining multicast group %s" % (self.name))
        self.transport.joinGroup(self.multicastgroup)

    def datagramReceived(self, datagram, (host,port)):
        # The uniqueID check is to ensure we only service requests from ourselves
        if 'Ping?' in datagram:
            print("[ummd-node %s] Received:" % self.name + repr(datagram) + " from %s:%s" %(host,port))
            self.transport.write("Pong!! Im \"%s\"" % self.name ,(host,port))

if __name__ == "__main__":
    
    reactor.listenMulticast(8005,MulticastServerUDP(socket.gethostname()),listenMultiple=True)
    reactor.run()


