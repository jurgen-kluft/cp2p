package main

import (
	"github.com/jurgen-kluft/ccode"
	"github.com/jurgen-kluft/cp2p/package"
)

func main() {
	ccode.Init()
	ccode.Generate(cp2p.GetPackage())
}
