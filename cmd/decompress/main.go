// decompress is a tool for decompressing compressed files

package main

import (
	"flag"
	"fmt"
	"io"
	"log"
	"os"

	"github.com/ulikunitz/xz/lzma"
)

var (
	usage = func() {
		fmt.Fprintf(flag.CommandLine.Output(), "Usage: decompress [file]\n")
		flag.PrintDefaults()
		os.Exit(1)
	}
)

func decompress(infile *os.File, outfile *os.File) error {
	r, err := lzma.NewReader(infile)
	if err != nil {
		return fmt.Errorf("couldn't open lzma file: %w", err)
	}

	_, err = io.Copy(outfile, r)
	if err != nil {
		return fmt.Errorf("couldn't copy lzma file: %w", err)
	}

	return nil
}

func main() {
	flag.Usage = usage
	flag.Parse()

	filepaths := flag.Args()
	if len(filepaths) == 0 {
		// Decompress from stdin
		decompress(os.Stdin, os.Stdout)
	} else {
		// Decompress files
		if len(filepaths) > 1 {
			usage()
		}

		filepath := filepaths[0]
		file, err := os.Open(filepath)
		if err != nil {
			log.Fatalf("couldn't decompress file %v error %v", filepath, err.Error())
		}
		decompress(file, os.Stdout)
		file.Close()
	}
}
