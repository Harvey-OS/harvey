package main

import (
	"encoding/json"
	"log"
	"os"
)

// compCmd is a Compilation DB compCmd object
type compCmd struct {
	Directory string   `json:"directory"`
	Command   string   `json:"command,omitempty"`
	File      string   `json:"file"`
	Arguments []string `json:"arguments,omitempty"`
	Output    string   `json:"output,omitempty"`
}

var compDB = make(chan compCmd)

type collector []compCmd

func (c *collector) run() {
	for {
		select {
		case cmd, ok := <-compDB:
			if !ok {
				return
			}
			*c = append(*c, cmd)
		}
	}
}

func (c collector) Print(s string) error {
	log.Printf("writing compilation db to %s\n", s)
	f, err := os.Create(s)
	if err != nil {
		return err
	}
	return json.NewEncoder(f).Encode(c)
}
