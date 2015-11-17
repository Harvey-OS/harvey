package main

import (
	"archive/tar"
	"bytes"
	"compress/bzip2"
	"compress/gzip"
	"crypto/sha1"
	"crypto/sha256"
	"crypto/sha512"
	"encoding/hex"
	"encoding/json"
	"flag"
	"hash"
	"io"
	"log"
	"net/http"
	"os"
	"path"
	"path/filepath"
	"strings"
)

type V struct {
	Upstream     string
	Digest       map[string]string
	Compress     string
	RemovePrefix bool
	Exclude      []string
}

func main() {
	log.SetFlags(log.Lshortfile | log.LstdFlags)
	flag.Parse()

	f, err := os.Open("vendor.json")
	if err != nil {
		log.Fatal(err)
	}
	defer f.Close()

	vendor := &V{}
	if err := json.NewDecoder(f).Decode(vendor); err != nil {
		log.Fatal(err)
	}

	do(vendor)
}

func do(v *V) {
	f, err := os.Create("cmdVendor")
	if err != nil {
		log.Fatal(err)
	}
	defer os.Remove(f.Name())

	req, err := http.NewRequest("GET", v.Upstream, nil)
	if err != nil {
		log.Fatal(err)
	}
	client := &http.Client{
		Transport: &http.Transport{
			Proxy:              http.ProxyFromEnvironment,
			DisableCompression: true,
		},
	}
	res, err := client.Do(req)
	if err != nil {
		log.Fatal(err)
	}
	defer res.Body.Close()

	var digests []match
	for k, v := range v.Digest {
		g, err := hex.DecodeString(v)
		if err != nil {
			log.Fatal(err)
		}
		switch k {
		case "sha1":
			digests = append(digests, match{sha1.New(), g, k})
		case "sha224":
			digests = append(digests, match{sha256.New224(), g, k})
		case "sha256":
			digests = append(digests, match{sha256.New(), g, k})
		case "sha384":
			digests = append(digests, match{sha512.New384(), g, k})
		case "sha512":
			digests = append(digests, match{sha512.New(), g, k})
		}
	}
	ws := make([]io.Writer, len(digests))
	for i := range digests {
		ws[i] = digests[i]
	}
	w := io.MultiWriter(ws...)

	if _, err := io.Copy(f, io.TeeReader(res.Body, w)); err != nil {
		log.Fatal(err)
	}
	for _, h := range digests {
		if !h.OK() {
			log.Fatalf("mismatched %q hash\n\tWanted %x\n\tGot %x\n", h.Name, h.Good, h.Hash.Sum(nil))
		}
	}

	if _, err := f.Seek(0, 0); err != nil {
		log.Fatal(err)
	}

	var unZ io.Reader
	switch v.Compress {
	case "gzip":
		unZ, err = gzip.NewReader(f)
		if err != nil {
			log.Fatal(err)
		}
	case "bzip2":
		unZ = bzip2.NewReader(f)
	default:
		unZ = f
	}

	ar := tar.NewReader(unZ)
	h, err := ar.Next()
untar:
	for ; err == nil; h, err = ar.Next() {
		n := h.Name
		if v.RemovePrefix {
			n = strings.SplitN(n, "/", 2)[1]
		}
		for _, ex := range v.Exclude {
			if strings.HasPrefix(n, ex) {
				continue untar
			}
		}
		n = path.Join("upstream", n)
		if h.FileInfo().IsDir() {
			os.MkdirAll(n, 0700)
			continue
		}
		out, err := os.Create(n)
		if err != nil {
			log.Println(err)
			continue
		}
		defer out.Close()

		if n, err := io.Copy(out, ar); n != h.Size || err != nil {
			log.Fatal(err, n, h.Size)
		}
	}
	if err != io.EOF {
		log.Fatal(err)
	}

	if err := filepath.Walk("upstream", readonly); err != nil {
		log.Fatal(err)
	}
}

type match struct {
	hash.Hash
	Good []byte
	Name string
}

func (m match) OK() bool {
	return bytes.Equal(m.Good, m.Hash.Sum(nil))
}

func readonly(path string, fi os.FileInfo, err error) error {
	if err != nil {
		return err
	}
	if fi.IsDir() {
		return os.Chmod(path, 0555)
	}
	return os.Chmod(path, 0444)
}
