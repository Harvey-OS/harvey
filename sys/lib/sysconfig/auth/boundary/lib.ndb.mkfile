ATTR=sys ip ipnet tcp il udp dom ether port authdom hostid database soa

all:V:
	9fs kfs
	9fs $fileserver
	mk local.^($ATTR)
	mk local-cs.^($ATTR)
	mk common.^($ATTR)
	mk friends.^($ATTR)
	mk trip.^($ATTR)
	mk mesa.^($ATTR)
	mk external.^($ATTR)
	mk external-cs.^($ATTR)

local.%: local
	ndb/mkhash local $stem
local-cs.%: local-cs
	ndb/mkhash local-cs $stem
common.%: common
	ndb/mkhash common $stem
external.%: external
	ndb/mkhash external $stem
external-cs.%: external-cs
	ndb/mkhash external-cs $stem
friends.%: friends
	ndb/mkhash friends $stem
trip.%: trip
	ndb/mkhash trip $stem
mesa.%: mesa
	ndb/mkhash mesa $stem

local: /n/$fileserver/lib/ndb/local
	cp /n/$fileserver/lib/ndb/local /n/kfs/lib/ndb

local-cs: /n/$fileserver/lib/ndb/local-cs
	cp /n/$fileserver/lib/ndb/local-cs /n/kfs/lib/ndb

common: /n/$fileserver/lib/ndb/common
	cp /n/$fileserver/lib/ndb/common /n/kfs/lib/ndb

friends: /n/$fileserver/lib/ndb/friends
	cp /n/$fileserver/lib/ndb/friends /n/kfs/lib/ndb

trip: /n/$fileserver/lib/ndb/trip
	cp /n/$fileserver/lib/ndb/trip /n/kfs/lib/ndb

mesa: /n/$fileserver/lib/ndb/mesa
	cp /n/$fileserver/lib/ndb/mesa /n/kfs/lib/ndb

external: /n/$fileserver/lib/ndb/external
	cp /n/$fileserver/lib/ndb/external /n/kfs/lib/ndb

external-cs: /n/$fileserver/lib/ndb/external-cs
	cp /n/$fileserver/lib/ndb/external-cs /n/kfs/lib/ndb

