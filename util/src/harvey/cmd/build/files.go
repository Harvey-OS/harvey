package main

import (
	"os"
	"path/filepath"
)

func mvAll(files []string, to string) error {
	for _, f := range files {
		if err := mv(f, to); err != nil {
			return err
		}
	}
	return nil
}

func mv(f string, to string) error {
	fi, err := os.Stat(to)
	if err != nil {
		return err
	}
	if fi.IsDir() {
		to = filepath.Join(to, filepath.Base(f))
	}
	if err := os.Rename(f, to); err != nil {
		return err
	}
	return nil
}
