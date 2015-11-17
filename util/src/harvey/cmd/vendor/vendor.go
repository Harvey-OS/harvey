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
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"os/exec"
	"path"
	"path/filepath"
	"strings"
)

const ignore = "*\n!.gitignore\n"

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

	f, err := ioutil.ReadFile("vendor.json")
	if err != nil {
		log.Fatal(err)
	}

	vendor := &V{}
	if err := json.Unmarshal(f, vendor); err != nil {
		log.Fatal(err)
	}

	if _, err := os.Stat("upstream"); os.IsExist(err) {
		run("chmod", "-R", "+w", "upstream")
		run("git", "rm", "-r", "-f", "upstream")
	}

	if err := do(vendor); err != nil {
		log.Fatal(err)
	}

	os.Mkdir("patch", 0755)
	os.Mkdir("build", 0755)
	ig, err := os.Create(path.Join("build", ".gitignore"))
	if err != nil {
		log.Fatal(err)
	}
	defer ig.Close()
	if _, err := ig.WriteString(ignore); err != nil {
		log.Fatal(err)
	}
	run("git", "add", ig.Name())
	run("git", "add", "vendor.json")
	run("git", "commit", "-s", "-m", "vendor: pull in "+path.Base(vendor.Upstream))
}

func do(v *V) error {
	name := fetch(v)
	f, err := os.Open(name)
	if err != nil {
		return err
	}
	defer os.Remove(name)

	var unZ io.Reader
	switch v.Compress {
	case "gzip":
		unZ, err = gzip.NewReader(f)
		if err != nil {
			return err
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
		os.MkdirAll(path.Dir(n), 0700)
		out, err := os.Create(n)
		if err != nil {
			log.Println(err)
			continue
		}

		if n, err := io.Copy(out, ar); n != h.Size || err != nil {
			return err
		}
		out.Close()
	}
	if err != io.EOF {
		return err
	}

	if err := filepath.Walk("upstream", readonly); err != nil {
		return err
	}

	return run("git", "add", "upstream")
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

func fetch(v *V) string {
	if len(v.Digest) == 0 {
		log.Fatal("no checksums specifed")
	}

	f, err := ioutil.TempFile("", "cmdVendor")
	if err != nil {
		log.Fatal(err)
	}
	defer f.Close()

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
	return f.Name()
}

func run(exe string, arg ...string) error {
	cmd := exec.Command(exe, arg...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}
