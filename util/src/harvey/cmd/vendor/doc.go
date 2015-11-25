/*
Vendor is a tool to vendor software for harvey.

It downloads a tarball, verifies it against supplied hashes, extracts it
into "upstream", modifies all the files to be read-only, and then commits
the results.

Vendor is purposely unhelpful and un-customisable.

VENDORFILE

It requires a "vendor.json" file in the current directory with the following
structure:

	{
		"Upstream":"",
		"Digest": {
			"":""
		},
		"Compress":"",
		"RemovePrefix": true,
		"Exclude": [
			""
		]
	}

"Upstream" is the URL to fetch a tarball from.

"Digest" is a map of algorithm-hash pairs for calculating checksums. All
of the sha functions in the go standard library are supported. The hash is
hex-encoded, just like sha*sum output.

"Compress" is the compression type of the tarball. Gzip and bzip are
supported. If this key is omitted, the tarball is assumed to be uncompressed.

"RemovePrefix" is a boolean toggle for if the first element of files in the
archive should be removed. Defaults to false if omitted.

"Exclude" is an array of prefix strings for files that should not be
extracted. They are used as literal prefixes and not interpreted in any way.
*/
package main
