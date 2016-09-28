package main

import (
	"github.com/jurgen-kluft/xcode"
	"github.com/jurgen-kluft/xp2p/package"
)

func main() {
	xcode.Generate(xp2p.GetPackage())
}
