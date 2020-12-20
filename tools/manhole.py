# This is a script that allows you to talk to the embedded python interpreter
# using ssh. To use it, put it in the GUIScript directory and then add the
# following to include.py, or issue using the python console
#
# import manhole
# manhole.createManhole(port = 2222, users = { 'gemrb': 'password' })
#
# Then you can connect using ssh to the given port and issue python commands.


import GemRB
from twisted.internet import reactor

reactor.startRunning()
GemRB.SetTickHook(lambda: reactor.iterate())

from twisted.conch import manhole, manhole_ssh
from twisted.conch.insults import insults
from twisted.cred import checkers, portal
def createManhole(port = 2222, users = { 'gemrb': 'password' }):
	"""Create a twisted manhole for accessing the embedded python interpreter"""
	namespace = { 'GemRB' : GemRB }

	def makeProtocol():
		return insults.ServerProtocol(manhole.ColoredManhole, namespace)
	r = manhole_ssh.TerminalRealm()
	r.chainedProtocolFactory = makeProtocol
	c = checkers.InMemoryUsernamePasswordDatabaseDontUse()
	for (username, password) in users.iteritems():
		c.addUser(username, password)
	p = portal.Portal(r, [c])
	f = manhole_ssh.ConchFactory(p)
	return reactor.listenTCP(port, f)
