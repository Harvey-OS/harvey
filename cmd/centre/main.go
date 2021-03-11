// Copyright 2018 the u-root Authors. All rights reserved
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// centre is used to support one or more of DHCP, TFTP, and HTTP services
// on harvey networks.
package main

import (
	"bufio"
	"bytes"
	"flag"
	"fmt"
	"log"
	"math"
	"net"
	"net/http"
	"os"
	"runtime"
	"strings"
	"sync"
	"time"

	"github.com/insomniacslk/dhcp/dhcpv4"
	"github.com/insomniacslk/dhcp/dhcpv4/bsdp"
	"github.com/insomniacslk/dhcp/dhcpv4/server4"
	"github.com/insomniacslk/dhcp/dhcpv6"
	"github.com/insomniacslk/dhcp/dhcpv6/server6"
	"harvey-os.org/ninep/protocol"
	"harvey-os.org/ninep/ufs"
	"pack.ag/tftp"
)

var (
	// TODO: get info from centre ipv4
	inf = flag.String("i", "eth0", "Interface to serve DHCPv4 on")

	// DHCPv4-specific
	ipv4         = flag.Bool("4", true, "IPv4 DHCP server")
	rootpath     = flag.String("rootpath", "", "RootPath option to serve via DHCPv4")
	bootfilename = flag.String("bootfilename", "pxelinux.0", "Boot file to serve via DHCPv4")
	raspi        = flag.Bool("raspi", false, "Configure to boot Raspberry Pi")
	dnsServers   = flag.String("dns", "", "Comma-separated list of DNS servers for DHCPv4")
	gateway      = flag.String("gw", "", "Optional gateway IP for DHCPv4")
	hostFile     = flag.String("hostfile", "", "Optional additional hosts file for DHCPv4")

	// DHCPv6-specific
	ipv6           = flag.Bool("6", false, "DHCPv6 server")
	v6Bootfilename = flag.String("v6-bootfilename", "", "Boot file to serve via DHCPv6")

	// File serving
	tftpDir    = flag.String("tftp-dir", "", "Directory to serve over TFTP")
	tftpPort   = flag.Int("tftp-port", 69, "Port to serve TFTP on")
	httpDir    = flag.String("http-dir", "", "Directory to serve over HTTP")
	httpPort   = flag.Int("http-port", 80, "Port to serve HTTP on")
	ninepDir   = flag.String("ninep-dir", "", "Directory to serve over 9p")
	ninepAddr  = flag.String("ninep-addr", ":5640", "addr to serve 9p on")
	ninepDebug = flag.Int("ninep-debug", 0, "Debug level for ninep -- for now, only non-zero matters")
)

type dserver4 struct {
	mac          net.HardwareAddr
	yourIP       net.IP
	submask      net.IPMask
	self         net.IP
	bootfilename string
	rootpath     string
	dns          []net.IP
	hostFile     string
}

// lookupIP looks up an IP address corresponding to the given address
// for mac address, prefix a "u", e.g. "u00:11:22:33:44:55"
func lookupIP(hostFile string, addr string) ([]net.IP, error) {
	var err error
	// First try the override
	if hostFile != `` {
		// Read the file and walk each line looking for a match.
		// We do this so you can update the file without restarting the server
		var f *os.File
		if f, err = os.Open(hostFile); err != nil {
			return nil, err
		}
		// We're going to be real simple-minded. We take each line to consist of
		// one IP, followed by one or more whitespace-separated hostnames
		scan := bufio.NewScanner(f)
		for scan.Scan() {
			fields := strings.Fields(scan.Text())
			if len(fields) < 2 {
				continue
			}
			for _, fld := range fields {
				if strings.ToLower(fld) == strings.ToLower(addr) {
					return []net.IP{net.ParseIP(fields[0])}, nil
				}
			}
		}
	}
	// Now just do a regular lookup since we didn't find it in the override
	return net.LookupIP(addr)
}

func lookupHostname(hostFile string, ip net.IP) (string, error) {
	var err error
	// First try the override
	if hostFile != `` {
		// Read the file and walk each line looking for a match.
		// We do this so you can update the file without restarting the server
		var f *os.File
		if f, err = os.Open(hostFile); err != nil {
			return "", err
		}
		// We're going to be real simple-minded. We take each line to consist of
		// one IP, followed by one or more whitespace-separated hostnames
		scan := bufio.NewScanner(f)
		for scan.Scan() {
			fields := strings.Fields(scan.Text())
			// You have to have at least 3 fields: <ip> <hostname>... <mac addr>
			if len(fields) < 3 {
				continue
			}
			if fields[0] != ip.String() {
				continue
			}
			// just return the first hostname given
			return fields[1], nil
		}
	}
	// Now just do a regular lookup since we didn't find it in the override
	names, err := net.LookupAddr(ip.String())
	if len(names) == 0 {
		return "", nil
	}
	return names[0], nil
}

func (s *dserver4) dhcpHandler(conn net.PacketConn, peer net.Addr, m *dhcpv4.DHCPv4) {
	log.Printf("Handling request %v for peer %v", m, peer)

	var replyType dhcpv4.MessageType
	switch mt := m.MessageType(); mt {
	case dhcpv4.MessageTypeDiscover:
		replyType = dhcpv4.MessageTypeOffer
	case dhcpv4.MessageTypeRequest:
		replyType = dhcpv4.MessageTypeAck
	default:
		log.Printf("Can't handle type %v", mt)
		return
	}

	i, err := lookupIP(s.hostFile, fmt.Sprintf("u%s", m.ClientHWAddr))
	if err != nil {
		log.Printf("Not responding to DHCP request for mac %s", m.ClientHWAddr)
		log.Printf("You can create a host entry of the form 'a.b.c.d [names] u%s' 'ip6addr [names] u%s'if you wish", m.ClientHWAddr, m.ClientHWAddr)
		return
	}

	// Since this is dserver4, we force it to be an ip4 address.
	ip := i[0].To4()

	// get the hostname
	hostname, err := lookupHostname(s.hostFile, ip)
	if err != nil {
		log.Printf("Failure in trying to look up hostname: %v", err)
		return
	}

	modifiers := []dhcpv4.Modifier{
		dhcpv4.WithMessageType(replyType),
		dhcpv4.WithServerIP(s.self),
		dhcpv4.WithRouter(s.self),
		dhcpv4.WithNetmask(s.submask),
		dhcpv4.WithYourIP(ip),
		// RFC 2131, Section 4.3.1. Server Identifier: MUST
		dhcpv4.WithOption(dhcpv4.OptServerIdentifier(s.self)),
		// RFC 2131, Section 4.3.1. IP lease time: MUST
		dhcpv4.WithOption(dhcpv4.OptIPAddressLeaseTime(dhcpv4.MaxLeaseTime)),
	}
	if hostname != `` {
		modifiers = append(modifiers, dhcpv4.WithOption(dhcpv4.OptHostName(hostname)))
	}
	if *raspi {
		modifiers = append(modifiers,
			dhcpv4.WithOption(dhcpv4.OptClassIdentifier("PXEClient")),
			// Add option 43, suboption 9 (PXE native boot menu) to allow Raspberry Pi to recognise the offer
			dhcpv4.WithOption(dhcpv4.Option{
				Code: dhcpv4.OptionVendorSpecificInformation,
				Value: bsdp.VendorOptions{Options: dhcpv4.OptionsFromList(
					// The dhcp package only seems to support Apple BSDP boot menu items,
					// so we have to craft the option by hand.
					// \x11 is the length of the 'Raspberry Pi Boot' string...
					dhcpv4.OptGeneric(dhcpv4.GenericOptionCode(9), []byte("\000\000\x11Raspberry Pi Boot")),
				)},
			}),
		)
	}
	if len(s.dns) != 0 {
		modifiers = append(modifiers, dhcpv4.WithDNS(s.dns...))
	}
	if *gateway != `` {
		modifiers = append(modifiers, dhcpv4.WithGatewayIP(net.ParseIP(*gateway)))
		modifiers = append(modifiers, dhcpv4.WithRouter(net.ParseIP(*gateway)))
	}
	reply, err := dhcpv4.NewReplyFromRequest(m, modifiers...)

	// RFC 6842, MUST include Client Identifier if client specified one.
	if val := m.Options.Get(dhcpv4.OptionClientIdentifier); len(val) > 0 {
		reply.UpdateOption(dhcpv4.OptGeneric(dhcpv4.OptionClientIdentifier, val))
	}
	if len(s.bootfilename) > 0 {
		reply.BootFileName = s.bootfilename
	}
	if len(s.rootpath) > 0 {
		reply.UpdateOption(dhcpv4.OptRootPath(s.rootpath))
	}
	if err != nil {
		log.Printf("Could not create reply for %v: %v", m, err)
		return
	}

	// Experimentally determined. You can't just blindly send a broadcast packet
	// with the broadcast address. You can, however, send a broadcast packet
	// to a subnet for an interface. That actually makes some sense.
	// This fixes the observed problem that OSX just swallows these
	// packets if the peer is 255.255.255.255.
	// I chose this way of doing it instead of files with build constraints
	// because this is not that expensive and it's just a tiny bit easier to
	// follow IMHO.
	if runtime.GOOS == "darwin" {
		p := &net.UDPAddr{IP: s.yourIP.Mask(s.submask), Port: 68}
		log.Printf("Changing %v to %v", peer, p)
		peer = p
	}

	log.Printf("Sending %v to %v", reply.Summary(), peer)
	if _, err := conn.WriteTo(reply.ToBytes(), peer); err != nil {
		log.Printf("Could not write %v: %v", reply, err)
	}
}

type dserver6 struct {
	mac         net.HardwareAddr
	yourIP      net.IP
	bootfileurl string
}

func (s *dserver6) dhcpHandler(conn net.PacketConn, peer net.Addr, m dhcpv6.DHCPv6) {
	log.Printf("Handling DHCPv6 request %v sent by %v", m.Summary(), peer.String())

	msg, err := m.GetInnerMessage()
	if err != nil {
		log.Printf("Could not find unpacked message: %v", err)
		return
	}

	if msg.MessageType != dhcpv6.MessageTypeSolicit {
		log.Printf("Only accept SOLICIT message type, this is a %s", msg.MessageType)
		return
	}
	if msg.GetOneOption(dhcpv6.OptionRapidCommit) == nil {
		log.Printf("Only accept requests with rapid commit option.")
		return
	}
	if mac, err := dhcpv6.ExtractMAC(msg); err != nil {
		log.Printf("No MAC address in request: %v", err)
		return
	} else if s.mac != nil && !bytes.Equal(s.mac, mac) {
		log.Printf("MAC address %s doesn't match expected MAC %s", mac, s.mac)
		return
	}

	// From RFC 3315, section 17.1.4, If the client includes a Rapid Commit
	// option in the Solicit message, it will expect a Reply message that
	// includes a Rapid Commit option in response.
	reply, err := dhcpv6.NewReplyFromMessage(msg)
	if err != nil {
		log.Printf("Failed to create reply for %v: %v", m, err)
		return
	}

	iana := msg.Options.OneIANA()
	if iana != nil {
		iana.Options.Update(&dhcpv6.OptIAAddress{
			IPv6Addr:          s.yourIP,
			PreferredLifetime: math.MaxUint32 * time.Second,
			ValidLifetime:     math.MaxUint32 * time.Second,
		})
		reply.AddOption(iana)
	}
	if len(s.bootfileurl) > 0 {
		reply.Options.Add(dhcpv6.OptBootFileURL(s.bootfileurl))
	}

	if _, err := conn.WriteTo(reply.ToBytes(), peer); err != nil {
		log.Printf("Failed to send response %v: %v", reply, err)
		return
	}

	log.Printf("DHCPv6 request successfully handled, reply: %v", reply.Summary())
}

func main() {
	flag.Parse()

	var wg sync.WaitGroup
	if len(*tftpDir) != 0 {
		wg.Add(1)
		go func() {
			defer wg.Done()
			server, err := tftp.NewServer(fmt.Sprintf(":%d", *tftpPort))
			if err != nil {
				log.Fatalf("Could not start TFTP server: %v", err)
			}

			log.Println("starting file server")
			server.ReadHandler(tftp.FileServer(*tftpDir))
			log.Fatal(server.ListenAndServe())
		}()
	}
	if len(*httpDir) != 0 {
		wg.Add(1)
		go func() {
			defer wg.Done()
			http.Handle("/", http.FileServer(http.Dir(*httpDir)))
			log.Fatal(http.ListenAndServe(fmt.Sprintf(":%d", *httpPort), nil))
		}()
	}

	var dns []net.IP
	parts := strings.Split(*dnsServers, ",")
	for _, p := range parts {
		ip := net.ParseIP(p)
		if ip != nil {
			dns = append(dns, ip)
		}
	}

	if *inf != "" {
		centre, err := lookupIP(*hostFile, "centre")
		if err != nil {
			log.Printf("No centre entry found via LookupIP: not serving DHCP")
		} else if *ipv4 {
			ip := centre[0].To4()
			wg.Add(1)
			go func() {
				defer wg.Done()
				s := &dserver4{
					self:         ip,
					bootfilename: *bootfilename,
					rootpath:     *rootpath,
					submask:      ip.DefaultMask(),
					dns:          dns,
					hostFile:     *hostFile,
				}

				laddr := &net.UDPAddr{Port: dhcpv4.ServerPort}
				server, err := server4.NewServer(*inf, laddr, s.dhcpHandler)
				if err != nil {
					log.Fatal(err)
				}
				if err := server.Serve(); err != nil {
					log.Fatal(err)
				}
			}()
		}
	}

	// not yet.
	if false && *ipv6 && *inf != "" {
		wg.Add(1)
		go func() {
			defer wg.Done()

			s := &dserver6{
				bootfileurl: *v6Bootfilename,
			}
			laddr := &net.UDPAddr{
				IP:   net.IPv6unspecified,
				Port: dhcpv6.DefaultServerPort,
			}
			server, err := server6.NewServer("eth0", laddr, s.dhcpHandler)
			if err != nil {
				log.Fatal(err)
			}

			log.Println("starting dhcpv6 server")
			if err := server.Serve(); err != nil {
				log.Fatal(err)
			}
		}()
	}

	// TODO: serve on ip6
	if len(*ninepDir) != 0 {
		ln, err := net.Listen("tcp4", *ninepAddr)
		if err != nil {
			log.Fatalf("Listen failed: %v", err)
		}

		ufslistener, err := ufs.NewUFS(*ninepDir, *ninepDebug, func(l *protocol.NetListener) error {
			l.Trace = nil
			if *ninepDebug > 1 {
				l.Trace = log.Printf
			}
			return nil
		})

		if err != nil {
			log.Fatal(err)
		}
		if err := ufslistener.Serve(ln); err != nil {
			log.Fatal(err)
		}

	}
	wg.Wait()
}
