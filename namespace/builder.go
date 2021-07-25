package namespace

import (
	"errors"
	"fmt"
	"io"
	"os"
)

// OpenFunc opens files
type OpenFunc func(path string) (io.Reader, error)

// Builder helps building plan9 namespaces
type Builder struct {
	dir  string
	file File

	open OpenFunc
}

func open1(path string) (io.Reader, error) { return os.Open(path) }

// NewBuilder returns the default runner
func NewBuilder() (*Builder, error) {
	wd, err := os.Getwd()
	if err != nil {
		return nil, err
	}
	return &Builder{
		dir:  wd,
		open: open1,
	}, nil
}

func newRunner(wd string, b OpenFunc) (*Builder, error) {
	return &Builder{
		dir:  wd,
		open: b,
	}, nil
}

// Parse takes a path and parses the namespace file
func (b *Builder) Parse(file string) error {
	var f io.Reader
	var err error
	f, err = b.open(file)
	if err != nil {
		return err
	}
	b.file, err = Parse(f)
	if err != nil {
		return err
	}
	return nil
}

// Run takes a namespace and runs commands defined in the namespace file
func (b *Builder) buildNS(ns Namespace) error {
	for _, c := range b.file {
		var err error
		args := []string{}
		for _, arg := range c.args {
			args = append(args, os.ExpandEnv(arg))
		}

		switch syzcall(c.syscall) {
		case BIND:
			if len(args) < 2 {
				return errors.New("bind doesn't have  enough arguments")
			}
			err = ns.Bind(args[0], args[1], c.flag)
		case MOUNT:
			if len(args) < 2 {
				return errors.New("mount doesn't have  enough arguments")
			}
			servername := args[0]
			old := args[1]
			spec := ""
			if len(args) == 3 {
				spec = args[2]
			}
			err = ns.Mount(servername, old, spec, c.flag)
		case UNMOUNT:
			new, old := "", ""
			if len(args) == 2 {
				new = args[0]
				old = args[1]
			} else {
				old = args[0]
			}
			err = ns.Unmount(new, old)
		case RFORK:
			err = ns.Clear()
		case CHDIR:
			err = ns.Chdir(args[0])
			b.dir = args[0]
		case IMPORT:
			host := ""
			remotepath := ""
			mountpoint := ""
			if len(args) == 2 {
				host = args[0]
				mountpoint = args[1]
			} else if len(args) == 3 {
				host = args[0]
				remotepath = args[1]
				mountpoint = args[2]
			}
			err = ns.Import(host, remotepath, mountpoint, c.flag)
		case INCLUDE:
			var nb *Builder
			nb, err = newRunner(b.dir, b.open)
			if err != nil {
				break
			}
			err = nb.Parse(args[0])
			if err != nil {
				break
			}
			err = nb.buildNS(ns)
			if err != nil {
				break
			}
			b.dir = nb.dir // if the new file has changed the directory we'd like to know
		default:
			err = fmt.Errorf("%s not implmented", c.syscall)
		}
		if err != nil {
			return fmt.Errorf("newns failed to perform %s failed: %v", c, err)
		}
	}
	return nil
}
